#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include "banking.h"

void create_account(int client_fd);
void deposit(int client_fd);
void withdraw(int client_fd);
void transaction_history(int client_fd);
void log_transaction(const char *account_number, const char *operation, double amount, double balance);
void send_message(int client_fd, const char *message);

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("소켓 생성 실패");
        exit(1);
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);
    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        exit(1);
    }

    printf("서버 실행 중...\n");

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        printf("클라이언트 연결 완료\n");

        while (1) {
            int choice;

            if (read(client_fd, &choice, sizeof(choice)) <= 0) {
                perror("기능 번호 읽기 실패");
                break;
            }

            switch (choice) {
                case 1: create_account(client_fd); break;
                case 2: deposit(client_fd); break;
                case 3: withdraw(client_fd); break;
                case 4: transaction_history(client_fd); break;
                case 5:
                    printf("클라이언트가 exit 호출\n");
                    close(client_fd);
                    break;
                default:
                    printf("유효하지 않은 옵션 : %d\n", choice);
                    break;
            }
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}

void create_account(int client_fd) {
    Account account;
    char account_file[BUFFER_SIZE];

    if (read(client_fd, &account, sizeof(Account)) <= 0) {
        perror("계좌 정보 읽기 오류");
        return;
    }

    snprintf(account_file, sizeof(account_file), "%s.dat", account.account_number);
    FILE *file = fopen(account_file, "w");
    if (!file) {
        perror("계좌 파일 생성 오류");
        send_message(client_fd, "ERROR: 계좌 파일 생성 실패.\n");
        return;
    }

    account.balance = 0;
    fprintf(file, "이름: %s\n", account.name);
    fprintf(file, "계좌 번호: %s\n", account.account_number);
    fprintf(file, "비밀번호: %s\n", account.password);
    fprintf(file, "잔액: %.2lf\n", account.balance);
    fclose(file);

    printf("계좌 생성: %s\n", account_file);
    log_transaction(account.account_number, "CREATE", 0, account.balance);
    send_message(client_fd, "계좌 생성 성공.\n");
}

void deposit(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];
    Account account;

    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        perror("계좌 번호 읽기 오류");
        send_message(client_fd, "ERROR: 계좌 번호 읽기 실패.\n");
        return;
    }
    printf("입금 요청 계좌 번호: %s\n", account_number);

    snprintf(account_file, sizeof(account_file), "%s.dat", account_number);
    FILE *file = fopen(account_file, "r");
    if (!file) {
        perror("계좌 파일 열기 오류");
        send_message(client_fd, "ERROR: 계좌 파일 찾을 수 없음.\n");
        return;
    }

    fscanf(file, "이름: %[^\n]\n", account.name);
    fscanf(file, "계좌 번호: %[^\n]\n", account.account_number);
    fscanf(file, "비밀번호: %[^\n]\n", account.password);
    fscanf(file, "잔액: %lf\n", &account.balance);
    fclose(file);

    if (read(client_fd, name, sizeof(name)) <= 0) {
        perror("이름 읽기 오류");
        send_message(client_fd, "ERROR: 이름 읽기 실패.\n");
        return;
    }

    if (strcmp(account.name, name) != 0) {
        printf("입력한 이름 불일치.\n");
        send_message(client_fd, "ERROR: 이름 불일치.\n");
        return;
    }

    if (read(client_fd, password, sizeof(password)) <= 0) {
        perror("비밀번호 읽기 오류");
        send_message(client_fd, "ERROR: 비밀번호 읽기 실패.\n");
        return;
    }

    if (strcmp(account.password, password) != 0) {
        printf("비밀번호 불일치.\n");
        send_message(client_fd, "ERROR: 비밀번호 불일치.\n");
        return;
    }

    if (read(client_fd, &amount, sizeof(amount)) <= 0) {
        perror("입금 금액 읽기 오류");
        return;
    }

    account.balance += amount;

    file = fopen(account_file, "w");
    if (!file) {
        perror("계좌 파일 업데이트 오류");
        return;
    }
    fprintf(file, "이름: %s\n", account.name);
    fprintf(file, "계좌 번호: %s\n", account.account_number);
    fprintf(file, "비밀번호: %s\n", account.password);
    fprintf(file, "잔액: %.2lf\n", account.balance);
    fclose(file);

    printf("입금 성공. 업데이트된 잔액: %.2lf\n", account.balance);
    log_transaction(account.account_number, "DEPOSIT", amount, account.balance);
    send_message(client_fd, "입금이 완료되었습니다.\n");
}

