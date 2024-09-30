#include <iostream>
#include <fstream>

#include <filesystem>
#include <cstdlib>

#include <chrono>
#include <string>

namespace fs = std::filesystem;

auto isBackupFile(const fs::path& master_file, const fs::path& wingman_file) {

    auto master_filename = master_file.filename().string();
    auto wingman_filename = wingman_file.filename().string();

    if (wingman_filename == master_filename + ".gz") {
        return true;
    }

    return false;
}

auto isChanged(const fs::path& master_file, const fs::path& wingman_file) {

    if (!fs::exists(wingman_file)) {
        return true;
    }

    if (!isBackupFile(master_file, wingman_file)) {
        return true;
    }

    auto master_file_time = fs::last_write_time(master_file);
    auto wingman_file_time = fs::last_write_time(wingman_file);

    return master_file_time > wingman_file_time;
}

auto processFile(const fs::path& master_file, const fs::path& wingman_file, std::ofstream& log) {

    try {
        fs::copy_file(master_file, wingman_file, fs::copy_options::overwrite_existing);

        if (master_file.extension() != ".gz") {

            auto compress_command = "gzip -f \"" + wingman_file.string() + "\"";
            auto result = system(compress_command.c_str());

            if (result != 0) {
                log << "Compressing error: " << wingman_file.string() << "\n";
                throw std::runtime_error("Compressing error: " + wingman_file.string());
            }

            log << "File " << wingman_file << " successfully copied and compressed.\n";
        } else {

            log << "File " << wingman_file << " is already compressed, no further compression needed.\n";
        }
    } catch (const std::exception& e) {

        log << "File processing error: " << e.what() << '\n';
        std::cerr << "File processing error: " << e.what() << '\n';
    }
}

auto doBackup(const fs::path& src_dir, const fs::path& dest_dir, const std::string& log_file) {

    std::ofstream log(log_file, std::ios::app);
    if (!log.is_open()) {
        throw std::ios_base::failure("Error: Unable to open log file: " + log_file);
    }

    auto any_changes = false;
    log << "--- Backup operation started ---\n";

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
            fs::path wingman_file = dest_dir / (relative_path.string());
            auto comp_wingman_file = wingman_file.string() + ".gz";

            if (isChanged(entry.path(), comp_wingman_file)) {
                fs::create_directories(wingman_file.parent_path());
                processFile(entry.path(), wingman_file, log);
                any_changes = true;
            }
        }
    }

    if (!any_changes) {
        log << "No files were changed. Backup completed with no updates.\n";
    }

    log << "--- Backup operation finished ---\n";
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

        doBackup(src_dir, dest_dir, log_file);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}