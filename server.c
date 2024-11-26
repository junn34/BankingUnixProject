#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include "include/banking.h"

#define DATA_DIR "data"

void create_account(int client_fd);
void deposit(int client_fd);
void withdraw(int client_fd);
void show_transactions(int client_fd);
char *current_time();
int authenticate(const char *account_file, const char *password);
void log_transaction(const char *account_file, const char *operation, double amount, double balance);

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;

    // 소켓 초기화
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
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

    printf("Server is running...\n");

    // 클라이언트 연결 처리
    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected.\n");

        int choice;
        read(client_fd, &choice, sizeof(choice));
        switch (choice) {
            case 1: create_account(client_fd); break;
            case 2: deposit(client_fd); break;
            case 3: withdraw(client_fd); break;
            case 4: show_transactions(client_fd); break;
            default: printf("Invalid option.\n"); break;
        }

        close(client_fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}

void create_account(int client_fd) {
    Account account;
    char account_file[BUFFER_SIZE];
    read(client_fd, &account, sizeof(Account));

    snprintf(account_file, sizeof(account_file), "%s/%s.dat", DATA_DIR, account.account_number);

    FILE *file = fopen(account_file, "wb");
    if (!file) {
        perror("Account creation failed");
        return;
    }

    account.balance = 0;
    fwrite(&account, sizeof(Account), 1, file);
    fclose(file);

    char *time_str = current_time();
    log_transaction(account_file, "CREATE", 0, account.balance);
    free(time_str);

    printf("Account created: %s\n", account.account_number);
}

void deposit(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];

    read(client_fd, account_number, sizeof(account_number));
    snprintf(account_file, sizeof(account_file), "%s/%s.dat", DATA_DIR, account_number);

    FILE *file = fopen(account_file, "rb+");
    if (!file) {
        perror("Account not found");
        return;
    }

    Account account;
    fread(&account, sizeof(Account), 1, file);

    // 사용자 확인
    read(client_fd, name, sizeof(name));
    if (strcmp(account.name, name) != 0) {
        printf("Name mismatch.\n");
        fclose(file);
        return;
    }

    // 비밀번호 확인
    read(client_fd, password, sizeof(password));
    if (!authenticate(account_file, password)) {
        printf("Authentication failed.\n");
        fclose(file);
        return;
    }

    // 입금 처리
    read(client_fd, &amount, sizeof(amount));
    account.balance += amount;

    fseek(file, 0, SEEK_SET);
    fwrite(&account, sizeof(Account), 1, file);
    fclose(file);

    log_transaction(account_file, "DEPOSIT", amount, account.balance);

    printf("Deposit successful: %lf\n", amount);
}

void withdraw(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];

    read(client_fd, account_number, sizeof(account_number));
    snprintf(account_file, sizeof(account_file), "%s/%s.dat", DATA_DIR, account_number);

    FILE *file = fopen(account_file, "rb+");
    if (!file) {
        perror("Account not found");
        return;
    }

    Account account;
    fread(&account, sizeof(Account), 1, file);

    // 사용자 확인
    read(client_fd, name, sizeof(name));
    if (strcmp(account.name, name) != 0) {
        printf("Name mismatch.\n");
        fclose(file);
        return;
    }

    // 비밀번호 확인
    read(client_fd, password, sizeof(password));
    if (!authenticate(account_file, password)) {
        printf("Authentication failed.\n");
        fclose(file);
        return;
    }

    // 출금 처리
    read(client_fd, &amount, sizeof(amount));
    if (amount > account.balance) {
        printf("Insufficient funds.\n");
        fclose(file);
        return;
    }

    account.balance -= amount;

    fseek(file, 0, SEEK_SET);
    fwrite(&account, sizeof(Account), 1, file);
    fclose(file);

    log_transaction(account_file, "WITHDRAW", amount, account.balance);

    printf("Withdrawal successful: %lf\n", amount);
}

void show_transactions(int client_fd) {
    char account_number[20], account_file[BUFFER_SIZE];
    read(client_fd, account_number, sizeof(account_number));

    snprintf(account_file, sizeof(account_file), "%s/%s.dat", DATA_DIR, account_number);
    char log_file[BUFFER_SIZE];
    snprintf(log_file, sizeof(log_file), "%s.log", account_file);

    FILE *file = fopen(log_file, "r");
    if (!file) {
        perror("Transaction log not found");
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        write(client_fd, line, strlen(line) + 1);
    }
    fclose(file);
}

char *current_time() {
    time_t now = time(NULL);
    char *time_str = malloc(30);
    strftime(time_str, 30, "%Y-%m-%d %H:%M:%S", localtime(&now));
    return time_str;
}

int authenticate(const char *account_file, const char *password) {
    FILE *file = fopen(account_file, "rb");
    if (!file) return 0;

    Account account;
    fread(&account, sizeof(Account), 1, file);
    fclose(file);

    return strcmp(account.password, password) == 0;
}

void log_transaction(const char *account_file, const char *operation, double amount, double balance) {
    char log_file[BUFFER_SIZE];
    snprintf(log_file, sizeof(log_file), "%s.log", account_file);

    FILE *file = fopen(log_file, "a");
    if (!file) {
        perror("Transaction log creation failed");
        return;
    }

    char *time_str = current_time();
    fprintf(file, "%s | %s | %.2lf | %.2lf\n", time_str, operation, amount, balance);
    fclose(file);
    free(time_str);
}