void withdraw(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];
    Account account;

    // Account number 읽기
    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        perror("계좌 번호 읽기 오류");
        send_message(client_fd, "ERROR: 계좌 번호 읽기 실패.\n");
        return;
    }
    printf("출금 요청 계좌 번호: %s\n", account_number);

    snprintf(account_file, sizeof(account_file), "%s.dat", account_number);
    FILE *file = fopen(account_file, "r");
    if (!file) {
        perror("계좌 파일 열기 오류");
        send_message(client_fd, "ERROR: 계좌 파일 찾을 수 없음.\n");
        return;
    }

    fscanf(file, "이름: %[^\n]\n", account.name);
    fscanf(file, "계좌 번호: %[^\n]\n", account.account_number);
    fscanf(file, "비밀번호: %[^\n]\n", account.password);
    fscanf(file, "잔액: %lf\n", &account.balance);
    fclose(file);

    // Name 확인
    if (read(client_fd, name, sizeof(name)) <= 0) {
        perror("이름 읽기 오류");
        send_message(client_fd, "ERROR: 이름 읽기 실패.\n");
        return;
    }

    if (strcmp(account.name, name) != 0) {
        printf("입력한 이름 불일치.\n");
        send_message(client_fd, "ERROR: 이름 불일치.\n");
        return;
    }

    // Password 확인
    if (read(client_fd, password, sizeof(password)) <= 0) {
        perror("비밀번호 읽기 오류");
        send_message(client_fd, "ERROR: 비밀번호 읽기 실패.\n");
        return;
    }

    if (strcmp(account.password, password) != 0) {
        printf("비밀번호 불일치.\n");
        send_message(client_fd, "ERROR: 비밀번호 불일치.\n");
        return;
    }

    // Withdraw 금액 읽기
    if (read(client_fd, &amount, sizeof(amount)) <= 0) {
        perror("출금 금액 읽기 오류");
        return;
    }

    if (account.balance < amount) {
        printf("잔액 부족. 잔액: %.2lf, 요청 금액: %.2lf\n", account.balance, amount);
        send_message(client_fd, "ERROR: 잔액 부족.\n");
        return;
    }

    account.balance -= amount;

    // 파일 업데이트
    file = fopen(account_file, "w");
    if (!file) {
        perror("계좌 파일 업데이트 오류");
        return;
    }
    fprintf(file, "이름: %s\n", account.name);
    fprintf(file, "계좌 번호: %s\n", account.account_number);
    fprintf(file, "비밀번호: %s\n", account.password);
    fprintf(file, "잔액: %.2lf\n", account.balance);
    fclose(file);

    printf("출금 성공. 업데이트된 잔액: %.2lf\n", account.balance);
    log_transaction(account.account_number, "WITHDRAW", amount, account.balance);
    send_message(client_fd, "출금이 완료되었습니다.\n");
}

void transaction_history(int client_fd) {
    char account_number[20], account_file[BUFFER_SIZE], line[BUFFER_SIZE];

    // Account number 읽기
    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        perror("계좌 번호 읽기 오류");
        send_message(client_fd, "ERROR: 계좌 번호 읽기 실패.\n");
        return;
    }
    printf("거래 내역 요청 계좌 번호: %s\n", account_number);

    snprintf(account_file, sizeof(account_file), "%s.log", account_number);
    FILE *file = fopen(account_file, "r");
    if (!file) {
        perror("거래 내역 파일 열기 오류");
        send_message(client_fd, "ERROR: 거래 내역 파일을 찾을 수 없습니다.\n");
        return;
    }

    printf("거래 내역 파일 열기 성공: %s\n", account_file);

    while (fgets(line, sizeof(line), file)) {
        if (write(client_fd, line, strlen(line))<=0){
	   perror("거래 내역 전송 오류");
	   fclose(file);
	   return;
        }
    }
    fclose(file);

    // 거래 내역의 끝 표시
    if(write(client_fd, "END\n", 4)<=0){
       perror("END 메시지 전송 오류");
    }
    printf("거래 내역 전송 완료.\n");
}


void log_transaction(const char *account_number, const char *operation, double amount, double balance) {
    char log_file[BUFFER_SIZE];
    snprintf(log_file, sizeof(log_file), "%s.log", account_number);

    FILE *file = fopen(log_file, "a");
    if (!file) {
        perror("거래 로그 파일 생성 오류");
        return;
    }

    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline

    fprintf(file, "%s | %s | %.2lf | %.2lf\n", time_str, operation, amount, balance);
    fclose(file);
}

void send_message(int client_fd, const char *message) {
    write(client_fd, message, strlen(message) + 1);
}

