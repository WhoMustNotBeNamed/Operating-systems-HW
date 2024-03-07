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

int shm_id;
char gen_object[] = "gen-memory"; //  имя объекта
int* addr;

// Метод завершения, который вызывается после получения сигнала о завершении
void cleanup(int signum) {
    if (signum == SIGINT ) {
        *addr = 1111; //Устанавливаем значение, которое будет являться сигналом для завершения
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
    addr = mmap(0, sizeof(int), PROT_WRITE|PROT_READ, MAP_SHARED, shm_id, 0);
    if (addr == (int*)-1 ) {
        printf("Error getting pointer to shared memory\n");
        return 1;
    }

    while(1){
        sleep(1);
        printf("%d\n", *addr);

        // Если флаг равен 1111, завершаем программу
        if (*addr == 1111) {
            if(shm_id != -1) {
                shm_unlink(gen_object);
                close(shm_id);
            }
            exit(0);
        }
    }

    return 0;
}
