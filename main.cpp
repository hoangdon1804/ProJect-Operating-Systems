#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "Feature/feature.h"

ProcessManager manager; // Global để signal handler truy cập
PROCESS_INFORMATION foregroundProcess = {0};

int main()
{
    char input[512];

    // Đăng ký Ctrl+C
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE))
    {
        std::cerr << "Failed to set control handler" << std::endl;
        return 1;
    }
    while (true)
    {
        std::cout << "my_shell> ";
        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0'; // Xóa newline

        if (strlen(input) == 0)
            continue;

        // -------- Built-in Commands --------
        else if (strcmp(input, "exit") == 0)
        {
            manager.terminate_all_background();
            break;
        }
        else if (strcmp(input, "help") == 0)
        {
            print_help();
            continue;
        }

        else if (strncmp(input, "cd ", 3) == 0)
        {
            handle_cd(input + 3);
            continue;
        }

        else if (strcmp(input, "list") == 0)
        {
            manager.list_process();
            continue;
        }

        else if (strncmp(input, "kill ", 5) == 0)
        {
            DWORD pid = atoi(input + 5);
            manager.kill_process(pid);
            continue;
        }

        else if (strncmp(input, "stop ", 5) == 0)
        {
            DWORD pid = atoi(input + 5);
            manager.stop_process(pid);
            continue;
        }

        else if (strncmp(input, "resume ", 7) == 0)
        {
            DWORD pid = atoi(input + 7);
            manager.resume_process(pid);
            continue;
        }

        // ------- Time-related Commands -------
        else if (strcmp(input, "date") == 0)
        {
            ShellTime::showDate();
            continue;
        }

        else if (strcmp(input, "time") == 0)
        {
            ShellTime::showTime();
            continue;
        }

        else if (strcmp(input, "datetime") == 0)
        {
            ShellTime::showDateTime();
            continue;
        }
        else if (strcmp(input, "dir") == 0)
        {
            showCurrentDirectoryContents();
            continue;
        }
        else if (strncmp(input, "runbat ", 7) == 0)
        {
            std::string batFile = input + 7;
            execute_bat(batFile);
            continue;
        }

        // -------- External Commands --------
        else if (strncmp(input, "start_foreground ", 17) == 0)
        {
            manager.execute_foreground(input + 17);
        }
        else if (strncmp(input, "start_background ", 17) == 0)
        {
            manager.execute_background(*(input + 17) ? std::vector<std::string>{input + 17} : std::vector<std::string>{});
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
