#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "banking.h"


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
        exit(1);
    }

    int choice;
    do {
        printf("\n==== 환영합니다 ====\n");
        printf("1. 계좌 생성\n");
        printf("2. 입금\n");
        printf("3. 출금\n");
        printf("4. 거래 내역 확인\n");
        printf("5. 종료\n");
        printf("사용하실 기능의 번호를 입력하세요.: ");
        scanf("%d", &choice);

        if(write(client_fd, &choice, sizeof(choice))<=0){
		perror("서버에 기능의 번호 전송이 실패했습니다.");
		break;
	}

        switch (choice) {
            case 1: create_account(client_fd); break;
            case 2: deposit(client_fd); break;
            case 3: withdraw(client_fd); break;
            case 4: show_transactions(client_fd); break;
            case 5:  
		printf("종료 중...\n"); 
		close(client_fd);
		exit(0);
            default: printf("유효하지 않은 기능입니다. 다시 시도해주세요.\n");
        }
    } while (choice != 5);

    close(client_fd);
    return 0;
}

void create_account(int socket_fd) {
    Account account;

    printf("\n--- 계좌 생성 ---\n");
    printf("이름을 입력해주세요.: ");
    scanf("%s", account.name);
    printf("계좌 번호를 입력해주세요.: ");
    scanf("%s", account.account_number);
    printf("비밀 번호를 입력해주세요.: ");
    scanf("%s", account.password);

    write(socket_fd, &account, sizeof(Account));
    receive_message(socket_fd);
}

void deposit(int socket_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;

    printf("\n--- 입금 ---\n");
    printf("계좌 번호를 입력해 주세요.: ");
    scanf("%s", account_number);
    write(socket_fd, account_number, sizeof(account_number));

    printf("계좌 생성 당시에 사용하신 이름을 입력해 주세요.: ");
    scanf("%s", name);
    write(socket_fd, name, sizeof(name));

    printf("비밀 번호를 입력해주세요.: ");
    scanf("%s", password);
    write(socket_fd, password, sizeof(password));

    printf("맡기실 금액을 입력해주세요.: ");
    scanf("%lf", &amount);
    write(socket_fd, &amount, sizeof(amount));

    receive_message(socket_fd);
}

void withdraw(int socket_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;

    printf("\n--- 출금 ---\n");
    printf("계좌 번호를 입력해주세요.: ");
    scanf("%s", account_number);
    write(socket_fd, account_number, sizeof(account_number));

    printf("계좌 생성 당시에 사용하신 이름을 입력해 주세요.: ");
    scanf("%s", name);
    write(socket_fd, name, sizeof(name));

    printf("비밀 번호를 입력해주세요: ");
    scanf("%s", password);
    write(socket_fd, password, sizeof(password));

    printf("찾으실 금액을 입력해주세요.: ");
    scanf("%lf", &amount);
    write(socket_fd, &amount, sizeof(amount));

    receive_message(socket_fd);
}

void show_transactions(int socket_fd) {
    char account_number[20], buffer[BUFFER_SIZE];
    int read_data;
    printf("\n--- 거래내역 확인  ---\n");
    printf("계좌 번호를 입력해주세요.: ");
    scanf("%s", account_number);
    if (write(socket_fd, account_number, sizeof(account_number))<=0){
	perror("계좌 번호 전송 실패");
	return;
    }
    printf("\n--- 거래 내역 ---\n");
    while (1) {
	read_data = read(socket_fd, buffer, sizeof(buffer)-1);
        if (read_data<= 0) {
       	    perror("거래 내역 읽기 오류");
	    break;
        }
	buffer[read_data] = '\0';
        if (strcmp(buffer, "END\n") == 0) {
            printf("\n--- 거래 내역 끝 ---\n");
            break;
        }
        printf("%s", buffer);
    }
    printf("\n메뉴로 돌아갑니다.\n");
}

void receive_message(int socket_fd) {
    char response[BUFFER_SIZE];
    if (read(socket_fd, response, sizeof(response)) > 0) {
        printf("%s\n", response);
    }
}
