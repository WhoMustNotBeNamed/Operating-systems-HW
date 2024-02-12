#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const int BUFFER_SIZE =  32;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(-1);
    }

    int input_fd, output_fd;
    ssize_t read_bytes, written_bytes;
    char buffer[BUFFER_SIZE];
  

    // Открытие входного файла для чтения
    if ((input_fd = open(argv[1], O_RDONLY)) < 0) {
        printf("Failed to open input file");
        exit(-1);
    }

    // Открытие выходного файла для записи
    if ((output_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
        printf("Failed to open output file");
        close(input_fd);
        exit(-1);
    }

    // Чтение из входного файла и запись в выходной файл
    while ((read_bytes = read(input_fd, buffer, BUFFER_SIZE)) > 0) {
        written_bytes = write(output_fd, buffer, read_bytes);
        if (written_bytes != read_bytes) {
            printf("Failed to write to output file");
            close(input_fd);
            close(output_fd);
            exit(-1);
        }
    }

    // Проверка на ошибку чтения
    if (read_bytes < 0) {
        printf("Failed to read from input file");
        close(input_fd);
        close(output_fd);
        exit(-1);
    }

    // Закрытие файлов
    if (close(input_fd) < 0) {
        printf("Failed to close input file");
        exit(-1);
    }

    if (close(output_fd) < 0) {
        printf("Failed to close output file");
        exit(-1);
    }

    printf("File copied successfully.\n");
    return 0;
}
