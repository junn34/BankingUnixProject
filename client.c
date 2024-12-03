#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "banking.h"

// 함수 선언
void create_account(int socket_fd);
void deposit(int socket_fd);
void withdraw(int socket_fd);
void show_transactions(int socket_fd);
void receive_message(int socket_fd);

int main() {
    int client_fd;
    struct sockaddr_un addr;

    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("소켓 생성 실패");
        exit(1);
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("서버 연결 실패");
        close(client_fd);
        exit(1);
    }

    int choice;
    while (1) {
        printf("\n==== 은행 시스템 ====\n");
        printf("1. 계좌 생성\n");
        printf("2. 입금\n");
        printf("3. 출금\n");
        printf("4. 거래 내역 확인\n");
        printf("5. 프로그램 종료\n");
        printf("번호를 선택하세요: ");
        scanf("%d", &choice);

        if (write(client_fd, &choice, sizeof(choice)) <= 0) {
            perror("서버에 기능 번호 전송 실패");
            break;
        }

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
                show_transactions(client_fd);
                break;
            case 5:
                printf("프로그램을 종료합니다.\n");
                close(client_fd);
                exit(0); // 종료
            default:
                printf("잘못된 입력입니다. 다시 시도하세요.\n");
        }
    }

    close(client_fd);
    return 0;
}

void create_account(int socket_fd) {
    Account account;

    printf("\n--- 계좌 생성 ---\n");
    printf("이름을 입력하세요: ");
    scanf("%s", account.name);
    printf("계좌 번호를 입력하세요: ");
    scanf("%s", account.account_number);
    printf("비밀번호를 입력하세요: ");
    scanf("%s", account.password);

    if (write(socket_fd, &account, sizeof(Account)) <= 0) {
        perror("계좌 정보 전송 실패");
        return;
    }

    receive_message(socket_fd); // 서버 응답 수신
}

void deposit(int socket_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;

    printf("\n--- 입금 ---\n");
    printf("계좌 번호를 입력하세요: ");
    scanf("%s", account_number);
    if (write(socket_fd, account_number, sizeof(account_number)) <= 0) {
        perror("계좌 번호 전송 실패");
        return;
    }

    printf("계좌 생성 당시 이름을 입력하세요: ");
    scanf("%s", name);
    if (write(socket_fd, name, sizeof(name)) <= 0) {
        perror("이름 전송 실패");
        return;
    }

    printf("비밀번호를 입력하세요: ");
    scanf("%s", password);
    if (write(socket_fd, password, sizeof(password)) <= 0) {
        perror("비밀번호 전송 실패");
        return;
    }

    printf("입금 금액을 입력하세요: ");
    scanf("%lf", &amount);
    if (write(socket_fd, &amount, sizeof(amount)) <= 0) {
        perror("입금 금액 전송 실패");
        return;
    }

    receive_message(socket_fd); // 서버 응답 수신
}

void withdraw(int socket_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;

    printf("\n--- 출금 ---\n");
    printf("계좌 번호를 입력하세요: ");
    scanf("%s", account_number);
    if (write(socket_fd, account_number, sizeof(account_number)) <= 0) {
        perror("계좌 번호 전송 실패");
        return;
    }

    printf("계좌 생성 당시 이름을 입력하세요: ");
    scanf("%s", name);
    if (write(socket_fd, name, sizeof(name)) <= 0) {
        perror("이름 전송 실패");
        return;
    }

    printf("비밀번호를 입력하세요: ");
    scanf("%s", password);
    if (write(socket_fd, password, sizeof(password)) <= 0) {
        perror("비밀번호 전송 실패");
        return;
    }

    printf("출금 금액을 입력하세요: ");
    scanf("%lf", &amount);
    if (write(socket_fd, &amount, sizeof(amount)) <= 0) {
        perror("출금 금액 전송 실패");
        return;
    }

    receive_message(socket_fd); // 서버 응답 수신
}

void show_transactions(int socket_fd) {
    char account_number[20], password[MAX_PASS_LEN], buffer[BUFFER_SIZE];
    int read_data;

    printf("\n--- 거래 내역 확인 ---\n");
    printf("계좌 번호를 입력하세요: ");
    scanf("%s", account_number);

    printf("비밀번호를 입력하세요: ");
    scanf("%s", password);

    if (write(socket_fd, account_number, sizeof(account_number)) <= 0) {
        perror("계좌 번호 전송 실패");
        return;
    }

    if (write(socket_fd, password, sizeof(password)) <= 0) {
        perror("비밀번호 전송 실패");
        return;
    }

    printf("\n--- 거래 내역 ---\n");
    while (1) {
        read_data = read(socket_fd, buffer, sizeof(buffer) - 1);
        if (read_data <= 0) {
            break;
        }

        buffer[read_data] = '\0'; // NULL 종료
        if (strcmp(buffer, "END\n") == 0) {
            printf("--- 거래 내역 끝 ---\n");
            break;
        }
        printf("%s", buffer);
    }

    printf("\n거래 내역 확인 완료.\n");
}

void receive_message(int socket_fd) {
    char response[BUFFER_SIZE];
    if (read(socket_fd, response, sizeof(response)) > 0) {
        printf("%s\n", response);
    }
}

