// shmem-gen.c
#include <stdio.h>
#include <unistd.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <time.h>
#include <error.h>
#include <signal.h>

int shm_id;
int *share;


// Метод завершения, который вызывается после получения сигнала о завершении
void cleanup(int signum) {
  *share = 1111; //Устанавоиваем значение, которое будет являться сигналом для завершения
  sleep(1);
  shmdt(share);
  shmctl(shm_id, IPC_RMID, NULL);
  exit(0);
}

int main(){
  int num;

  signal(SIGINT, cleanup); // Обработка сигнала

  srand(time(NULL));
  shm_id = shmget (0x2FF, getpagesize(), 0666 | IPC_CREAT);
  printf("shm_id = %d\n", shm_id);
  if(shm_id < 0){
    perror("shmget()");
    exit(1);
  }

  share = (int *)shmat(shm_id, 0, 0);
  if(share == NULL){
    perror("shmat()");
    exit(2);
  }
  printf("share = %p\n", share);

  while(1){
    num = random() % 1000;
    *share = num;
    printf("write a random number %d\n", num);
    sleep(1);

    // Если переменная равно 1111, завершаем работу
    if (*share == 1111) {
      shmdt(share);
      shmctl(shm_id, IPC_RMID, NULL);
      exit(0);
    }
  }
  return 0;
}