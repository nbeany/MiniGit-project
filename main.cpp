#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <sys/stat.h>

struct FileNode {
    std::string fileName;
    int versionCount;  // track current version number
};

struct Commit {
    std::string id;
    std::string message;
    std::string timestamp;
    std::string parentId;
    std::vector<std::pair<std::string, int>> files; // file name and version
};

std::vector<FileNode> stagedFiles;
std::string lastCommitId = "";  // track latest commit

// ---------------------- Utility Functions ----------------------

bool directoryExists(const char* path) {
    struct stat info;
    if (stat(path, &info) != 0) return false;
    return (info.st_mode & S_IFDIR) != 0;
}

void saveHEAD(const std::string& commitId) {
    std::ofstream out(".minigit/HEAD");
    if (out) {
        out << commitId;
    } else {
        std::cerr << "Error writing HEAD\n";
    }
}

std::string loadHEAD() {
    std::ifstream in(".minigit/HEAD");
    std::string id;
    if (in) {
        std::getline(in, id);
    }
    return id;
}

void initializeMiniGit() {
    if (!directoryExists(".minigit")) {
        system("mkdir .minigit");
        std::cout << "Created .minigit folder\n";
    } else {
        std::cout << ".minigit folder already exists\n";
    }
    lastCommitId = loadHEAD();  // load latest commit ID from HEAD file
}

void createFolders() {
    if (!directoryExists(".minigit/objects")) {
        system("mkdir .minigit/objects");
        std::cout << "Created .minigit/objects folder\n";
    }

    if (!directoryExists(".minigit/commits")) {
        system("mkdir .minigit/commits");
        std::cout << "Created .minigit/commits folder\n";
    }
}

std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return buf;
}

std::string generateCommitId() {
    std::stringstream ss;
    ss << std::time(nullptr);
    return ss.str();
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

// ---------------------- Core MiniGit Features ----------------------

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

void saveCommitMetadata(const Commit& commit) {
    std::string path = ".minigit/commits/" + commit.id + ".txt";
    std::ofstream out(path);
    if (!out) {
        std::cerr << "Error: Failed to save commit metadata.\n";
        return;
    }

    out << "Commit ID: " << commit.id << "\n";
    out << "Timestamp: " << commit.timestamp << "\n";
    out << "Message: " << commit.message << "\n";
    out << "Parent ID: " << commit.parentId << "\n";
    out << "Files:\n";
    for (const auto& file : commit.files) {
        out << " - " << file.first << " (v" << file.second << ")\n";
    }

    std::cout << "Saved commit metadata to: " << path << "\n";
}

void commit() {
    createFolders();

    Commit newCommit;
    newCommit.id = generateCommitId();
    newCommit.timestamp = getCurrentTimestamp();
    newCommit.parentId = lastCommitId;

    std::cout << "Enter commit message:\n> ";
    std::string message;
    std::getline(std::cin, message);
    newCommit.message = message;

    for (auto& f : stagedFiles) {
        std::string versionedName = f.fileName + "_" + std::to_string(f.versionCount);
        std::string dest = ".minigit/objects/" + versionedName;

        if (copyFile(f.fileName, dest)) {
            std::cout << "Committed " << f.fileName << " as " << versionedName << "\n";
            newCommit.files.push_back({f.fileName, f.versionCount});
            f.versionCount++;  // version increases after commit
        } else {
            std::cout << "Failed to commit " << f.fileName << "\n";
        }
    }

    saveCommitMetadata(newCommit);
    lastCommitId = newCommit.id;
    saveHEAD(newCommit.id);  // update HEAD with latest commit
}

// ---------------------- Main Program ----------------------

int main() {
    initializeMiniGit();
    addFile();

    std::cout << "\nFiles staged for tracking:\n";
    for (const auto& f : stagedFiles) {
        std::cout << "- " << f.fileName << " (current version count: " << f.versionCount << ")\n";
    }

    char answer;
    do {
        std::cout << "\nCommitting files...\n";
        commit();

        std::cout << "\nCommit again? (y/n): ";
        std::cin >> answer;
        std::cin.ignore();  // clear newline
    } while (answer == 'y' || answer == 'Y');

    std::cout << "Exiting MiniGit.\n";
    return 0;
}
