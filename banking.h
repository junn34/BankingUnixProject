#ifndef BANKING_H
#define BANKING_H

// 소켓 경로 및 버퍼 크기 정의
#define SOCKET_PATH "banking_socket"
#define MAX_NAME_LEN 50
#define MAX_PASS_LEN 20
#define BUFFER_SIZE 256

// 계좌 정보를 저장하는 구조체
typedef struct {
    char name[MAX_NAME_LEN];       // 사용자 이름
    char account_number[20];       // 계좌 번호
    char password[MAX_PASS_LEN];   // 계좌 비밀번호
    double balance;                // 계좌 잔액
} Account;

#endif
