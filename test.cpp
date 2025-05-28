#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <signal.h>
#include <tlhelp32.h>

std::map<DWORD, std::pair<HANDLE, std::string>> processMap;
HANDLE foregroundProcess = NULL;

// Kiểm tra args của 1 tiến trình xem có phải là foreground hay không
bool isForeground(std::vector<std::string> args)
{
    return (args.back() != "&");
}

// Kiểm tra file có phải theo batch không
bool isBatchFile(const std::string &filepath)
{
    if (filepath.length() < 4)
        return false;

    std::string extension = filepath.substr(filepath.length() - 4);
    for (char &c : extension)
        c = tolower(c);

    return (extension == ".bat");
}

// Xử lý sự kiện Ctrl + C
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    if (fdwCtrlType == CTRL_C_EVENT)
    {
        if (foregroundProcess != NULL)
        {
            std::cout << "\n[Shell] Ctrl+C detected! Terminating foreground process...\n";
            TerminateProcess(foregroundProcess, 0);
            CloseHandle(foregroundProcess);
            foregroundProcess = NULL;
            std::cout << "[Shell] Process stopped.\n";
        }
        else
        {
            std::cout << "\n[Shell] Exitting shell.\n";
        }

        std::cin.clear();

        return TRUE;
    }
    return FALSE;
}

