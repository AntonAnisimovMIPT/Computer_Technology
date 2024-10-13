#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define MAX_TASKS 100
#define MAX_CMD_LEN 256

struct Task {
    unsigned int waiting_time;
    char programm[MAX_CMD_LEN];
};

void parseFile(const char* input_file, struct Task tasks[], int* task_count) {
    FILE* file = fopen(input_file, "r");
    if (file == NULL) {
        perror("Error: Unable to open file");
        exit(EXIT_FAILURE);
    }

    char line[MAX_CMD_LEN + 10];
    *task_count = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%u %[^\n]", &tasks[*task_count].waiting_time, tasks[*task_count].programm) != 2) {
            fprintf(stderr, "Error: Invalid task format\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
        (*task_count)++;
        if (*task_count >= MAX_TASKS) {
            fprintf(stderr, "Error: Too many tasks\n");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    if (*task_count == 0) {
        fprintf(stderr, "Error: No valid tasks found in file\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}

void execute(struct Task task) {
    pid_t pid = fork();
    if (pid == 0) {
        char* args[MAX_CMD_LEN / 2 + 1];
        char* token = strtok(task.programm, " ");
        int i = 0;
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (execvp(args[0], args) == -1) {
            perror("Failed to execute command");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else {
        wait(NULL);
    }
}

int compareTasks(const void* a, const void* b) {
    struct Task* taskA = (struct Task*)a;
    struct Task* taskB = (struct Task*)b;
    return taskA->waiting_time - taskB->waiting_time;
}

void runTasksWithSchedule(struct Task tasks[], int task_count) {
    qsort(tasks, task_count, sizeof(struct Task), compareTasks);

    time_t startTime = time(NULL);
    unsigned int lastTime = 0;

    for (int i = 0; i < task_count; i++) {
        unsigned int delay = tasks[i].waiting_time - lastTime;
        lastTime = tasks[i].waiting_time;

        sleep(delay);

        execute(tasks[i]);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <tasks_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct Task tasks[MAX_TASKS];
    int task_count;

    parseFile(argv[1], tasks, &task_count);
    runTasksWithSchedule(tasks, task_count);

    return 0;
}
