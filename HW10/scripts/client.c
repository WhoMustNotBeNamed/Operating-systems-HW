#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_MSG_SIZE 1024

// ANSI Escape sequences для смены цвета текста в консоли
#define BLUE_COLOR "\033[1;34m"
#define RESET_COLOR "\033[0m"

// Функция для приема сообщений от сервера и вывода их в консоль
void *receive_handler(void *socket_desc) {
    int client_socket = *((int*)socket_desc);
    char buffer[MAX_MSG_SIZE];
    int bytes_received;

    while (1) {
        bytes_received = recv(client_socket, buffer, MAX_MSG_SIZE, 0);
        if (bytes_received == -1) {
            perror("Ошибка приема сообщения");
            break;
        } else if (bytes_received == 0) {
            printf("Соединение закрыто сервером.\n");
            break;
        } else {
            buffer[bytes_received] = '\0';
            printf("\n%sПолучено от сервера: %s%s\n", BLUE_COLOR, buffer, RESET_COLOR);
            printf("Введите сообщение (для завершения введите 'The End'): ");
            fflush(stdout); 
        }
    }

    return NULL;
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[MAX_MSG_SIZE];
    pthread_t receive_thread; // Переменная для идентификатора потока
    char server_ip[16]; // Массив для хранения IP-адреса сервера
    int server_port; // Переменная для хранения порта сервера

    // Запрос IP-адреса и порта сервера у пользователя
    printf("Введите IP-адрес сервера: ");
    scanf("%15s", server_ip);
    printf("Введите порт сервера: ");
    scanf("%d", &server_port);

    // Очистка входного буфера
    while (getchar() != '\n');

    // Создание сокета
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Ошибка создания сокета");
        exit(EXIT_FAILURE);
    }

    // Заполнение структуры для адреса сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    // Подключение к серверу
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка подключения");
        exit(EXIT_FAILURE);
    }

    printf("Подключение к серверу успешно.\n");

    // Создание отдельного потока для приема сообщений от сервера и вывода их в консоль
    if (pthread_create(&receive_thread, NULL, receive_handler, (void*)&client_socket) != 0) {
        perror("Ошибка создания потока для приема сообщений");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Отправка сообщений серверу
    while (1) {
        printf("Введите сообщение (для завершения введите 'The End'): ");
        fgets(buffer, MAX_MSG_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Удаление символа новой строки
        send(client_socket, buffer, strlen(buffer), 0);
        if (strcmp(buffer, "The End") == 0) {
            printf("Завершение работы...\n");
            break;
        }
    }

    // Ожидание завершения потока приема сообщений
    pthread_join(receive_thread, NULL);

    // Закрытие сокета
    close(client_socket);

    return 0;
}

