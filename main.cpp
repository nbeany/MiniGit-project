#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <filesystem>
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

// Structure to represent a commit
struct Commit {
    std::vector<std::string> parents;         // Parent commit hashes
    std::string timestamp;                    // Commit timestamp
    std::string message;                      // Commit message
    std::map<std::string, std::string> files; // Filename to blob hash mapping
};

// Simple hash function for content (may have collisions)
unsigned long custom_hash(const std::string& str) {
    unsigned long hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// Convert hash to a fixed-length string
std::string hash_to_string(unsigned long hash) {
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

// Get current timestamp in ISO format
std::string get_current_timestamp() {
    auto now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

// Serialize a commit object to a string
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

// Load a commit from its hash
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

// Check if one commit is an ancestor of another
bool is_ancestor(const std::string& ancestor, const std::string& descendant) {
    std::set<std::string> visited;
    std::queue<std::string> queue;
    queue.push(descendant);
    while (!queue.empty()) {
        std::string commit = queue.front();
        queue.pop();
        if (commit == ancestor) {
            return true;
        }
        if (visited.find(commit) != visited.end()) {
            continue;
        }
        visited.insert(commit);
        Commit commit_obj = load_commit(commit);
        for (const auto& parent : commit_obj.parents) {
            queue.push(parent);
        }
    }
    return false;
}

// Find the lowest common ancestor (LCA) of two commits
std::string find_lca(const std::string& commit1, const std::string& commit2) {
    std::queue<std::string> queue1;
    std::queue<std::string> queue2;
    std::set<std::string> visited1;
    std::set<std::string> visited2;
    queue1.push(commit1);
    queue2.push(commit2);
    while (!queue1.empty() || !queue2.empty()) {
        if (!queue1.empty()) {
            std::string c1 = queue1.front();
            queue1.pop();
            if (visited2.find(c1) != visited2.end()) {
                return c1;
            }
            if (visited1.find(c1) == visited1.end()) {
                visited1.insert(c1);
                Commit commit = load_commit(c1);
                for (const auto& parent : commit.parents) {
                    queue1.push(parent);
                }
            }
        }
        if (!queue2.empty()) {
            std::string c2 = queue2.front();
            queue2.pop();
            if (visited1.find(c2) != visited1.end()) {
                return c2;
            }
            if (visited2.find(c2) == visited2.end()) {
                visited2.insert(c2);
                Commit commit = load_commit(c2);
                for (const auto& parent : commit.parents) {
                    queue2.push(parent);
                }
            }
        }
    }
    return ""; // No common ancestor found
}

// Initialize a new MiniGit repository
void init() {
    if (fs::exists(".minigit")) {
        std::cerr << "Error: .minigit already exists.\n";
        return;
    }
    fs::create_directory(".minigit");
    fs::create_directory(".minigit/objects");
    fs::create_directory(".minigit/refs");
    fs::create_directory(".minigit/refs/heads");
    std::ofstream head_file(".minigit/HEAD");
    head_file << "ref: refs/heads/master";
    head_file.close();
    std::ofstream master_file(".minigit/refs/heads/master");
    master_file << "0000000000000000"; // Initial null commit
    master_file.close();
    std::ofstream index_file(".minigit/index");
    index_file.close();
    std::cout << "Initialized empty MiniGit repository in .minigit/\n";
}

// Add a file to the staging area
void add(const std::string& filename) {
    if (!fs::exists(filename)) {
        std::cerr << "Error: File does not exist: " << filename << "\n";
        return;
    }
    std::ifstream file(filename, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    unsigned long hash = custom_hash(content);
    std::string hash_str = hash_to_string(hash);
    std::string blob_path = ".minigit/objects/" + hash_str;
    if (!fs::exists(blob_path)) {
        std::ofstream blob_file(blob_path, std::ios::binary);
        blob_file << content;
        blob_file.close();
    }
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
    std::cout << "Added " << filename << " to staging area.\n";
}

// Commit staged changes
void commit(const std::string& message) {
    std::ifstream head_file(".minigit/HEAD");
    std::string head_content;
    std::getline(head_file, head_content);
    head_file.close();
    std::string current_branch;
    if (head_content.substr(0, 5) == "ref: ") {
        current_branch = head_content.substr(5);
    } else {
        std::cerr << "Error: Detached HEAD not supported.\n";
        return;
    }
    std::string branch_path = ".minigit/" + current_branch;
    std::ifstream branch_file(branch_path);
    std::string last_commit_hash;
    std::getline(branch_file, last_commit_hash);
    branch_file.close();
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
    Commit new_commit;
    new_commit.timestamp = get_current_timestamp();
    new_commit.message = message;
    new_commit.files = index;
    if (last_commit_hash != "0000000000000000") {
        Commit last_commit = load_commit(last_commit_hash);
        if (last_commit.files == index) {
            std::cout << "No changes to commit.\n";
            return;
        }
        new_commit.parents.push_back(last_commit_hash);
    }
    std::string commit_content = serialize_commit(new_commit);
    unsigned long commit_hash = custom_hash(commit_content);
    std::string commit_hash_str = hash_to_string(commit_hash);
    std::string commit_path = ".minigit/objects/" + commit_hash_str;
    std::ofstream commit_file(commit_path);
    commit_file << commit_content;
    commit_file.close();
    std::ofstream branch_file_out(branch_path);
    branch_file_out << commit_hash_str;
    branch_file_out.close();
    std::cout << "Committed as " << commit_hash_str << "\n";
}

// Display commit history
void log() {
    std::ifstream head_file(".minigit/HEAD");
    std::string head_content;
    std::getline(head_file, head_content);
    head_file.close();
    std::string current_branch;
    if (head_content.substr(0, 5) == "ref: ") {
        current_branch = head_content.substr(5);
    } else {
        std::cerr << "Error: Detached HEAD not supported.\n";
        return;
    }
    std::string branch_path = ".minigit/" + current_branch;
    std::ifstream branch_file(branch_path);
    std::string commit_hash;
    std::getline(branch_file, commit_hash);
    branch_file.close();
    if (commit_hash == "0000000000000000") {
        std::cout << "No commits yet.\n";
        return;
    }
    while (true) {
        Commit commit = load_commit(commit_hash);
        std::cout << "Commit " << commit_hash << "\n";
        std::cout << "Date: " << commit.timestamp << "\n";
        std::cout << commit.message << "\n\n";
        if (commit.parents.empty()) {
            break;
        }
        commit_hash = commit.parents[0];
    }
}

// Create a new branch
void branch(const std::string& branch_name) {
    std::ifstream head_file(".minigit/HEAD");
    std::string head_content;
    std::getline(head_file, head_content);
    head_file.close();
    std::string current_branch;
    if (head_content.substr(0, 5) == "ref: ") {
        current_branch = head_content.substr(5);
    } else {
        std::cerr << "Error: Detached HEAD not supported.\n";
        return;
    }
    std::string branch_path = ".minigit/" + current_branch;
    std::ifstream branch_file(branch_path);
    std::string commit_hash;
    std::getline(branch_file, commit_hash);
    branch_file.close();
    if (commit_hash == "0000000000000000") {
        std::cerr << "Error: No commits yet. Cannot create branch.\n";
        return;
    }
    std::string new_branch_path = ".minigit/refs/heads/" + branch_name;
    if (fs::exists(new_branch_path)) {
        std::cerr << "Error: Branch already exists.\n";
        return;
    }
    std::ofstream new_branch_file(new_branch_path);
    new_branch_file << commit_hash;
    new_branch_file.close();
    std::cout << "Created branch " << branch_name << "\n";
}

// Checkout a branch or commit
void checkout(const std::string& target, const std::string& executable_name) {
    std::string commit_hash;
    std::string branch_path = ".minigit/refs/heads/" + target;
    if (fs::exists(branch_path)) {
        std::ifstream branch_file(branch_path);
        std::getline(branch_file, commit_hash);
        branch_file.close();
        std::ofstream head_file(".minigit/HEAD");
        head_file << "ref: refs/heads/" << target;
        head_file.close();
    } else {
        commit_hash = target;
        if (!fs::exists(".minigit/objects/" + commit_hash)) {
            std::cerr << "Error: Commit hash does not exist.\n";
            return;
        }
        std::ofstream head_file(".minigit/HEAD");
        head_file << commit_hash;
        head_file.close();
    }
    Commit commit = load_commit(commit_hash);
    // Get the executable filename without path
    std::string exec_filename = fs::path(executable_name).filename().string();
    for (const auto& entry : fs::directory_iterator(".")) {
        std::string filename = entry.path().filename().string();
        if (filename != ".minigit" && filename != exec_filename) {
            try {
                fs::remove_all(entry.path());
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error removing file " << filename << ": " << e.what() << "\n";
            }
        }
    }
    for (const auto& pair : commit.files) {
        std::string filename = pair.first;
        std::string blob_hash = pair.second;
        std::string blob_path = ".minigit/objects/" + blob_hash;
        std::ifstream blob_file(blob_path, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(blob_file)), std::istreambuf_iterator<char>());
        blob_file.close();
        std::ofstream file(filename, std::ios::binary);
        file << content;
        file.close();
    }
    std::ofstream index_file(".minigit/index");
    for (const auto& pair : commit.files) {
        index_file << pair.first << ":" << pair.second << "\n";
    }
    index_file.close();
    std::cout << "Checked out to " << target << "\n";
}

// Merge a branch into the current branch
void merge(const std::string& branch_name) {
    if (!fs::exists(".minigit/refs/heads/" + branch_name)) {
        std::cerr << "Error: Branch does not exist: " << branch_name << "\n";
        return;
    }
    std::ifstream head_file(".minigit/HEAD");
    std::string head_content;
    std::getline(head_file, head_content);
    head_file.close();
    std::string current_branch;
    if (head_content.substr(0, 5) == "ref: ") {
        current_branch = head_content.substr(5);
    } else {
        std::cerr << "Error: Detached HEAD not supported.\n";
        return;
    }
    std::string current_branch_path = ".minigit/" + current_branch;
    std::ifstream current_branch_file(current_branch_path);
    std::string current_commit_hash;
    std::getline(current_branch_file, current_commit_hash);
    current_branch_file.close();
    std::string target_branch_path = ".minigit/refs/heads/" + branch_name;
    std::ifstream target_branch_file(target_branch_path);
    std::string target_commit_hash;
    std::getline(target_branch_file, target_commit_hash);
    target_branch_file.close();

    // Check for fast-forward or already up-to-date cases
    if (current_commit_hash == target_commit_hash) {
        std::cout << "Already up-to-date.\n";
        return;
    }
    if (is_ancestor(current_commit_hash, target_commit_hash)) {
        std::ofstream current_branch_file_out(current_branch_path);
        current_branch_file_out << target_commit_hash;
        current_branch_file_out.close();
        checkout(branch_name, "");
        std::cout << "Fast-forward merge.\n";
        return;
    }
    if (is_ancestor(target_commit_hash, current_commit_hash)) {
        std::cout << "Already up-to-date.\n";
        return;
    }

    // Perform three-way merge
    std::string lca_hash = find_lca(current_commit_hash, target_commit_hash);
    if (lca_hash.empty()) {
        std::cerr << "Error: No common ancestor found.\n";
        return;
    }
    Commit lca = load_commit(lca_hash);
    Commit current = load_commit(current_commit_hash);
    Commit target = load_commit(target_commit_hash);

    // Collect all files involved in the merge
    std::set<std::string> all_files;
    for (const auto& pair : lca.files) all_files.insert(pair.first);
    for (const auto& pair : current.files) all_files.insert(pair.first);
    for (const auto& pair : target.files) all_files.insert(pair.first);

    std::map<std::string, std::string> merged_files;
    std::vector<std::string> conflicts;
    for (const std::string& file : all_files) {
        bool in_lca = lca.files.find(file) != lca.files.end();
        bool in_current = current.files.find(file) != current.files.end();
        bool in_target = target.files.find(file) != target.files.end();
        std::string lca_hash = in_lca ? lca.files[file] : "";
        std::string current_hash = in_current ? current.files[file] : "";
        std::string target_hash = in_target ? target.files[file] : "";

        if (in_lca && in_current && in_target) {
            if (current_hash == lca_hash && target_hash == lca_hash) {
                merged_files[file] = lca_hash;
            } else if (current_hash == lca_hash) {
                merged_files[file] = target_hash;
            } else if (target_hash == lca_hash) {
                merged_files[file] = current_hash;
            } else if (current_hash == target_hash) {
                merged_files[file] = current_hash;
            } else {
                conflicts.push_back(file);
            }
        } else if (in_lca && in_current && !in_target) {
            if (current_hash == lca_hash) {
                // File deleted in target
            } else {
                conflicts.push_back(file);
            }
        } else if (in_lca && !in_current && in_target) {
            if (target_hash == lca_hash) {
                // File deleted in current
            } else {
                conflicts.push_back(file);
            }
        } else if (!in_lca && in_current && in_target) {
            if (current_hash == target_hash) {
                merged_files[file] = current_hash;
            } else {
                conflicts.push_back(file);
            }
        } else if (!in_lca && in_current && !in_target) {
            merged_files[file] = current_hash;
        } else if (!in_lca && !in_current && in_target) {
            merged_files[file] = target_hash;
        }
    }

    if (!conflicts.empty()) {
        for (const std::string& file : conflicts) {
            std::cout << "CONFLICT: both modified " << file << "\n";
        }
        std::cout << "Merge aborted.\n";
        return;
    }

    // Update working directory
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.path().filename() != ".minigit") {
            try {
                fs::remove_all(entry.path());
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Error removing file " << entry.path().filename().string() << ": " << e.what() << "\n";
            }
        }
    }
    for (const auto& pair : merged_files) {
        std::string filename = pair.first;
        std::string blob_hash = pair.second;
        std::string blob_path = ".minigit/objects/" + blob_hash;
        std::ifstream blob_file(blob_path, std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(blob_file)), std::istreambuf_iterator<char>());
        blob_file.close();
        std::ofstream file(filename, std::ios::binary);
        file << content;
        file.close();
    }

    // Update index
    std::ofstream index_file(".minigit/index");
    for (const auto& pair : merged_files) {
        index_file << pair.first << ":" << pair.second << "\n";
    }
    index_file.close();

    // Create merge commit
    Commit new_commit;
    new_commit.parents = {current_commit_hash, target_commit_hash};
    new_commit.timestamp = get_current_timestamp();
    new_commit.message = "Merge branch " + branch_name;
    new_commit.files = merged_files;
    std::string commit_content = serialize_commit(new_commit);
    unsigned long commit_hash = custom_hash(commit_content);
    std::string commit_hash_str = hash_to_string(commit_hash);
    std::string commit_path = ".minigit/objects/" + commit_hash_str;
    std::ofstream commit_file(commit_path);
    commit_file << commit_content;
    commit_file.close();

    // Update current branch
    std::ofstream branch_file(current_branch_path);
    branch_file << commit_hash_str;
    branch_file.close();
    std::cout << "Merged " << branch_name << " into " << current_branch << "\n";
}

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
            std::cerr << "Usage: minigit branch <branch-name>\n";
            return 1;
        }
        branch(argv[2]);
    } else if (command == "checkout") {
        if (argc < 3) {
            std::cerr << "Usage: minigit checkout <branch-name> or <commit-hash>\n";
            return 1;
        }
        checkout(argv[2], argv[0]);
    } else if (command == "merge") {
        if (argc < 3) {
            std::cerr << "Usage: minigit merge <branch-name>\n";
            return 1;
        }
        merge(argv[2]);
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        return 1;
    }
    return 0;
}