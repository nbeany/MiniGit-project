#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

struct FileNode {
    std::string fileName;
    int versionCount;  // track current version number
};

std::vector<FileNode> stagedFiles;

bool directoryExists(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) return false;
    return (info.st_mode & S_IFDIR) != 0;
}

void initializeMiniGit() {
    if (!directoryExists(".minigit")) {
        system("mkdir .minigit");
        std::cout << "Created .minigit folder\n";
    } else {
        std::cout << ".minigit folder already exists\n";
    }
}

void createObjectsFolder() {
    if (!directoryExists(".minigit/objects")) {
#ifdef _WIN32
        system("mkdir .minigit\\objects");
#else
        system("mkdir -p .minigit/objects");
#endif
        std::cout << "Created .minigit/objects folder\n";
    } else {
        std::cout << ".minigit/objects folder already exists\n";
    }
}

void addFile() {
    std::string name;
    std::cout << "Enter filename to track (type 'done' to finish):\n";
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, name);
        if (name == "done") break;

        bool alreadyAdded = false;
        for (const auto& f : stagedFiles) {
            if (f.fileName == name) {
                std::cout << "File already added.\n";
                alreadyAdded = true;
                break;
            }
        }
        if (!alreadyAdded) {
            stagedFiles.push_back({name, 0});
            std::cout << "Added: " << name << "\n";
        }
    }
}

bool copyFile(const std::string& src, const std::string& dest) {
    std::ifstream in(src.c_str(), std::ios::binary);
    if (!in) {
        std::cerr << "Error: Cannot open source file: " << src << "\n";
        return false;
    }
    std::ofstream out(dest.c_str(), std::ios::binary);
    if (!out) {
        std::cerr << "Error: Cannot create destination file: " << dest << "\n";
        return false;
    }
    out << in.rdbuf();
    return true;
}

void commit() {
    createObjectsFolder();

    for (auto& f : stagedFiles) {
        std::string versionedName = f.fileName + "_" + std::to_string(f.versionCount);
        std::string dest = ".minigit/objects/" + versionedName;

        if (copyFile(f.fileName, dest)) {
            std::cout << "Committed " << f.fileName << " as " << versionedName << "\n";
            f.versionCount++;  // increment version number after commit
        } else {
            std::cout << "Failed to commit " << f.fileName << "\n";
        }
    }
}

int main() {
    initializeMiniGit();

    addFile();

    std::cout << "\nFiles staged for tracking:\n";
    for (const auto& f : stagedFiles) {
        std::cout << "- " << f.fileName << " (current version count: " << f.versionCount << ")\n";
    }

    char answer;
    do {
        std::cout << "\nCommiting files...\n";
        commit();

        std::cout << "\nCommit again? (y/n): ";
        std::cin >> answer;
        std::cin.ignore();  // clear newline from input buffer
    } while (answer == 'y' || answer == 'Y');

    std::cout << "Exiting MiniGit.\n";
    return 0;
}
