#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "Feature/process.h"
#include "Feature/signal_handler.h"
#include "Feature/help.h"
#include "Feature/cd_command.h"

ProcessManager manager; // Global để signal handler truy cập

int main()
{
    char input[512];
    DWORD pid = GetCurrentProcessId();

    // Đăng ký Ctrl+C
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE))
    {
        std::cerr << "Failed to set control handler" << std::endl;
        return 1;
    }

    // Intro
    std::cout << "========================================\n";
    std::cout << "              Tiny Shell                \n";
    std::cout << "========================================\n";
    std::cout << "Welcome to Tiny Shell!\n";
    std::cout << "PID of Tiny Shell: " << pid << "\n";
    std::cout << "Type 'help' to see the list of available commands.\n";
    std::cout << "========================================\n";

    while (true)
    {
        std::cout << "my_shell> ";
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0'; // Xóa ký tự newline

        // -------- Built-in Commands --------
        if (strcmp(input, "exit") == 0)
            break;

        if (strcmp(input, "help") == 0)
        {
            print_help();
            continue;
        }

        if (strncmp(input, "cd ", 3) == 0)
        {
            handle_cd(input + 3);
            continue;
        }

        if (strcmp(input, "list") == 0)
        {
            manager.list_process();
            continue;
        }

        if (strncmp(input, "kill ", 5) == 0)
        {
            DWORD pid = atoi(input + 5);
            manager.kill_process(pid);
            continue;
        }

        if (strncmp(input, "stop ", 5) == 0)
        {
            DWORD pid = atoi(input + 5);
            manager.stop_process(pid);
            continue;
        }

        if (strncmp(input, "resume ", 7) == 0)
        {
            DWORD pid = atoi(input + 7);
            manager.resume_process(pid);
            continue;
        }

        // -------- External Commands --------
        if (strncmp(input, "start_foreground ", 17) == 0)
        {
            manager.execute_foreground(input + 17);
        }
        else if (strncmp(input, "start_background ", 17) == 0)
        {
            manager.execute_background(input + 17);
        }
        else
        {
            manager.execute_foreground(input); // Default: foreground
        }
    }

    // Xóa Ctrl+C handler
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE))
    {
        std::cerr << "Failed to remove control handler" << std::endl;
    }
    return 0;
}
