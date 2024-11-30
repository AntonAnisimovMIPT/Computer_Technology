#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <V>\n", argv[0]);
        return 1;
    }

    int V = atoi(argv[1]);
    if (V <= 0) {
        fprintf(stderr, "V must be a positive integer.\n");
        return 1;
    }

    int running_processes = 0;
    char command[MAX_COMMAND_LENGTH];

    while (fgets(command, MAX_COMMAND_LENGTH, stdin) != NULL) {
        // Удаляем символ новой строки, если он есть
        command[strcspn(command, "\n")] = '\0';

        if (running_processes >= V) {
            fprintf(stderr, "Error: Maximum number of running processes (%d) reached.\n", V);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            // Дочерний процесс
            execl("/bin/sh", "sh", "-c", command, (char *)NULL);
            perror("execl");
            exit(1);
        } else {
            // Родительский процесс
            running_processes++;
        }
    }

    // Ожидаем завершения всех запущенных процессов
    while (running_processes > 0) {
        wait(NULL);
        running_processes--;
    }

    return 0;
}