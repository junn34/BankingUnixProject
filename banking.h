#ifndef BANKING_H
#define BANKING_H

#define SOCKET_PATH "banking_socket"
#define MAX_NAME_LEN 50
#define MAX_PASS_LEN 20
#define BUFFER_SIZE 256

typedef struct {
    char name[MAX_NAME_LEN];
    char account_number[20];
    char password[MAX_PASS_LEN];
    double balance;
} Account;

typedef struct {
    char timestamp[30];
    char operation[10];  // "DEPOSIT", "WITHDRAW", etc.
    double amount;
    double balance_after;
} Transaction;

#endif
