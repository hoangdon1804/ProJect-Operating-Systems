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
              << "exit - Exit shell\n";
}
