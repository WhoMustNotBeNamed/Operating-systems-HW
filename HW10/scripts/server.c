#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_MSG_SIZE 1024

// Структура для передачи аргументов в поток
struct client_args {
    int client_socket;
    int other_client_socket;
};

// Функция обработки соединения с клиентом
void *handle_connection(void *arg) {
    struct client_args *args = (struct client_args *)arg;
    int client_socket = args->client_socket;
    int other_client_socket = args->other_client_socket;
    char buffer[MAX_MSG_SIZE];
    int bytes_received;

    while (1) {
        bytes_received = recv(client_socket, buffer, MAX_MSG_SIZE, 0);
        if (bytes_received == -1) {
            perror("Ошибка приема сообщения");
            close(client_socket);
            close(other_client_socket);
            pthread_exit(NULL);
        } else if (bytes_received == 0) {
            printf("Клиент отключился.\n");
            close(client_socket);
            close(other_client_socket);
            pthread_exit(NULL);
        } else {
            buffer[bytes_received] = '\0';
            printf("Получено от клиента: %s\n", buffer);
            if (strcmp(buffer, "The End") == 0) {
                printf("Завершение соединения...\n");
                close(client_socket);
                close(other_client_socket);
                pthread_exit(NULL);
            }

            // Определение, кому переслать сообщение
            int recipient_socket = (client_socket == args->client_socket) ? args->other_client_socket : args->client_socket;
            // Пересылка сообщения другому клиенту
            send(recipient_socket, buffer, strlen(buffer), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <IP-адрес> <порт>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int server_socket, client_socket1, client_socket2;
    struct sockaddr_in server_addr, client_addr1, client_addr2;
    socklen_t client_len1, client_len2;
    pthread_t tid1, tid2;
    struct client_args args1, args2;

    // Создание сокета
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    // Заполнение структуры для адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Привязка сокета к адресу сервера
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка привязки");
        exit(EXIT_FAILURE);
    }

    // Ожидание подключения клиента №1
    if (listen(server_socket, 2) == -1) {
        perror("Ошибка ожидания подключения");
        exit(EXIT_FAILURE);
    }

    // Получение информации об IP-адресе сервера
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    getsockname(server_socket, (struct sockaddr *)&address, &addr_len);
    printf("Сервер запущен на IP-адресе %s, порт %d. Ожидание подключения...\n", inet_ntoa(address.sin_addr), port);

    // Принятие соединения от клиента №1
    client_len1 = sizeof(client_addr1);
    client_socket1 = accept(server_socket, (struct sockaddr*)&client_addr1, &client_len1);
    if (client_socket1 == -1) {
        perror("Ошибка принятия соединения от клиента №1");
        exit(EXIT_FAILURE);
    }
    printf("Клиент №1 подключен.\n");

    // Принятие соединения от клиента №2
    client_len2 = sizeof(client_addr2);
    client_socket2 = accept(server_socket, (struct sockaddr*)&client_addr2, &client_len2);
    if (client_socket2 == -1) {
        perror("Ошибка принятия соединения от клиента №2");
        exit(EXIT_FAILURE);
    }
    printf("Клиент №2 подключен.\n");

    // Настройка аргументов для потоков
    args1.client_socket = client_socket1;
    args1.other_client_socket = client_socket2;
    args2.client_socket = client_socket2;
    args2.other_client_socket = client_socket1;

    // Создание потоков для обработки соединений с клиентами
    if (pthread_create(&tid1, NULL, handle_connection, &args1) != 0 ||
        pthread_create(&tid2, NULL, handle_connection, &args2) != 0) {
        perror("Ошибка создания потока");
        close(client_socket1);
        close(client_socket2);
        exit(EXIT_FAILURE);
    }

    // Ожидание завершения работы потоков
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    // Закрытие сокетов сервера (этот код никогда не будет достигнут)
    close(server_socket);

    return 0;
}
