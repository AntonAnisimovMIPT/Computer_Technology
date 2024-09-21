#include <fstream>
#include <sstream>
#include <stdexcept>
#include <chrono>
#include <string>
#include <vector>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>

struct Task {
    unsigned int waiting_time;
    std::string programm;
};

auto fileParser(const std::string& input_file) {

    std::vector<Task> tasks;

    std::ifstream file(input_file);
    std::string line;

    if (!file.is_open()) {
        throw std::ios_base::failure("Error: Unable to open file: " + input_file);
    }

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        Task task;
        iss >> task.waiting_time;
        std::getline(iss >> std::ws, task.programm);
        tasks.push_back(task);
    }

    return tasks;
}

void execute(const Task& task, std::chrono::time_point<std::chrono::steady_clock> startTime) {
    auto taskStartTime = startTime + std::chrono::seconds(task.waiting_time);

    // Ожидание до момента запуска задачи
    std::this_thread::sleep_until(taskStartTime);

    // Создаем дочерний процесс
    pid_t pid = fork();
    if (pid == 0) {
        // В дочернем процессе
        std::istringstream iss(task.programm);
        std::vector<std::string> args;
        std::string arg;

        // Разделяем команду на аргументы
        while (iss >> arg) {
            args.push_back(arg);
        }

        // Преобразуем в массив указателей для execvp
        std::vector<char*> c_args;
        for (auto& arg : args) {
            c_args.push_back(&arg[0]);
        }
        c_args.push_back(nullptr);

        // Выполняем команду
        if (execvp(c_args[0], c_args.data()) == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to execute command: " + task.programm);
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        throw std::system_error(errno, std::generic_category(), "Fork failed for command: " + task.programm);
    } else {
        // Родительский процесс может не ждать завершения дочернего процесса
    }
}

auto scheduleTasks(const std::vector<Task>& tasks) {
    auto startTime = std::chrono::steady_clock::now();

    for (const auto& task : tasks) {

        std::thread([&task, startTime]() {
            execute(task, startTime);
        }).detach();
    }
}

int main() {

    auto tasks = fileParser("./commands");

    scheduleTasks(tasks);

    std::this_thread::sleep_for(std::chrono::seconds(15));
}
