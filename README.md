MiniGit - A Custom Version Control System
Welcome to MiniGit, a lightweight, command-line-based version control system implemented in C++ from scratch. This project simulates core Git functionalities such as initialization, file staging, committing, branching, merging, and viewing commit history. It serves as an educational tool to understand the inner workings of version control systems and data structures like hashing, DAGs, and HashMaps.
This README provides step-by-step instructions to set up, compile, and test the MiniGit implementation on your local machine.
Repository Overview

Repository: MiniGit-project

Prerequisites
Before proceeding, ensure your system meets the following requirements:

Operating System: Windows, Linux, or macOS.
C++ Compiler: Install a C++17-compliant compiler:
Linux: g++ (install via sudo apt install g++ on Ubuntu).
macOS: g++ (install via xcode-select --install).
Windows: MinGW (install via MinGW-w64 or use WSL with Ubuntu).


Terminal: Any command-line interface (e.g., Bash, PowerShell, Command Prompt).
Filesystem Support: Ensure your compiler supports C++17 <filesystem> (e.g., g++ version 8 or later).

Installation and Setup
Follow these steps to clone the repository and set up the project:

Clone the Repository:

Open a terminal and run:git clone https://github.com/bethe1996/MiniGit-project.git
cd MiniGit-project




Verify the Code:

Ensure main.cpp is present in the directory:dir  # Windows
ls   # Linux/macOS




Compile the Code:

Compile the project using g++ with C++17 support:g++ -std=c++17 main.cpp -o minigit


Windows: If using MinGW, the executable will be minigit.exe.
Note: If <filesystem> is not supported, add the filesystem library:g++ -std=c++17 main.cpp -o minigit -lstdc++fs


If compilation fails, check your g++ version:g++ --version

Upgrade if necessary (e.g., install a newer version of MinGW or update your package manager).



Usage
MiniGit is a command-line tool that mimics Git's basic commands. Run the compiled executable with the following commands:

Initialize a Repository:
./minigit init


Creates a .minigit/ directory with subdirectories for objects, refs, and files like HEAD.


Add Files:
./minigit add <filename>


Stages a file for the next commit (e.g., minigit add test.txt).


Commit Changes:
./minigit commit -m "Commit message"


Commits staged files with a message (e.g., minigit commit -m "Initial commit").


View Log:
./minigit log


Displays the commit history from HEAD backward.


Create a Branch:
./minigit branch <branch-name>


Creates a new branch (e.g., minigit branch dev).


Checkout a Branch or Commit:
./minigit checkout <branch-name>  # e.g., minigit checkout dev
./minigit checkout <commit-hash>  # e.g., minigit checkout 3453a856


Switches the working directory to the specified branch or commit. The executable is preserved during this operation.


Merge Branches:
./minigit merge <branch-name>


Merges the specified branch into the current branch, handling conflicts with a three-way merge strategy.



Testing the Implementation
To verify the MiniGit functionality, follow these steps in a test environment:

Create a Test Directory:

From the MiniGit-project directory:mkdir test_repo
cd test_repo
cp ../minigit .  # Linux/macOS
copy ..\minigit.exe .  # Windows




Run Basic Tests:

Initialize:./minigit init  # or minigit.exe on Windows


Verify .minigit/ is created.


Add and Commit:echo "Hello, MiniGit!" > test.txt
./minigit add test.txt
./minigit commit -m "Initial commit"


Verify a commit hash is generated and stored in .minigit/refs/heads/master.


Log:./minigit log


Check the commit details are displayed.


Branch:./minigit branch dev


Verify .minigit/refs/heads/dev exists.


Checkout:./minigit checkout dev


Verify test.txt remains, and minigit (or minigit.exe) is not deleted. Check .minigit/HEAD points to refs/heads/dev.


Modify and Commit on dev:echo "Modified on dev" >> test.txt
./minigit add test.txt
./minigit commit -m "Update on dev"


Merge:./minigit checkout master
echo "Master file" > master.txt
./minigit add master.txt
./minigit commit -m "Add master.txt on master"
./minigit merge dev


Verify both test.txt and master.txt are present with expected content.




Test Edge Cases:

Invalid File: ./minigit add nonexistent.txt (should fail with "Error: File does not exist").
Re-run Init: ./minigit init (should fail with "Error: .minigit already exists").
Conflict Merge: Modify test.txt differently on master and another branch, then merge to test conflict detection.


Clean Up:

Remove the test directory:cd ..
rm -rf test_repo  # Linux/macOS
rmdir /s /q test_repo  # Windows





Troubleshooting

Compilation Errors:

Ensure g++ supports C++17. Update your compiler if needed.
Check for missing libraries (e.g., -lstdc++fs for <filesystem>).


"Access is denied" on Checkout:

The code is updated to skip deleting the running executable, resolving this issue on Windows. If it persists, verify the executable name matches argv[0] (e.g., minigit.exe).


File Not Found:

Ensure you’re running commands from the directory containing minigit or minigit.exe.


Merge Conflicts:

If a merge fails with "CONFLICT", resolve manually by editing the conflicting file and recommitting.


Contributing
Feel free to fork this repository, make improvements, and submit pull requests. Potential enhancements include:

Implementing SHA-1 hashing for better collision resistance.
Adding a diff command for line-by-line differences.
Supporting conflict markers in merge.

License
This project is open-source. Feel free to use and modify it for educational purposes. Consider adding your own license if you distribute it further.
Acknowledgements

Last updated: June 20, 2025, 12:17 PM EAT
