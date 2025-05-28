#pragma once
#include <iostream>

inline void print_help()
{
    std::cout << "Available commands:\n"
              << "start_foreground <cmd> - Run a command in foreground\n"
              << "start_background <cmd> - Run a command in background\n"
              << "list - List background processes\n"
              << "kill <pid> - Kill a background process\n"
              << "stop <pid> - Suspend a background process\n"
              << "resume <pid> - Resume a suspended process\n"
              << "exit - Exit shell\n"
              << "help - Show this help message\n"
              << "date - Show current date\n"
              << "time - Show current time\n"
              << "datetime - Show current date and time\n"
              << "cd <path> - Change current directory\n"
              << "dir - List files in current directory\n"
              << "path - Show current environment variables\n"
              << "addpath <path> - Add a path to environment variables\n";
}
