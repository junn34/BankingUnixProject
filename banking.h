#ifndef BANKING_H
#define BANKING_H


// 클라이언트와 서버가 통신할 때 사용하는 UNIX 소켓 파일 경로
#define SOCKET_PATH "banking_socket"

// 사용자 이름, 비밀번호, 데이터 버퍼의 최대 크기 정의
#define MAX_NAME_LEN 50    // 계좌 소유자의 이름 최대 길이
#define MAX_PASS_LEN 20    // 계좌 비밀번호 최대 길이
#define BUFFER_SIZE 1024    // 데이터 전송 시 사용하는 버퍼 크기

// 계좌 정보를 저장하는 구조체 정의
typedef struct {
    char name[MAX_NAME_LEN];       // 사용자 이름
    char account_number[20];       // 계좌 번호
    char password[MAX_PASS_LEN];   // 계좌 비밀번호
    double balance;                // 계좌 잔액
} Account;

#endif 

