#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>



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
        perror("Socket creation failed");
        exit(1);
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Connection to server failed");
        exit(1);
    }

    int choice;
    do {
        printf("\n==== Banking System ====\n");
        printf("1. Create Account\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. Show Transactions\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        write(client_fd, &choice, sizeof(choice));

        switch (choice) {
            case 1: create_account(client_fd); break;
            case 2: deposit(client_fd); break;
            case 3: withdraw(client_fd); break;
            case 4: show_transactions(client_fd); break;
            case 5: printf("Exiting...\n"); break;
            default: printf("Invalid choice. Try again.\n");
        }
    } while (choice != 5);

    close(client_fd);
    return 0;
}

void create_account(int socket_fd) {
    Account account;

    printf("\n--- Create Account ---\n");
    printf("Enter your name: ");
    scanf("%s", account.name);
    printf("Enter your account number: ");
    scanf("%s", account.account_number);
    printf("Enter your password: ");
    scanf("%s", account.password);

    write(socket_fd, &account, sizeof(Account));
    receive_message(socket_fd);
}

void deposit(int socket_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;

    printf("\n--- Deposit ---\n");
    printf("Enter account number: ");
    scanf("%s", account_number);
    write(socket_fd, account_number, sizeof(account_number));

    printf("Enter account holder name: ");
    scanf("%s", name);
    write(socket_fd, name, sizeof(name));

    printf("Enter password: ");
    scanf("%s", password);
    write(socket_fd, password, sizeof(password));

    printf("Enter amount to deposit: ");
    scanf("%lf", &amount);
    write(socket_fd, &amount, sizeof(amount));

    receive_message(socket_fd);
}

void withdraw(int socket_fd) {
    char account_number[20], name[MAX_NAME_LEN], password[MAX_PASS_LEN];
    double amount;

    printf("\n--- Withdraw ---\n");
    printf("Enter account number: ");
    scanf("%s", account_number);
    write(socket_fd, account_number, sizeof(account_number));

    printf("Enter account holder name: ");
    scanf("%s", name);
    write(socket_fd, name, sizeof(name));

    printf("Enter password: ");
    scanf("%s", password);
    write(socket_fd, password, sizeof(password));

    printf("Enter amount to withdraw: ");
    scanf("%lf", &amount);
    write(socket_fd, &amount, sizeof(amount));

    receive_message(socket_fd);
}

void show_transactions(int socket_fd) {
    char account_number[20], buffer[BUFFER_SIZE];

    printf("\n--- Show Transactions ---\n");
    printf("Enter account number: ");
    scanf("%s", account_number);
    write(socket_fd, account_number, sizeof(account_number));

    printf("\n--- Transaction Log ---\n");
    while (1) {
        if (read(socket_fd, buffer, sizeof(buffer)) <= 0) {
            break;
        }
        if (strcmp(buffer, "END\n") == 0) {
            break;
        }
        printf("%s", buffer);
    }
}

void receive_message(int socket_fd) {
    char response[BUFFER_SIZE];
    if (read(socket_fd, response, sizeof(response)) > 0) {
        printf("%s\n", response);
    }
}
