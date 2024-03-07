// out-shmem.c
// read from the shm every 1 second
#include<stdio.h>
#include<unistd.h>
#include<sys/shm.h>
#include<stdlib.h>
#include<error.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

// Структура для хранения двух переменных в сегменте памяти
typedef struct {
    int num;
    int flag;
} Data;

int shm_id;
char gen_object[] = "gen-memory"; //  имя объекта
Data data;
Data* addr;

// Метод завершения, который вызывается после получения сигнала о завершении
void cleanup(int signum) {
    if (signum == SIGINT ) {
        data.flag = 1; // Меняем флаг на равный 1, чтобы передать сигнал о завершении 
        *addr = data;
    }
    
    if(shm_id != -1) {
        shm_unlink(gen_object);
        close(shm_id);
    }
    exit(0);
}

int main() {
    signal(SIGINT, cleanup); // Обработка сигнала завершения

    //открыть объект
    if ( (shm_id = shm_open(gen_object, O_RDWR, 0666)) == -1 ) {
        printf("Opening error\n");
        perror("shm_open");
        return 1;
    } else {
        printf("Object is open: name = %s, id = 0x%x\n", gen_object, shm_id);
    }

    //получить доступ к памяти
    addr = mmap(0, sizeof(Data), PROT_WRITE|PROT_READ, MAP_SHARED, shm_id, 0);
    if (addr == MAP_FAILED) {
        printf("Error getting pointer to shared memory\n");
        return 1;
    }

    while(1){
        sleep(1);
        printf("%d\n", addr->num);

        // Если флаг равен 1, завершаем программу
        if (addr->flag == 1) {
            if(shm_id != -1) {
                shm_unlink(gen_object);
                close(shm_id);
            }
            exit(0);
        }
    }

    return 0;
}
