#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>



void create_account(int client_fd);
void deposit(int client_fd);
void withdraw(int client_fd);
void transaction_history(int client_fd);
void log_transaction(const char *filename, const char *operation, double amount, double balance);
void send_message(int client_fd, const char *message);
int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;

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

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected.\n");

        int choice;
        if (read(client_fd, &choice, sizeof(choice)) <= 0) {
            perror("Failed to read choice");
            close(client_fd);
            continue;
        }

        switch (choice) {
            case 1: create_account(client_fd); break;
            case 2: deposit(client_fd); break;
            case 3: withdraw(client_fd); break;
            case 4: transaction_history(client_fd); break;
            case 5:
                printf("Client requested exit.\n");
                close(client_fd);
                continue; // 다른 클라이언트 요청을 처리하기 위해 루프 유지
            default:
                printf("Invalid option received: %d\n", choice);
                break;
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

    // 클라이언트에서 계좌 정보 읽기
    if (read(client_fd, &account, sizeof(Account)) <= 0) {
        perror("Error reading account data");
        return;
    }

    snprintf(account_file, sizeof(account_file), "%s.dat", account.account_number);

    FILE *file = fopen(account_file, "w");
    if (!file) {
        perror("Error creating account file");
        return;
    }

    account.balance = 0;
    fprintf(file, "Name: %s\n", account.name);
    fprintf(file, "Account Number: %s\n", account.account_number);
    fprintf(file, "Password: %s\n", account.password);
    fprintf(file, "Balance: %.2lf\n", account.balance);
    fclose(file);

    printf("Account created: %s\n", account_file);
    log_transaction(account_file, "CREATE", 0, account.balance);
    send_message(client_fd, "SUCCESS: Account created\n");
}

void deposit(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];
    Account account;

    // Account number 읽기
    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        perror("Error reading account number");
        send_message(client_fd, "ERROR: Failed to read account number\n");
        return;
    }

    snprintf(account_file, sizeof(account_file), "%s.dat", account_number);

    FILE *file = fopen(account_file, "r");
    if (!file) {
        perror("Error opening account file");
        send_message(client_fd, "ERROR: Account file not found\n");
        return;
    }

    fscanf(file, "Name: %[^\n]\n", account.name);
    fscanf(file, "Account Number: %[^\n]\n", account.account_number);
    fscanf(file, "Password: %[^\n]\n", account.password);
    fscanf(file, "Balance: %lf\n", &account.balance);
    fclose(file);

    // Name 확인
    if (read(client_fd, name, sizeof(name)) <= 0) {
        perror("Error reading name");
        send_message(client_fd, "ERROR: Failed to read name\n");
        return;
    }

    if (strcmp(account.name, name) != 0) {
        printf("Name mismatch: Expected %s, Got %s\n", account.name, name);
        send_message(client_fd, "ERROR: Name mismatch\n");
        return;
    }

    // Password 확인
    if (read(client_fd, password, sizeof(password)) <= 0) {
        perror("Error reading password");
        send_message(client_fd, "ERROR: Failed to read password\n");
        return;
    }

    if (strcmp(account.password, password) != 0) {
        printf("Password mismatch.\n");
        send_message(client_fd, "ERROR: Incorrect password\n");
        return;
    }

    // Deposit 금액 읽기
    if (read(client_fd, &amount, sizeof(amount)) <= 0) {
        perror("Error reading deposit amount");
        return;
    }

    account.balance += amount;

    // 파일 업데이트
    file = fopen(account_file, "w");
    if (!file) {
        perror("Error updating account file");
        return;
    }
    fprintf(file, "Name: %s\n", account.name);
    fprintf(file, "Account Number: %s\n", account.account_number);
    fprintf(file, "Password: %s\n", account.password);
    fprintf(file, "Balance: %.2lf\n", account.balance);
    fclose(file);

    printf("Deposit successful. New balance: %.2lf\n", account.balance);
    log_transaction(account_file, "DEPOSIT", amount, account.balance);
    send_message(client_fd, "SUCCESS: Deposit completed\n");
}

void withdraw(int client_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;
    char account_file[BUFFER_SIZE];
    Account account;

    // Account number 읽기
    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        perror("Error reading account number");
        send_message(client_fd, "ERROR: Failed to read account number\n");
        return;
    }

    snprintf(account_file, sizeof(account_file), "%s.dat", account_number);

    FILE *file = fopen(account_file, "r");
    if (!file) {
        perror("Error opening account file");
        send_message(client_fd, "ERROR: Account file not found\n");
        return;
    }

    fscanf(file, "Name: %[^\n]\n", account.name);
    fscanf(file, "Account Number: %[^\n]\n", account.account_number);
    fscanf(file, "Password: %[^\n]\n", account.password);
    fscanf(file, "Balance: %lf\n", &account.balance);
    fclose(file);

    // Name 확인
    if (read(client_fd, name, sizeof(name)) <= 0) {
        perror("Error reading name");
        send_message(client_fd, "ERROR: Failed to read name\n");
        return;
    }

    if (strcmp(account.name, name) != 0) {
        printf("Name mismatch: Expected %s, Got %s\n", account.name, name);
        send_message(client_fd, "ERROR: Name mismatch\n");
        return;
    }

    // Password 확인
    if (read(client_fd, password, sizeof(password)) <= 0) {
        perror("Error reading password");
        send_message(client_fd, "ERROR: Failed to read password\n");
        return;
    }

    if (strcmp(account.password, password) != 0) {
        printf("Password mismatch.\n");
        send_message(client_fd, "ERROR: Incorrect password\n");
        return;
    }

    // Withdraw 금액 읽기
    if (read(client_fd, &amount, sizeof(amount)) <= 0) {
        perror("Error reading withdraw amount");
        return;
    }

    if (account.balance < amount) {
        printf("Insufficient funds: Balance %.2lf, Withdraw %.2lf\n", account.balance, amount);
        send_message(client_fd, "ERROR: Insufficient funds\n");
        return;
    }

    account.balance -= amount;

    // 파일 업데이트
    file = fopen(account_file, "w");
    if (!file) {
        perror("Error updating account file");
        return;
    }
    fprintf(file, "Name: %s\n", account.name);
    fprintf(file, "Account Number: %s\n", account.account_number);
    fprintf(file, "Password: %s\n", account.password);
    fprintf(file, "Balance: %.2lf\n", account.balance);
    fclose(file);

    printf("Withdrawal successful. New balance: %.2lf\n", account.balance);
    log_transaction(account_file, "WITHDRAW", amount, account.balance);
    send_message(client_fd, "SUCCESS: Withdrawal completed\n");
}

void transaction_history(int client_fd) {
    char account_number[20], account_file[BUFFER_SIZE], line[BUFFER_SIZE];

    if (read(client_fd, account_number, sizeof(account_number)) <= 0) {
        perror("Error reading account number");
        send_message(client_fd, "ERROR: Failed to read account number\n");
        return;
    }

    snprintf(account_file, sizeof(account_file), "%s.dat.log", account_number);

    FILE *file = fopen(account_file, "r");
    if (!file) {
        perror("Error opening transaction log");
        send_message(client_fd, "ERROR: Transaction log not found\n");
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        write(client_fd, line, strlen(line));
    }
    fclose(file);
    write(client_fd, "END\n", 4); // End marker
}

void log_transaction(const char *filename, const char *operation, double amount, double balance) {
    char log_file[BUFFER_SIZE];
    snprintf(log_file, sizeof(log_file), "%s.log", filename);

    FILE *file = fopen(log_file, "a");
    if (!file) {
        perror("Error creating transaction log");
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
