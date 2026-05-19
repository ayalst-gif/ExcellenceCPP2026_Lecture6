#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace fs = std::filesystem;

// Function to print detailed information about a specific file descriptor
void printFdInfo(const fs::path& fdPath) {
    std::string fdStr = fdPath.filename().string();
    int fd;

    try {
        fd = std::stoi(fdStr);
    }
    catch (const std::invalid_argument&) {
        return; // Skip if it's not a valid integer
    }

    std::cout << "File Descriptor: [" << fd << "]\n";

    // 1. Resolve the symlink to see what resource the FD points to
    std::error_code ec;
    fs::path targetPath = fs::read_symlink(fdPath, ec);
    std::string target = ec ? "Unknown (or permission denied)" : targetPath.string();
    std::cout << "  -> Target Resource: " << target << "\n";

    // 2. Fetch underlying file statistics using fstat()
    struct stat statbuf;
    if (fstat(fd, &statbuf) == 0) {
        std::cout << "  -> Inode Number:    " << statbuf.st_ino << "\n";
        std::cout << "  -> File Size:       " << statbuf.st_size << " bytes\n";
        std::cout << "  -> Permissions:     0" << std::oct << (statbuf.st_mode & 0777) << std::dec << " (octal)\n";
    }
    else {
        std::cout << "  -> [fstat failed to retrieve stats]\n";
    }

    // 3. Read Linux-specific metadata from /proc/self/fdinfo/
    // This provides file position (pos), access flags (flags), and mount ID (mnt_id)
    fs::path fdInfoPath = fs::path("/proc/self/fdinfo") / fdStr;
    std::ifstream fdInfoFile(fdInfoPath);
    if (fdInfoFile.is_open()) {
        std::string line;
        while (std::getline(fdInfoFile, line)) {
            std::cout << "  -> fdinfo metadata: " << line << "\n";
        }
    }
    std::cout << "--------------------------------------------------\n";
}

int main() {
    std::cout << "=== Linux File Descriptor Inspector ===\n\n";

    // Step A: Create some dummy file descriptors to make the output interesting

    // 1. A standard file
    std::ofstream testFile("lecturer_demo.txt");
    testFile << "Hello, Students!";

    // 2. A pipe (creates two FDs: one for reading, one for writing)
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        std::cerr << "Failed to create pipe.\n";
        return 1;
    }

    // Step B: Iterate through the /proc/self/fd directory
    // This directory contains symlinks named after the open FD numbers
    std::string fdDirectory = "/proc/self/fd";

    try {
        for (const auto& entry : fs::directory_iterator(fdDirectory)) {
            printFdInfo(entry.path());
        }
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << '\n';
    }

    // Step C: Cleanup
    testFile.close();
    close(pipefd[0]);
    close(pipefd[1]);
    std::remove("lecturer_demo.txt"); // Delete the temporary file

    return 0;
}
