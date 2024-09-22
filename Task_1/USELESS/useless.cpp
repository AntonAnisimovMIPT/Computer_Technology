#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <vector>
#include <algorithm>

#include <chrono>
#include <thread>
#include <stdexcept>

#include <unistd.h>
#include <sys/wait.h>

struct Task {
    unsigned int waiting_time;
    std::string programm;
};

auto parseFile(const std::string& input_file) {

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
        if (task.programm.empty()) {
            throw std::invalid_argument("Error: Empty command in task.");
        }
        tasks.push_back(task);
    }

    if (tasks.empty()) {
        throw std::runtime_error("Error: No valid tasks found in file: " + input_file);
    }

    return tasks;
}

auto execute(const Task& task) {

    auto pid = fork();
    if (pid == 0) {

        std::istringstream iss(task.programm);
        std::vector<std::string> args;
        std::string arg;

        while (iss >> arg) {
            args.push_back(arg);
        }

        std::vector<char*> c_args;
        for (auto& arg : args) {
            c_args.push_back(&arg[0]);
        }
        c_args.push_back(nullptr);

        if (execvp(c_args[0], c_args.data()) == -1) {
            throw std::system_error(errno, std::generic_category(), "Failed to execute command: " + task.programm);
        }
    } else if (pid < 0) {
        throw std::system_error(errno, std::generic_category(), "Fork failed for command: " + task.programm);

    } else {
        wait(nullptr);
    }
}

auto runTasksWithSchedule(std::vector<Task>& tasks) {

    std::sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
        return a.waiting_time < b.waiting_time;
    });

    auto startTime = std::chrono::steady_clock::now();
    auto lastTime = 0;

    for (const auto& task : tasks) {
        auto delay = task.waiting_time - lastTime;
        lastTime = task.waiting_time;

        std::this_thread::sleep_for(std::chrono::seconds(delay));

        try {
            execute(task);
        } catch (const std::system_error& e) {
            std::cerr << "Execution error: " << e.what() << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            throw std::invalid_argument("Usage: " + std::string(argv[0]) + " <tasks_file>");
        }

        auto filename = argv[1];
        auto tasks = parseFile(filename);

        runTasksWithSchedule(tasks);

    } catch (const std::ios_base::failure& e) {
        std::cerr << "File error: " << e.what() << std::endl;
        return 1;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Argument error: " << e.what() << std::endl;
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
