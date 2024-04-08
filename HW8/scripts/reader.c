// reader.c - читатель забирающий из первой по очереди занятой ячейки
#include "common.h"
#include <unistd.h>

// Семафор для отсекания лишних читателей
// То есть, для формирования только одного процесса-читателя
const char *reader_sem_name = "/reader-semaphore";
sem_t *reader;   // указатель на семафор пропуска читателей

void sigfunc(int sig) {
  if(sig != SIGINT && sig != SIGTERM) {
    return;
  }
  if(sig == SIGINT) {
    kill(buffer->writer_pid, SIGTERM);
    printf("Reader(SIGINT) ---> Writer(SIGTERM)\n");
  } else if(sig == SIGTERM) {
    printf("Reader(SIGTERM) <--- Writer(SIGINT)\n");
  }
  // Закрывает свой семафор
  if(sem_close(reader) == -1) {
    perror("sem_close: Incorrect close of reader semaphore");
    exit(-1);
  };
  // Удаляет свой семафор
  if(sem_unlink(reader_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of reader semaphore");
    // exit(-1);
  };
  printf("Reader: bye!!!\n");
  exit (10);
}

// Функция вычисления факториала
int factorial(int n) {
  int p = 1;
  for(int i = 1; i <= n; ++i) {
    p *= i;
  }
  return p;
}

int main() {
  signal(SIGINT,sigfunc);
  signal(SIGTERM,sigfunc);

  srand(time(0)); // Инициализация генератора случайных чисел
  init();         // Начальная инициализация общих семафоров
  // printf("init checkout\n");

  // Если читатель запустился раньше, он ждет разрешения от администратора,
  // Который находится в писателе
  if(sem_wait(admin) == -1) { // ожидание когда запустится писатель
    perror("sem_wait: Incorrect wait of admin semaphore");
    exit(-1);
  };
  printf("Consumer %d started\n", getpid());
  // После этого он вновь поднимает семафор, позволяющий запустить других читателей
  if(sem_post(admin) == -1) {
    perror("sem_post: Consumer can not increment admin semaphore");
    exit(-1);
  }

  // Читатель имеет доступ только к открытому объекту памяти,
  // но может не только читать, но и писать (забирать)
  if ( (buf_id = shm_open(shar_object, O_RDWR, 0666)) == -1 ) {
    perror("shm_open: Incorrect reader access");
    exit(-1);
  } else {
    printf("Memory object is opened: name = %s, id = 0x%x\n",
           shar_object, buf_id);
  }
  // Задание размера объекта памяти
  if (ftruncate(buf_id, sizeof(shared_memory)) == -1) {
    perror("ftruncate");
    exit(-1);
  } else {
    printf("Memory size set and = %lu\n", sizeof (shared_memory));
  }

  //получить доступ к памяти
  buffer = mmap(0, sizeof (shared_memory), PROT_WRITE|PROT_READ, MAP_SHARED, buf_id, 0);
  if (buffer == (shared_memory*)-1 ) {
    perror("reader: mmap");
    exit(-1);
  }

  // Разборки читателей. Семафор для конкуренции за работу
  if((reader = sem_open(reader_sem_name, O_CREAT, 0666, 1)) == 0) {
    perror("sem_open: Can not create reader semaphore");
    exit(-1);
  };
  // Первый просочившийся запрещает доступ остальным за счет установки флага
  if(sem_wait(reader) == -1) { // ожидание когда запустится писатель
    perror("sem_wait: Incorrect wait of reader semaphore");
    exit(-1);
  };
  // Он проверяет флаг и устанавливает его в 1, если первый
  // Остальные завершают работу по единичному флагу.
  if(buffer->have_reader) {
    // Завершение процесса
    printf("Reader %d: I have lost this work :(\n", getpid());
    // Безработных читателей может быть много. Нужно их тоже отфутболить.
    // Для этого они должны войти в эту секцию
    if(sem_post(reader) == -1) {
      perror("sem_post: Consumer can not increment reader semaphore");
      exit(-1);
    }
    exit(13);
  }
  buffer->have_reader = 1;  // подъем флага, запрещающего доступ другим
  // Пропуск  читателей других для определения возможности поработать
  if(sem_post(reader) == -1) {
    perror("sem_post: Consumer can not increment reader semaphore");
    exit(-1);
  }
  // сохранение pid для корректного взаимодействия с писателем
  buffer->reader_pid = getpid();

  // Алгоритм читателя
  while (1) {
    sleep(rand() % 3 +1);
    // Контроль наличия элементов в буфере
    if(sem_wait(full) == -1) {
      perror("sem_wait: Incorrect wait of full semaphore");
      exit(-1);
    };
    //критическая секция
    if(sem_wait(mutex) == -1) {
      perror("sem_wait: Incorrect wait of busy semaphore");
      exit(-1);
    };
    // Поиск первой занятой ячейки для записи. Она должна быть. Иначе сюда не попасть
    int i = 0;
    while(buffer->store[i] == -1) {
      printf("    --> checking store: store[%d] = %d\n", i, buffer->store[i]);
      ++i;
    }
    // Получение значения из читаемой ячейки
    int result = buffer->store[i];
    buffer->store[i] = -1;  // данные прочитаны
    // вычисление факториала
    int f = factorial(result);
    //количество свободных ячеек увеличилось на единицу
    if(sem_post(empty) == -1) {
      perror("sem_post: Incorrect post of free semaphore");
      exit(-1);
    };
    // Вывод информации об операции чтения
    pid_t pid = getpid();
    printf("Consumer %d: Reads value = %d from cell [%d], factorial = %d\n",
           pid, result, i, f) ;
    // Выход из критической секции
    if(sem_post(mutex) == -1) {
      perror("sem_post: Incorrect post of mutex semaphore");
      exit(-1);
    };
  }
}

