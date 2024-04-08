// writer.c - писатель пишущий в следующую по порядку ячейку
#include "common.h"
#include <semaphore.h>
#include <signal.h>

// Семафор для отсекания лишних писаталей
// То есть, для формирования только одного процесса-писателя
// Закрывает критическую секцию для анализа наличия писателей
const char *writer_sem_name = "/writer-semaphore";
sem_t *writer;   // указатель на семафор пропуска писателей
// Семафор, играющий роль флага для допуска писателей
const char *first_writer_sem_name = "/first_writer-semaphore";
sem_t *first_writer;   // указатель на семафор допуска

// Функция, осуществляющая обработку сигнала прерывания работы
// Осществляет удаление всех семафоров и памяти. Заодно "убивает" читателя
// независимо от его текущего состояния
void sigfunc(int sig) {
  if(sig != SIGINT && sig != SIGTERM) {
    return;
  }
  if(sig == SIGINT) {
    kill(buffer->reader_pid, SIGTERM);
    printf("Writer(SIGINT) ---> Reader(SIGTERM)\n");
  } else if(sig == SIGTERM) {
    printf("Writer(SIGTERM) <--- Reader(SIGINT)\n");
  }
  // Закрывает видимые семафоры и файлы, доступные процессу
  if(sem_close(writer) == -1) {
    perror("sem_close: Incorrect close of writer semaphore");
    exit(-1);
  };
  if(sem_close(first_writer) == -1) {
    perror("sem_close: Incorrect close of first_writer semaphore");
    exit(-1);
  };
  close_common_semaphores();

  // Удаляет все свои семафоры и разделяемую память
  if(sem_unlink(writer_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of writer semaphore");
    // exit(-1);
  };
  if(sem_unlink(first_writer_sem_name) == -1) {
    perror("sem_unlink: Incorrect unlink of first_writer semaphore");
    // exit(-1);
  };
  unlink_all();
  printf("Writer: bye!!!\n");
  exit (10);
}

int main() {
  // Задание обработчика, завершающего работу писателя и обеспечивающего
  // уничтожение контекста и процессов
  signal(SIGINT, sigfunc);
  signal(SIGTERM,sigfunc);

  srand(time(0)); // Инициализация генератора случайных чисел
  init();         // Начальная инициализация общих семафоров

  // Формирование объекта разделяемой памяти для общего доступа к кольцевому буферу
 if ( (buf_id = shm_open(shar_object, O_CREAT|O_RDWR, 0666)) == -1 ) {
    perror("shm_open");
    exit(-1);
  } else {
    printf("Object is open: name = %s, id = 0x%x\n", shar_object, buf_id);
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
    perror("writer: mmap");
    exit(-1);
  }
  printf("mmap checkout\n");

  // Разборки писателей. Семафор для конкуренции за работу
  if((writer = sem_open(writer_sem_name, O_CREAT, 0666, 1)) == 0) {
    perror("sem_open: Can not create writer semaphore");
    exit(-1);
  };
  // Дополнительный семафор, играющий роль счетчика-ограничителя
  if((first_writer = sem_open(first_writer_sem_name, O_CREAT, 0666, 1)) == 0) {
    perror("sem_open: Can not create first_writer semaphore");
    exit(-1);
  };
  // Первый просочившийся запрещает доступ остальным за счет установки флага
  if(sem_wait(writer) == -1) {
    perror("sem_wait: Incorrect wait of writer semaphore");
    exit(-1);
  };
  // Он проверяет значение семафора, к которому другие не имеют доступ
  // и устанавливает его в 0, если является первым писателем.
  // Остальные писатели при проверке завершают работу
  int writer_number = 0;
  sem_getvalue(first_writer, &writer_number);
  printf("checking: writer_number = %d\n", writer_number);
  // Остальные завершают работу по единичному флагу.
  if(writer_number == 0) {
    // Завершение процесса
    printf("Writer %d: I have lost this work :(\n", getpid());
    // Безработных писателей может быть много. Нужно их тоже отфутболить.
    // Для этого они должны войти в эту секцию
    if(sem_post(writer) == -1) {
      perror("sem_post: Consumer can not increment writer semaphore");
      exit(-1);
    }
    exit(13);
  }
  // buffer->have_writer = 1;  // подъем флага, запрещающего доступ другим
  // Первый просочившийся запрещает доступ остальным
  // за счет блокирования этого семафора. Они до этого кода не должны дойти
  if(sem_wait(first_writer) == -1) {
    perror("sem_wait: Incorrect wait of first_writer semaphore");
    exit(-1);
  };
  // Пропуск других писателей для определения возможности поработать
  if(sem_post(writer) == -1) {
    perror("sem_post: Consumer can not increment writer semaphore");
    exit(-1);
  }
  // сохранение pid для корректного взаимодействия с писателем
  buffer->writer_pid = getpid();
  printf("Writer %d: I am first for this work! :)\n", getpid());

  // Инициализация буфера отрицательными числам, имитирующими пустые ячейки
  // Перед доступом читателя, который обрабатывает натуральные числа в ограниченном диапазоне
  for(int i = 0; i < BUF_SIZE; ++i) {
    buffer->store[i] = -1;
  }
  buffer->have_reader = 0;
  // buffer->have_writer = 0;

  // Подъем администратора, разрешающего продолжение процессов читателя
  // После того, как сформировался объект в памяти и прошла его инициализация.
  // В начале идет проверка на запуск хотя бы отдного писателя, то есть,
  // на то, что семафор уже поднят, и писатели поступают.
  // Это позволяет новым писателям не увеличивать значение семафора.
  int is_writers = 0;
  sem_getvalue(admin, &is_writers);
  if(is_writers == 0) {
    // Срабатывает при запуске первого писателя
    if(sem_post(admin) == -1) {
      perror("sem_post: Can not increment admin semaphore");
      exit(-1);
    }
  }
  // printf("before while checkout\n");

  // Алгоритм писателя
  while (1) {
    // Проверка заполнения буфера (ждать если полон)
    if(sem_wait(empty) == -1) { //защита операции записи
      perror("sem_wait: Incorrect wait of empty semaphore");
      exit(-1);
    };
    //критическая секция, конкуренция с читателем
    if(sem_wait(mutex) == -1) {
      perror("sem_wait: Incorrect wait of mutex");
      exit(-1);
    };
    // Поиск первой свободной ячейки для записи. Она должна быть. Иначе сюда не попасть
    int i = 0;
    while(buffer->store[i] != -1) {
            printf("    --> checking store: store[%d] = %d\n", i, buffer->store[i]);
            ++i;
    }
    // Запись в первую свободную ячейку
    buffer->store[i] = rand() % 11; // число от 0 до 10
    //количество занятых ячеек увеличилось на единицу
    if(sem_post(full) == -1) {
      perror("sem_post: Incorrect post of full semaphore");
      exit(-1);
    };
    // Вывод информации об операции записи
    pid_t pid = getpid();
    printf("Producer %d writes value = %d to cell [%d]\n",
           pid, buffer->store[i], i);
           // pid, data, buffer->tail - 1 < 0 ? BUF_SIZE - 1: buffer->tail - 1 ;
    // Выход из критической секции
    if(sem_post(mutex) == -1) {
      perror("sem_post: Incorrect post of mutex semaphore");
      exit(-1);
    };
    sleep(rand() % 3 + 1);
  }
  return 0;
}

