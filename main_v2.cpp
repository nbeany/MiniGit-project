#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem; // Use shorter alias for filesystem

// Represents a commit in the MiniGit system
struct Commit {
    std::vector<std::string> parents; 
    std::string timestamp;            
    std::string message;              
    std::map<std::string, std::string> files; 
};

// Custom hash function (djb2)
unsigned long custom_hash(const std::string& str) {
    unsigned long hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// Converts hash to hexadecimal string
std::string hash_to_string(unsigned long hash) {
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

// Gets current timestamp in ISO 8601 format
std::string get_current_timestamp() {
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

// Converts a Commit struct to a string for saving
std::string serialize_commit(const Commit& commit) {
    std::stringstream ss;
    for (const auto& parent : commit.parents) {
        ss << "parent " << parent << "\n";
    }
    ss << "timestamp " << commit.timestamp << "\n";
    ss << "message " << commit.message << "\n";
    for (const auto& pair : commit.files) {
        ss << pair.first << ":" << pair.second << "\n";
    }
    return ss.str();
}

// Loads a commit object from disk by hash
Commit load_commit(const std::string& commit_hash) {
    std::string commit_path = ".minigit/objects/" + commit_hash;
    std::ifstream commit_file(commit_path);
    Commit commit;
    std::string line;
    while (std::getline(commit_file, line)) {
        if (line.substr(0, 7) == "parent ") {
            commit.parents.push_back(line.substr(7));
        } else if (line.substr(0, 10) == "timestamp ") {
            commit.timestamp = line.substr(10);
        } else if (line.substr(0, 8) == "message ") {
            commit.message = line.substr(8);
        } else {
            // filename:blob_hash
            size_t pos = line.find(':');
            if (pos != std::string::npos) {
                std::string fn = line.substr(0, pos);
                std::string bh = line.substr(pos + 1);
                commit.files[fn] = bh;
            }
        }
    }
    commit_file.close();
    return commit;
}

// Initializes a new .minigit directory structure
void init() {
    fs::create_directory(".minigit");
    fs::create_directory(".minigit/objects");
    fs::create_directory(".minigit/refs");
    fs::create_directory(".minigit/refs/heads");

    // Initialize HEAD to master
    std::ofstream head_file(".minigit/HEAD");
    head_file << "ref: refs/heads/master";
    head_file.close();

    // Set master to empty (no commit yet)
    std::ofstream master_file(".minigit/refs/heads/master");
    master_file << "0000000000000000";
    master_file.close();

    // Empty index file
    std::ofstream index_file(".minigit/index");
    index_file.close();

    std::cout << "Initialized empty MiniGit repository in .minigit/" << std::endl;
}

// Adds a file to the staging area (index)
void add(const std::string& filename) {
    if (!fs::exists(filename)) {
        std::cerr << "Error: File does not exist: " << filename << std::endl;
        return;
    }

    // Read file content
    std::ifstream file(filename, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Generate hash and save blob if not already saved
    unsigned long hash = custom_hash(content);
    std::string hash_str = hash_to_string(hash);
    std::string blob_path = ".minigit/objects/" + hash_str;
    if (!fs::exists(blob_path)) {
        std::ofstream blob_file(blob_path, std::ios::binary);
        blob_file << content;
        blob_file.close();
    }

    // Update index file (staging area)
    std::map<std::string, std::string> index;
    std::ifstream index_file(".minigit/index");
    std::string line;
    while (std::getline(index_file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string fn = line.substr(0, pos);
            std::string bh = line.substr(pos + 1);
            index[fn] = bh;
        }
    }
    index_file.close();

    index[filename] = hash_str;

    std::ofstream index_file_out(".minigit/index");
    for (const auto& pair : index) {
        index_file_out << pair.first << ":" << pair.second << "\n";
    }
    index_file_out.close();

    std::cout << "Added " << filename << " to staging area." << std::endl;
}

// Creates a new commit from staged files
void commit(const std::string& message) {
    std::ifstream head_file(".minigit/HEAD");
    std::string head_content;
    std::getline(head_file, head_content);
    head_file.close();

    std::string current_branch;
    if (head_content.substr(0, 5) == "ref: ") {
        current_branch = head_content.substr(5);
    } else {
        std::cerr << "Error: Detached HEAD not supported." << std::endl;
        return;
    }

    std::string branch_path = ".minigit/" + current_branch;

    // Read the last commit hash from the branch
    std::ifstream branch_file(branch_path);
    std::string last_commit_hash;
    std::getline(branch_file, last_commit_hash);
    branch_file.close();

    // Load the staging area
    std::map<std::string, std::string> index;
    std::ifstream index_file(".minigit/index");
    std::string line;
    while (std::getline(index_file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string fn = line.substr(0, pos);
            std::string bh = line.substr(pos + 1);
            index[fn] = bh;
        }
    }
    index_file.close();

    // Create new commit
    Commit new_commit;
    new_commit.timestamp = get_current_timestamp();
    new_commit.message = message;
    new_commit.files = index;

    // Check if this is the first commit
    if (last_commit_hash != "0000000000000000") {
        Commit last_commit = load_commit(last_commit_hash);
        if (last_commit.files == index) {
            std::cout << "No changes to commit." << std::endl;
            return;
        }
        new_commit.parents.push_back(last_commit_hash);
    }

    // Save commit to disk
    std::string commit_content = serialize_commit(new_commit);
    std::string commit_hash_str = hash_to_string(custom_hash(commit_content));
    std::ofstream commit_file(".minigit/objects/" + commit_hash_str);
    commit_file << commit_content;
    commit_file.close();

    // Update branch pointer
    std::ofstream branch_file_out(branch_path);
    branch_file_out << commit_hash_str;
    branch_file_out.close();

    std::cout << "Committed as " << commit_hash_str << std::endl;
}

// Shows commit log history
void log() {
    std::ifstream head_file(".minigit/HEAD");
    std::string head_content;
    std::getline(head_file, head_content);
    head_file.close();

    std::string current_branch;
    if (head_content.substr(0, 5) == "ref: ") {
        current_branch = head_content.substr(5);
    } else {
        std::cerr << "Error: Detached HEAD not supported." << std::endl;
        return;
    }

    std::string branch_path = ".minigit/" + current_branch;
    std::ifstream branch_file(branch_path);
    std::string commit_hash;
    std::getline(branch_file, commit_hash);
    branch_file.close();

    if (commit_hash == "0000000000000000") {
        std::cout << "No commits yet." << std::endl;
        return;
    }

    // Walk through parent history
    while (true) {
        Commit commit = load_commit(commit_hash);
        std::cout << "Commit " << commit_hash << "\n";
        std::cout << "Date: " << commit.timestamp << "\n";
        std::cout << commit.message << "\n\n";
        if (commit.parents.empty()) break;
        commit_hash = commit.parents[0];
    }
}

// Creates a new branch pointing to the current commit
void branch(const std::string& branch_name) {
    std::ifstream head_file(".minigit/HEAD");
    std::string head_content;
    std::getline(head_file, head_content);
    head_file.close();

    std::string current_branch;
    if (head_content.substr(0, 5) == "ref: ") {
        current_branch = head_content.substr(5);
    } else {
        std::cerr << "Error: Detached HEAD not supported." << std::endl;
        return;
    }

    std::string branch_path = ".minigit/" + current_branch;
    std::ifstream branch_file(branch_path);
    std::string commit_hash;
    std::getline(branch_file, commit_hash);
    branch_file.close();

    if (commit_hash == "0000000000000000") {
        std::cerr << "Error: No commits yet. Cannot create branch." << std::endl;
        return;
    }

    std::string new_branch_path = ".minigit/refs/heads/" + branch_name;
    if (fs::exists(new_branch_path)) {
        std::cerr << "Error: Branch already exists." << std::endl;
        return;
    }

    std::ofstream new_branch_file(new_branch_path);
    new_branch_file << commit_hash;
    new_branch_file.close();
    std::cout << "Created branch " << branch_name << std::endl;
}

// Switches to another branch or commit
void checkout(const std::string& target) {
    std::string commit_hash;

    // If it's a branch name
    if (fs::exists(".minigit/refs/heads/" + target)) {
        std::ifstream branch_file(".minigit/refs/heads/" + target);
        std::getline(branch_file, commit_hash);
        branch_file.close();
        std::ofstream head_file(".minigit/HEAD");
        head_file << "ref: refs/heads/" << target;
        head_file.close();
    } else {
        // If it's a commit hash
        commit_hash = target;
        if (!fs::exists(".minigit/objects/" + commit_hash)) {
            std::cerr << "Error: Commit hash does not exist." << std::endl;
            return;
        }
        std::ofstream head_file(".minigit/HEAD");
        head_file << commit_hash;
        head_file.close();
    }

    // Replace working directory files with commit content
    Commit commit = load_commit(commit_hash);
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.path().filename() != ".minigit") {
            fs::remove_all(entry.path());
        }
    }

    // Restore files from blob
    for (const auto& pair : commit.files) {
        std::string filename = pair.first;
        std::string blob_hash = pair.second;
        std::ifstream blob_file(".minigit/objects/" + blob_hash, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(blob_file)), std::istreambuf_iterator<char>());
        blob_file.close();
        std::ofstream file(filename, std::ios::binary);
        file << content;
        file.close();
    }

    // Update index
    std::ofstream index_file(".minigit/index");
    for (const auto& pair : commit.files) {
        index_file << pair.first << ":" << pair.second << "\n";
    }
    index_file.close();

    std::cout << "Checked out to " << target << std::endl;
}

// Entry point and command parser
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: minigit <command> [<args>]\n";
        return 1;
    }

    std::string command = argv[1];
    if (command == "init") {
        init();
    } else if (command == "add") {
        if (argc < 3) {
            std::cerr << "Usage: minigit add <filename>\n";
            return 1;
        }
        add(argv[2]);
    } else if (command == "commit") {
        if (argc < 4 || std::string(argv[2]) != "-m") {
            std::cerr << "Usage: minigit commit -m <message>\n";
            return 1;
        }
        commit(argv[3]);
    } else if (command == "log") {
        log();
    } else if (command == "branch") {
        if (argc < 3) {
            std::cerr << "Usage: minigit branch <branch_name>\n";
            return 1;
        }
        branch(argv[2]);
    } else if (command == "checkout") {
        if (argc < 3) {
            std::cerr << "Usage: minigit checkout <branch_or_commit_hash>\n";
            return 1;
        }
        checkout(argv[2]);
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        return 1;
    }
    return 0;
}
