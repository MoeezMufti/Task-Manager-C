# Task-Manager-C
A console-based task management system developed in C. This program allows users to efficiently organize, track, and simulate the completion of tasks through a simple, menu-driven interface.

# Prerequisites
A C compiler (such as GCC) is required to build and run the program.

Linux: GCC is typically pre-installed. You can install it with sudo apt-get install build-essential.

macOS: Install the Xcode Command Line Tools by running xcode-select --install in your terminal.

Windows: Install a compiler suite like MinGW-w64.

# How to Build and Run
Clone the repository (if applicable) or download the task_manager.c file.

Open your terminal and navigate to the directory containing the file.

Compile the code using the GCC compiler. The -lpthread flag is crucial for linking the POSIX Threads library, which enables multi-threading.

Terminal Cmd: *gcc task_manager.c -o task_manager -lpthread*
Run the executable: *./task_manager*