// Xóa tiến trình background
void cleanupBackgroundProcesses()
{
    for (auto it = processMap.begin(); it != processMap.end();)
    {
        DWORD exitCode;
        if (GetExitCodeProcess(it->second.first, &exitCode) && exitCode != STILL_ACTIVE)
        {
            CloseHandle(it->second.first);
            it = processMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// Xử lý nhập câu lệnh, chuyển về vector args
std::vector<std::string> parseCmd(std::string cmd)
{
    std::vector<std::string> result;
    std::istringstream iss(cmd);
    std::string token;
    while (iss >> token)
    {
        result.push_back(token);
    }
    return result;
}

// Thực thi câu lệnh (tạo tiến trình)
void executeCmd(std::vector<std::string> args)
{
    if (args.empty())
        return;

    std::string command = args[0];
    std::string arguments;
    for (size_t i = 1; i < args.size(); ++i)
    {
        arguments += args[i] + " ";
    }

    STARTUPINFO si = {sizeof(STARTUPINFO)};
    PROCESS_INFORMATION pi;

    // Chuyển string thành wchar_t
    std::string fullCommand = command + " " + arguments;
    wchar_t cmdLine[256];
    mbstowcs(cmdLine, fullCommand.c_str(), fullCommand.size() + 1);
    
    // Tạo tiến trình
    if (CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
    {
        std::cout << "[Shell] Process started: " << pi.dwProcessId << "\n";
        processMap[pi.dwProcessId] = {pi.hProcess, fullCommand};

        if (isForeground(args))
        {
            foregroundProcess = pi.hProcess;
            WaitForSingleObject(pi.hProcess, INFINITE);
            processMap.erase(pi.dwProcessId);
            foregroundProcess = NULL;
        }
        CloseHandle(pi.hThread);
    }
    else
    {
        std::cerr << "[Shell] Invalid command, error code: " << GetLastError() << "\n";
    }
}

// Liệt kê tiến trình
void listProcess()
{
    std::cout << "[Shell] List of running background processes:\n";
    std::cout << "--------------------------------------------\n";
    std::cout << "|  PID  | Command         | Status       |\n";
    std::cout << "--------------------------------------------\n";

    for (const auto &entry : processMap)
    {
        DWORD pid = entry.first;
        HANDLE hProcess = entry.second.first;
        std::string command = entry.second.second;

        DWORD exitCode;
        std::string status;
        if (GetExitCodeProcess(hProcess, &exitCode))
        {
            if (exitCode == STILL_ACTIVE)
            {
                status = "Running";
            }
            else
            {
                status = "Exited";
            }
        }
        else
        {
            status = "Unknown";
        }

        std::cout << "| " << pid << " | " << command << " | " << status << " |\n";
    }

    std::cout << "--------------------------------------------\n";
}

// Dừng tiến trình
void stopProcess(DWORD pid)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    // Vì hàm SuspendThread cần tham số là hThread nên cần tìm hThread qua pid
    if (Thread32First(hSnapshot, &te))
    {
        do
        {
            if (te.th32OwnerProcessID == pid)
            {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (hThread)
                {
                    SuspendThread(hThread);
                    CloseHandle(hThread);
                    std::cout << "[Shell] Process " << pid << " stopped.\n";
                    break;
                }
                else
                {
                    std::cerr << "Failed to open thread: " << GetLastError() << std::endl;
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    }
    else
    {
        std::cerr << "Thread32First failed: " << GetLastError() << std::endl;
    }

    CloseHandle(hSnapshot);
}

void resumeProcess(DWORD pid)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
        return;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    // Vì hàm ResumeThread cần tham số là hThread nên cần tìm hThread qua pid
    if (Thread32First(hSnapshot, &te))
    {
        do
        {
            if (te.th32OwnerProcessID == pid)
            {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (hThread)
                {
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                    std::cout << "[Shell] Process " << pid << " resumed.\n";
                    break;
                }
                else
                {
                    std::cerr << "Failed to open thread: " << GetLastError() << std::endl;
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    }
    else
    {
        std::cerr << "Thread32First failed: " << GetLastError() << std::endl;
    }

    CloseHandle(hSnapshot);
}

// Hủy tiến trình
void killProcess(DWORD pid)
{
    if (processMap.find(pid) != processMap.end())
    {
        TerminateProcess(processMap[pid].first, 0);
        DWORD result = WaitForSingleObject(processMap[pid].first, 1000);
        if (result == WAIT_OBJECT_0)
        {
            std::cerr << "[Shell] Process " << pid << " killed\n";
        }
        else
        {
            std::cerr << "[Shell] Process " << pid << " not killed\n";
        }
        CloseHandle(processMap[pid].first);
        processMap.erase(pid);
    }
    else
    {
        std::cerr << "[Shell] Process " << pid << " not found\n";
    }
}

void showDate()
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    std::cout << "[Shell] Current date: " << st.wDay << "/" << st.wMonth << "/" << st.wYear << std::endl;
}

void showDir()
{
    char buffer[256];
    GetCurrentDirectoryA(256, buffer);
    std::cout << "[Shell] Current directory: " << buffer << std::endl;
}

// Hàm xử lý với biến môi trường (hiển thị - show, thêm - add)
void showPath()
{
    char buffer[256];
    GetEnvironmentVariableA("PATH", buffer, 256);
    std::cout << "[Shell] Current environment variables: " << buffer << std::endl;
}

void addPath(std::vector<std::string> args)
{
    if (args.size() == 2)
    {
        std::string path = args[1];
        SetEnvironmentVariableA("PATH", path.c_str());
        std::cout << "[Shell] PATH added: " << path << std::endl;
    }
    else
    {
        std::cerr << "[Shell] Invalid command\n";
    }
}

// Thực thi file batch
void executeBatchFile(const std::string &filepath)
{
    if (!isBatchFile(filepath))
    {
        std::cerr << "[Shell] Invalid file format\n";
        return;
    }
    else
    {
        std::string command = "cmd.exe /c " + filepath;
        executeCmd(parseCmd(command));
    }
}

// Hiển thị câu lệnh
void help()
{
    std::cout << "[Shell] This is a simple shell program" << std::endl;
    std::cout << "[Shell] You can use the following commands:" << std::endl;
    std::cout << "[Shell] 1. help: show help" << std::endl;
    std::cout << "[Shell] 2. exit: exit the shell" << std::endl;
    std::cout << "[Shell] 3. list: list all running processes" << std::endl;
    std::cout << "[Shell] 4. stop <pid>: stop a process with given pid" << std::endl;
    std::cout << "[Shell] 5. resume <pid>: resume a process with given pid" << std::endl;
    std::cout << "[Shell] 6. kill <pid>: kill a process with given pid" << std::endl;
    std::cout << "[Shell] 7. <command> <arguments>: execute a command with arguments" << std::endl;
    std::cout << "[Shell] 8. date: show current date" << std::endl;
    std::cout << "[Shell] 9. dir: show current directory" << std::endl;
    std::cout << "[Shell] 10. path: show current environment variables" << std::endl;
    std::cout << "[Shell] 11. addpath <path>: add a path to environment variables" << std::endl;
    std::cout << "[Shell] 12. execute <filepath>: execute a batch file" << std::endl;
    std::cout << "[Shell] 13. Ctrl+C: stop all foreground processes" << std::endl;
}

// Thực thi câu lệnh
void process(std::string cmd)
{
    std::vector<std::string> args = parseCmd(cmd);
    if (args[0] == "help")
    {
        help();
    }
    else if (args[0] == "exit")
    {
        exit(0);
    }
    else if (args[0] == "list")
    {
        listProcess();
    }
    else if (args[0] == "stop")
    {
        if (args.size() == 2)
        {
            stopProcess(std::stoi(args[1]));
        }
        else
        {
            std::cerr << "[Shell] Invalid command\n";
        }
    }
    else if (args[0] == "resume")
    {
        if (args.size() == 2)
        {
            resumeProcess(std::stoi(args[1]));
        }
        else
        {
            std::cerr << "[Shell] Invalid command\n";
        }
    }
    else if (args[0] == "kill")
    {
        if (args.size() == 2)
        {
            killProcess(std::stoi(args[1]));
        }
        else
        {
            std::cerr << "[Shell] Invalid command\n";
        }
    }
    else if (args[0] == "date")
    {
        showDate();
    }
    else if (args[0] == "dir")
    {
        showDir();
    }
    else if (args[0] == "path")
    {
        showPath();
    }
    else if (args[0] == "addpath")
    {
        addPath(args);
    }
    else if (args[0] == "execute")
    {
        if (args.size() == 2)
        {
            executeBatchFile(args[1]);
        }
        else
        {
            std::cerr << "[Shell] Invalid command\n";
        }
    }
    else
    {
        executeCmd(args);
    }
}

int main()
{
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    std::string input;
    while (1)
    {   
        std::cout << "myShell> ";
        std::getline(std::cin, input);
        process(input);
    }
    return 0;
}