#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

enum class ProcessStatus
{
    Running,
    Stopped,
    Terminated
};

struct ProcessInfo
{
    DWORD pid;
    std::string command;
    HANDLE handle;
    HANDLE thread;
    ProcessStatus status;
};

class ProcessManager
{
public:
    static PROCESS_INFORMATION pi;
    std::vector<ProcessInfo> backgroundProcesses;

    // Tiến trình foreground
    bool execute_foreground(const char *command)
    {
        STARTUPINFOA si = {sizeof(si)};
        ZeroMemory(&pi, sizeof(pi));

        char cmdline[512];
        snprintf(cmdline, sizeof(cmdline), "cmd.exe /C %s", command);

        if (!CreateProcessA(
                NULL,
                cmdline,
                NULL,
                NULL,
                TRUE,
                0,
                NULL,
                NULL,
                &si,
                &pi))
        {
            std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
            return false;
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        if (pi.hProcess)
            CloseHandle(pi.hProcess);
        if (pi.hThread)
            CloseHandle(pi.hThread);
        pi.hProcess = NULL;
        pi.hThread = NULL;

        return true;
    }

    // Tiến trình background
    void execute_background(const char *command)
    {
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION local_pi = {};

        char cmdline[512];
        snprintf(cmdline, sizeof(cmdline), "cmd.exe /C %s", command);

        if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                            CREATE_NEW_CONSOLE, NULL, NULL, &si, &local_pi))
        {
            std::cerr << "Failed to start background process: " << GetLastError() << std::endl;
            return;
        }

        ProcessInfo info = {
            local_pi.dwProcessId,
            command,
            local_pi.hProcess,
            local_pi.hThread,
            ProcessStatus::Running};
        backgroundProcesses.push_back(info);
        std::cout << "Started background process: PID = " << local_pi.dwProcessId << "\n";
    }

    // Tìm tiến trình background theo PID
    ProcessInfo *findProcess(DWORD pid)
    {
        for (auto &proc : backgroundProcesses)
        {
            if (proc.pid == pid)
                return &proc;
        }
        return nullptr;
    }

    // Dừng (suspend) tiến trình background
    void stop_process(DWORD pid)
    {
        ProcessInfo *proc = findProcess(pid);
        if (!proc || proc->status != ProcessStatus::Running)
        {
            std::cerr << "Cannot stop process " << pid << ": Not found or not running.\n";
            return;
        }

        if (SuspendThread(proc->thread) == (DWORD)-1)
        {
            std::cerr << "Failed to suspend process " << pid << ".\n";
        }
        else
        {
            proc->status = ProcessStatus::Stopped;
            std::cout << "Suspended process " << pid << ".\n";
        }
    }

    // Tiếp tục (resume) tiến trình background
    void resume_process(DWORD pid)
    {
        ProcessInfo *proc = findProcess(pid);
        if (!proc || proc->status != ProcessStatus::Stopped)
        {
            std::cerr << "Cannot resume process " << pid << ": Not found or not stopped.\n";
            return;
        }

        if (ResumeThread(proc->thread) == (DWORD)-1)
        {
            std::cerr << "Failed to resume process " << pid << ".\n";
        }
        else
        {
            proc->status = ProcessStatus::Running;
            std::cout << "Resumed process " << pid << ".\n";
        }
    }

    // Kết thúc tiến trình background
    void kill_process(DWORD pid)
    {
        ProcessInfo *proc = findProcess(pid);
        if (!proc)
        {
            std::cerr << "Cannot kill process " << pid << ": Not found.\n";
            return;
        }

        if (!TerminateProcess(proc->handle, 0))
        {
            std::cerr << "Failed to terminate process " << pid << ".\n";
            return;
        }

        proc->status = ProcessStatus::Terminated;
        std::cout << "Terminated process " << pid << ".\n";
    }

    // Hiển thị danh sách tiến trình background
    void list_process()
    {
        std::cout << "\nBackground Processes:\n";
        std::cout << std::left
                  << std::setw(10) << "PID"
                  << std::setw(15) << "Status"
                  << "Command" << "\n";
        std::cout << std::string(50, '-') << "\n";

        std::vector<ProcessInfo> activeProcesses;

        for (auto &proc : backgroundProcesses)
        {
            DWORD waitCode = WaitForSingleObject(proc.handle, 0);
            if (waitCode == WAIT_OBJECT_0)
            {
                proc.status = ProcessStatus::Terminated;
            }

            std::string statusStr;
            switch (proc.status)
            {
            case ProcessStatus::Running:
                statusStr = "Running";
                break;
            case ProcessStatus::Stopped:
                statusStr = "Stopped";
                break;
            case ProcessStatus::Terminated:
                statusStr = "Terminated";
                break;
            }

            std::cout << std::left
                      << std::setw(10) << proc.pid
                      << std::setw(15) << statusStr
                      << proc.command << "\n";

            if (proc.status != ProcessStatus::Terminated)
            {
                activeProcesses.push_back(proc);
            }
            else
            {
                if (proc.handle)
                    CloseHandle(proc.handle);
                if (proc.thread)
                    CloseHandle(proc.thread);
            }
        }

        backgroundProcesses = std::move(activeProcesses);
    }
};

// Định nghĩa biến static
PROCESS_INFORMATION ProcessManager::pi = {};
