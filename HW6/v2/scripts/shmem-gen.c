// shmem-gen.c
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>
#include <error.h>
#include <signal.h>

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

int main(){
  int num;

  signal(SIGINT, cleanup);

  srand(time(NULL));
  shm_id = shmget (0x2FF, getpagesize(), 0666 | IPC_CREAT);
  printf("shm_id = %d\n", shm_id);
  if(shm_id < 0){
    perror("shmget()");
    exit(1);
  }

  share = (SharedData *)shmat(shm_id, 0, 0);
  if(share == NULL){
    perror("shmat()");
    exit(2);
  }
  printf("share = %p\n", share);

  while(1){
    num = random() % 1000;
    share->num = num;
    printf("write a random number %d\n", num);
    sleep(1);

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