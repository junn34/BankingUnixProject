#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include "banking.h"

// 함수 선언
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
    unlink(SOCKET_PATH); // 기존 소켓 제거
    //소켓 바인딩
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Bind 실패");
        close(server_fd);
        exit(1);
    }
    //클라이언트 요청 대기
    if (listen(server_fd, 5) == -1) {
        perror("Listen 실패");
        close(server_fd);
        exit(1);
    }

    printf("서버 실행 중...\n");

    while (1) {
        //클라이언트 연결 수락
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("클라이언트 연결 실패");
            continue;
        }

        printf("클라이언트 연결 완료\n");

        while (1) {
            int choice;
            //기능 번호 읽기
            if (read(client_fd, &choice, sizeof(choice)) <= 0) {
                close(client_fd);
                break;
            }
            //클라이언트 요청 처리
            switch (choice) {
                case 1:
                    create_account(client_fd);
                    break;
                case 2:
                    deposit(client_fd);
                    break;
                case 3:
                    withdraw(client_fd);
                    break;
                case 4:
                    transaction_history(client_fd);
                    break;
                case 5:
                    printf("클라이언트 연결 종료 요청\n");
                    close(client_fd);
                    return 0;
                default:
                    send_message(client_fd, "ERROR: 유효하지 않은 옵션입니다.\n");
                    break;
            }
        }
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}
// 계좌 생성 함수
void create_account(int client_fd) {
    Account account;
    char account_file[BUFFER_SIZE];
    //계좌 정보 수신
    if (read(client_fd, &account, sizeof(Account)) <= 0) {
        send_message(client_fd, "ERROR: 계좌 정보 읽기 실패.\n");
        return;
    }
    //계좌 파일 생성(.dat)
    snprintf(account_file, sizeof(account_file), "%s.dat", account.account_number);
    FILE *file = fopen(account_file, "w");
    if (!file) {
        send_message(client_fd, "ERROR: 계좌 파일 생성 실패.\n");
        return;
    }
    //잔액 초기화
    account.balance = 0;
    fprintf(file, "이름: %s\n", account.name);
    fprintf(file, "계좌 번호: %s\n", account.account_number);
    fprintf(file, "비밀번호: %s\n", account.password);
    fprintf(file, "잔액: %.2lf\n", account.balance);
    fclose(file);
    //거래 내역 생성
    log_transaction(account.account_number, "CREATE", 0, account.balance);
    send_message(client_fd, "계좌 생성 성공.");
}
//입금 함수
void deposit(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];
    Account account;
    //계좌 번호 읽기
    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        send_message(client_fd, "ERROR: 계좌 번호 읽기 실패.\n");
        return;
    }
    //계좌 파일 열기 (.dat)
    snprintf(account_file, sizeof(account_file), "%s.dat", account_number);
    FILE *file = fopen(account_file, "r");
    if (!file) {
        send_message(client_fd, "ERROR: 계좌 파일을 찾을 수 없습니다.\n");
        return;
    }
    
    fscanf(file, "이름: %[^\n]\n", account.name);
    fscanf(file, "계좌 번호: %[^\n]\n", account.account_number);
    fscanf(file, "비밀번호: %[^\n]\n", account.password);
    fscanf(file, "잔액: %lf\n", &account.balance);
    fclose(file);

    if (read(client_fd, name, sizeof(name)) <= 0) {
        send_message(client_fd, "ERROR: 이름 읽기 실패.\n");
        return;
    }

    if (strcmp(account.name, name) != 0) {
        send_message(client_fd, "ERROR: 이름 불일치.\n");
        return;
    }

    if (read(client_fd, password, sizeof(password)) <= 0) {
        send_message(client_fd, "ERROR: 비밀번호 읽기 실패.\n");
        return;
    }

    if (strcmp(account.password, password) != 0) {
        send_message(client_fd, "ERROR: 비밀번호 불일치.\n");
        return;
    }

    if (read(client_fd, &amount, sizeof(amount)) <= 0) {
        send_message(client_fd, "ERROR: 입금 금액 읽기 실패.\n");
        return;
    }
    //잔액 업데이트
    account.balance += amount;
   //계좌 파일 업데이트
    file = fopen(account_file, "w");
    if (!file) {
        send_message(client_fd, "ERROR: 계좌 파일 업데이트 실패.\n");
        return;
    }

    fprintf(file, "이름: %s\n", account.name);
    fprintf(file, "계좌 번호: %s\n", account.account_number);
    fprintf(file, "비밀번호: %s\n", account.password);
    fprintf(file, "잔액: %.2lf\n", account.balance);
    fclose(file);

    log_transaction(account.account_number, "DEPOSIT", amount, account.balance);
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "입금이 완료되었습니다. 현재 잔액: %.2lf", account.balance);
    send_message(client_fd, message);
}
//출금 함수
void withdraw(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];
    Account account;
   //계좌 번호 읽기
    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        send_message(client_fd, "ERROR: 계좌 번호 읽기 실패.\n");
        return;
    }
   //계좌 파일 열기
    snprintf(account_file, sizeof(account_file), "%s.dat", account_number);
    FILE *file = fopen(account_file, "r");
    if (!file) {
        send_message(client_fd, "ERROR: 계좌 파일을 찾을 수 없습니다.\n");
        return;
    }
    //계좌 정보 읽기
    fscanf(file, "이름: %[^\n]\n", account.name);
    fscanf(file, "계좌 번호: %[^\n]\n", account.account_number);
    fscanf(file, "비밀번호: %[^\n]\n", account.password);
    fscanf(file, "잔액: %lf\n", &account.balance);
    fclose(file);
   
    //이름 확인 
    if (read(client_fd, name, sizeof(name)) <= 0) {
        send_message(client_fd, "ERROR: 이름 읽기 실패.\n");
        return;
    }
   
    if (strcmp(account.name, name) != 0) {
        send_message(client_fd, "ERROR: 이름 불일치.\n");
        return;
    }
    //비밀번호 확인
    if (read(client_fd, password, sizeof(password)) <= 0) {
        send_message(client_fd, "ERROR: 비밀번호 읽기 실패.\n");
        return;
    }

    if (strcmp(account.password, password) != 0) {
        send_message(client_fd, "ERROR: 비밀번호 불일치.\n");
        return;
    }
   //출금 금액 확인
    if (read(client_fd, &amount, sizeof(amount)) <= 0) {
        send_message(client_fd, "ERROR: 출금 금액 읽기 실패.\n");
        return;
    }

    if (account.balance < amount) {
        send_message(client_fd, "ERROR: 잔액 부족.\n");
        return;
    }
    //계좌 잔액 업데이트
    account.balance -= amount;
    //계좌 파일 업데이트
    file = fopen(account_file, "w");
    if (!file) {
        send_message(client_fd, "ERROR: 계좌 파일 업데이트 실패.\n");
        return;
    }
    
    fprintf(file, "이름: %s\n", account.name);
    fprintf(file, "계좌 번호: %s\n", account.account_number);
    fprintf(file, "비밀번호: %s\n", account.password);
    fprintf(file, "잔액: %.2lf\n", account.balance);
    fclose(file);
    //로그 파일 업데이트
    log_transaction(account.account_number, "WITHDRAW", amount, account.balance);
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "출금이 완료되었습니다. 현재 잔액: %.2lf", account.balance);
    send_message(client_fd,message);
}
//거래 내역 함수
void transaction_history(int client_fd) {
    char account_number[20], password[MAX_PASS_LEN], account_file[BUFFER_SIZE], line[BUFFER_SIZE];
    Account account;

    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        send_message(client_fd, "ERROR: 계좌 번호 읽기 실패.\n");
        return;
    }

    snprintf(account_file, sizeof(account_file), "%s.dat", account_number);
    FILE *file = fopen(account_file, "r");
    if (!file) {
        send_message(client_fd, "ERROR: 계좌 파일을 찾을 수 없습니다.\n");
        return;
    }
    //계좌 정보 읽기
    fscanf(file, "이름: %[^\n]\n", account.name);
    fscanf(file, "계좌 번호: %[^\n]\n", account.account_number);
    fscanf(file, "비밀번호: %[^\n]\n", account.password);
    fscanf(file, "잔액: %lf\n", &account.balance);
    fclose(file);

    if (read(client_fd, password, sizeof(password)) <= 0) {
        send_message(client_fd, "ERROR: 비밀번호 읽기 실패.\n");
        return;
    }

    if (strcmp(account.password, password) != 0) {
        send_message(client_fd, "ERROR: 비밀번호 불일치.\n");
        return;
    }

    snprintf(account_file, sizeof(account_file), "%s.log", account_number);
    file = fopen(account_file, "r");
    if (!file) {
        send_message(client_fd, "ERROR: 거래 내역 파일을 찾을 수 없습니다.\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        write(client_fd, line, strlen(line));
    }

    fclose(file);
    write(client_fd, "END\n", 4);
    send_message(client_fd, "거래 내역 확인 완료.");
}
//로그 파일 함수
void log_transaction(const char *account_number, const char *operation, double amount, double balance) {
    char log_file[BUFFER_SIZE];
    snprintf(log_file, sizeof(log_file), "%s.log", account_number);

    FILE *file = fopen(log_file, "a");
    if (!file) {
        return;
    }
    //거래 시간 정보
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; 

    fprintf(file, "%s | %s | %.2lf | %.2lf\n", time_str, operation, amount, balance);
    fclose(file);
}
//클라이언트로 메시지 전송하는 함수
void send_message(int client_fd, const char *message) {
    write(client_fd, message, strlen(message) + 1);
}

