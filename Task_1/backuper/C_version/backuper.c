#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

void log_message(FILE* log, const char* message) {
    fprintf(log, "%s\n", message);
    fflush(log);
}

int is_file_changed(const char* src_file, const char* dest_file) {
    struct stat src_stat, dest_stat;

    if (stat(dest_file, &dest_stat) != 0) {
        return 1;
    }

    if (stat(src_file, &src_stat) != 0) {
        perror("Error reading the source file");
        return 0;
    }

    return src_stat.st_mtime > dest_stat.st_mtime;
}

int compress_file(const char* file_path, FILE* log) {
    char command[512];
    snprintf(command, sizeof(command), "gzip -f \"%s\"", file_path);

    int result = system(command);
    if (result != 0) {
        fprintf(log, "File compression error: %s\n", file_path);
        return 0;
    }

    return 1;
}

int copy_file(const char* src_file, const char* dest_file, FILE* log) {
    int src_fd = open(src_file, O_RDONLY);
    if (src_fd < 0) {
        fprintf(log, "Error opening the source file: %s\n", src_file);
        perror("File opening error");
        return 0;
    }

    int dest_fd = open(dest_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dest_fd < 0) {
        fprintf(log, "Error opening the destination file: %s\n", dest_file);
        perror("File opening error");
        close(src_fd);
        return 0;
    }

    char buffer[4096];
    ssize_t bytes_read, bytes_written;
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(log, "Error writing to the file: %s\n", dest_file);
            perror("Recording error");
            close(src_fd);
            close(dest_fd);
            return 0;
        }
    }

    if (bytes_read < 0) {
        fprintf(log, "File reading error: %s\n", src_file);
        perror("Reading error");
    }

    close(src_fd);
    close(dest_fd);

    log_message(log, "The file has been copied successfully.");
    return 1;
}

void process_directory(const char* src_dir, const char* dest_dir, FILE* log) {
    struct dirent* entry;
    DIR* dir = opendir(src_dir);

    if (dir == NULL) {
        fprintf(stderr, "Error opening the directory: %s\n", src_dir);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char src_path[512], dest_path[512];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, entry->d_name);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        struct stat stat_buf;
        if (stat(src_path, &stat_buf) == 0) {
            if (S_ISDIR(stat_buf.st_mode)) {
                mkdir(dest_path, 0755);
                process_directory(src_path, dest_path, log);
            } else if (S_ISREG(stat_buf.st_mode)) {
                char compressed_dest[1024];
                snprintf(compressed_dest, sizeof(compressed_dest), "%s.gz", dest_path);

                if (is_file_changed(src_path, compressed_dest)) {
                    if (copy_file(src_path, dest_path, log)) {
                        if (!compress_file(dest_path, log)) {
                            fprintf(stderr, "File compression error: %s\n", dest_path);
                        } else {
                            log_message(log, "The file has been successfully copied and compressed.");
                        }
                    }
                }
            }
        }
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source directory> <destination directory>\n", argv[0]);
        return 1;
    }

    const char* src_dir = argv[1];
    const char* dest_dir = argv[2];
    const char* log_file = "./log.txt";

    FILE* log = fopen(log_file, "w");
    if (!log) {
        perror("Error opening the log file");
        return 1;
    }

    struct stat src_stat;
    if (stat(src_dir, &src_stat) != 0 || !S_ISDIR(src_stat.st_mode)) {
        fprintf(stderr, "The source directory does not exist or is not a directory.\n");
        fclose(log);
        return 1;
    }

    struct stat dest_stat;
    if (stat(dest_dir, &dest_stat) != 0) {
        mkdir(dest_dir, 0755);
        log_message(log, "The destination directory has been created.");
    }

    log_message(log, "--- Start of the backup operation ---");

    process_directory(src_dir, dest_dir, log);

    log_message(log, "--- Completion of the backup operation ---");

    fclose(log);
    return 0;
}
