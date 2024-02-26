#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int receiver_pid;  // PID приемника
int current_bit = 0;  // Текущий передаваемый бит
int num_to_send;  // Число для передачи

// Функция для отправки одного бита по сигналу
void send_bit(int bit) {
    if (bit)
        kill(receiver_pid, SIGUSR2);  // Отправить SIGUSR2, если бит равен 1
    else
        kill(receiver_pid, SIGUSR1);  // Отправить SIGUSR1, если бит равен 0
}

// Обработчик сигнала SIGUSR1
void handler(int sig) {
    if (sig == SIGUSR1) {
        current_bit++;
        if (current_bit < 32) {
            // Передача следующего бита, если еще не все переданы
            send_bit((num_to_send >> current_bit) & 1);
        } else {
            // Все биты переданы, вывод результата и завершение программы
            for (int i = 31; i >= 0; i--) {
                printf("%d", (num_to_send >> i) & 1);
            }
            printf("\n");
            printf("Result number: %d\n", num_to_send);
            exit(0);
        }
    }
}

int main() {
    printf("Sender PID: %d\n", getpid());  // Вывести PID отправителя
    printf("Input receiver PID: ");
    scanf("%d", &receiver_pid);  // Получить PID приемника от пользователя

    printf("Enter an integer to send: ");
    scanf("%d", &num_to_send);  // Получить число для передачи от пользователя

    signal(SIGUSR1, handler);  // Установить обработчик для SIGUSR1

    // Начать передачу, отправив первый бит
    send_bit(num_to_send & 1);

    // Ожидание сигналов
    while (1) {
        pause();
    }

    return 0;
}
