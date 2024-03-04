// shmem-out.c
#include<stdio.h>
#include<unistd.h>
#include<sys/shm.h>
#include<stdlib.h>
#include<error.h>
#include<signal.h>

int shm_id;
int *share;

// Метод обработки сигнала завершения
void cleanup(int signum) {
  *share = 1111; //Устанавоиваем значение, которое будет являться сигналом для завершения
  sleep(1);
  shmdt(share);
  shmctl(shm_id, IPC_RMID, NULL);
  exit(0);
}

int main() {
  signal(SIGINT, cleanup); // Обработка сигнала завершения

  shm_id = shmget (0x2FF, getpagesize(), 0666 | IPC_CREAT);
  if(shm_id == -1){
    perror("shmget()");
    exit(1);
  }

  share = (int *)shmat(shm_id, 0, 0);
  if(share == NULL){
    perror("shmat()");
    exit(2);
  }

  while(1){
    sleep(1);
    printf("%d\n", *share);

    // Если переменная равно 1111, завершаем работу
    if (*share == 1111) {
      shmdt(share);
      shmctl(shm_id, IPC_RMID, NULL);
      exit(0);
    }
  }

  return 0;
}