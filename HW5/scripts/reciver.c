#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int sender_pid;  // PID передатчика
int received_num = 0;  // Полученное число
int current_bit = 0;  // Текущий полученный бит

// Обработчик сигнала SIGUSR1
void sigusr1_handler(int sig) {
    // Записываем полученный бит в число
    received_num |= (0 << current_bit);
    current_bit++;
    // Посылаем сигнал передатчику для подтверждения получения
    kill(sender_pid, SIGUSR1);
}

// Обработчик сигнала SIGUSR2
void sigusr2_handler(int sig) {
    // Записываем полученный бит в число
    received_num |= (1 << current_bit);
    current_bit++;
    // Посылаем сигнал передатчику для подтверждения получения
    kill(sender_pid, SIGUSR1);
}

int main() {
    printf("Receiver PID: %d\n", getpid());  // Выводим PID приемника
    printf("Input sender PID: ");
    scanf("%d", &sender_pid);  // Получаем PID передатчика от пользователя

    // Устанавливаем обработчики для SIGUSR1 и SIGUSR2
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);

    printf("Waiting for sender...\n");

    // Ожидаем получение всех битов
    while (current_bit < 32) {
        pause();
    }

    // Выводим полученное число в двоичном виде
    for (int i = 31; i >= 0; i--) {
        printf("%d", (received_num >> i) & 1);
    }
    printf("\n");

    printf("Result number: %d\n", received_num);  // Выводим полученное число

    return 0;
}
