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
// --- addSingleFile -------------------------------------------------
void addSingleFile(const std::string& name)
{
    for (const auto& f : stagedFiles)
        if (f.fileName == name) {
            std::cout << "Already staged.\n";
            return;
        }
    stagedFiles.push_back({name, 0});
    std::cout << "Staged " << name << "\n";
}

// --- commit (overload takes message) -------------------------------
void commit(const std::string& message)
{
    createFolders();

    Commit newCommit;
    newCommit.id        = generateCommitId();
    newCommit.timestamp = getCurrentTimestamp();
    newCommit.parentId  = lastCommitId;
    newCommit.message   = message;

    for (auto& f : stagedFiles) {
        std::string versionedName = f.fileName + "_" + std::to_string(f.versionCount);
        std::string dest = ".minigit/objects/" + versionedName;

        if (copyFile(f.fileName, dest)) {
            newCommit.files.push_back({f.fileName, f.versionCount});
            f.versionCount++;
        }
    }

    saveCommitMetadata(newCommit);
    lastCommitId = newCommit.id;
    saveHEAD(newCommit.id);
    std::cout << "Committed as " << newCommit.id << "\n";
}

// --- logHistory ----------------------------------------------------
void logHistory()
{
    std::string id = loadHEAD();
    if (id.empty()) { std::cout << "No commits yet.\n"; return; }

    while (!id.empty()) {
        std::ifstream in(".minigit/commits/" + id + ".txt");
        if (!in) { std::cerr << "Missing commit file for " << id << "\n"; break; }

        std::string line;
        std::string parent;
        while (std::getline(in, line)) {
            std::cout << line << '\n';
            if (line.rfind("Parent ID:", 0) == 0)
                parent = line.substr(11);          // crude parse
        }
        std::cout << "--------------------------\n";
        id = (parent == "" || parent == " ") ? "" : parent;
    }
}

// ---------------------- Main Program ----------------------

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "MiniGit commands: init | add <file> | commit -m \"msg\" | log\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "init") {
        initializeMiniGit();
        std::cout << "Repository initialised.\n";
        return 0;
    }

    // Every command below expects .minigit to exist
    initializeMiniGit();

    if (cmd == "add") {
        if (argc < 3) { std::cerr << "add <filename>\n"; return 1; }
        addSingleFile(argv[2]);            // new helper, see next block
    }
    else if (cmd == "commit") {
        if (argc >= 4 && std::string(argv[2]) == "-m") {
            commit(argv[3]);               // commit now takes message as param
        } else {
            std::cerr << "commit -m \"message\"\n"; return 1;
        }
    }
    else if (cmd == "log") {
        logHistory();                      // brand‑new function, coming up
    }
    else {
        std::cerr << "Unknown command.\n";
        return 1;
    }
    return 0;
}

