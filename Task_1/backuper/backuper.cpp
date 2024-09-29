#include <iostream>
#include <fstream>

#include <filesystem>
#include <cstdlib>

#include <chrono>
#include <string>

namespace fs = std::filesystem;

std::chrono::system_clock::time_point fileTimeToSystemTime(const fs::file_time_type& fileTime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        fileTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return sctp;
}

auto isChanged(const fs::path& cur_file, const fs::path& comp_file) {
    if (!fs::exists(comp_file)) {
        return true;
    }

    auto cur_file_time = fs::last_write_time(cur_file);
    auto comp_file_time = fs::last_write_time(comp_file);

    auto cur_file_time_point = fileTimeToSystemTime(cur_file_time);
    auto comp_file_time_point = fileTimeToSystemTime(comp_file_time);

    std::cout << "Current file: " << cur_file << " time: " << cur_file_time_point.time_since_epoch().count() << "\n";
    std::cout << "Comparative file: " << comp_file << " time: " << comp_file_time_point.time_since_epoch().count() << "\n";

    if (cur_file_time_point < comp_file_time_point) {
        std::cout << "ok\n";
        std::cout << cur_file << "\n"
                  << comp_file << "\n";
    }

    return cur_file_time_point < comp_file_time_point;
}

auto processFile(const fs::path& cur_file, const fs::path& comp_file, const std::string& log_file) {
    std::ofstream log(log_file, std::ios::app);
    std::string line;

    if (!log.is_open()) {
        throw std::ios_base::failure("Error: Unable to open file: " + log_file);
    }

    try {
        fs::copy_file(cur_file, comp_file, fs::copy_options::overwrite_existing);

        auto compress_command = "gzip -f \"" + comp_file.string() + "\"";
        auto result = system(compress_command.c_str());
        if (result != 0) {
            log << "Compressing error: " << comp_file.string() << "\n";
            throw std::runtime_error("Compressing error: " + comp_file.string());
        }

        log << "File " << cur_file << " successful copied and compressed.\n";
    } catch (const std::exception& e) {
        log << "File processing error: " << e.what() << '\n';
        std::cerr << "File processing error: " << e.what() << '\n';
    }
}

auto backupDir(const fs::path& src_dir, const fs::path& dest_dir, const std::string& log_file) {
    if (!fs::exists(src_dir)) {
        std::cerr << src_dir << " directory doesn't exist.\n";
        return;
    }

    if (!fs::exists(dest_dir)) {
        std::cout << "Destination directory doesn't exists, creating destination directory.\n";
        fs::create_directories(dest_dir);
    }

    for (const auto& entry : fs::recursive_directory_iterator(src_dir)) {
        if (fs::is_regular_file(entry)) {
            fs::path relative_path = fs::relative(entry.path(), src_dir);
            fs::path comp_file = dest_dir / relative_path;

            if (isChanged(entry.path(), comp_file)) {
                fs::create_directories(comp_file.parent_path());
                processFile(entry.path(), comp_file, log_file);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <source_directory> <destination_directory>\n";
        return 1;
    }

    fs::path src_dir = argv[1];
    fs::path dest_dir = argv[2];

    std::string log_file = "./log.txt";

    if (fs::exists(log_file)) {
        fs::remove(log_file);
    }

    try {

        backupDir(src_dir, dest_dir, log_file);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
