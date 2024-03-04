// shmem-out.c
#include<stdio.h>
#include<unistd.h>
#include<sys/shm.h>
#include<stdlib.h>
#include<error.h>
#include<signal.h>

// Структура для хранения двух переменных в сегменте памяти
typedef struct {
    int num;
    int flag;
} SharedData;

int shm_id;
SharedData *share;

void cleanup(int signum) {
    if (signum == SIGINT) {
        share->flag = 1; // Если поступил сигнал завершения, устанавливаем флаг равный 1
        sleep(1);
        shmdt(share);
        shmctl(shm_id, IPC_RMID, NULL);
        exit(0);
    }
}

int main() {
  signal(SIGINT, cleanup);

  shm_id = shmget (0x2FF, getpagesize(), 0666 | IPC_CREAT);
  if(shm_id == -1){
    perror("shmget()");
    exit(1);
  }

  share = (SharedData *)shmat(shm_id, 0, 0);
  if(share == NULL){
    perror("shmat()");
    exit(2);
  }

  while(1){
    sleep(1);
    printf("%d\n", share->num);

    // Если флаг завершения стал равен 1, 
    // то мы отсоединяемся и завершаем работу
    if (share->flag == 1) {
        shmdt(share);
        shmctl(shm_id, IPC_RMID, NULL);
        exit(0);
    }
  }

  return 0;
}