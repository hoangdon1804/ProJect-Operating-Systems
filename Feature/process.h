#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <tlhelp32.h>
extern PROCESS_INFORMATION foregroundProcess;
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
    std::vector<ProcessInfo> backgroundProcesses;

    void cleanupProcessHandles(ProcessInfo &proc)
    {
        if (proc.handle)
            CloseHandle(proc.handle);
        if (proc.thread)
            CloseHandle(proc.thread);
        proc.handle = nullptr;
        proc.thread = nullptr;
    }

    bool resumeAllThreads(DWORD pid)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
            return false;

        THREADENTRY32 te32 = {sizeof(THREADENTRY32)};
        bool success = false;

        if (Thread32First(hSnapshot, &te32))
        {
            do
            {
                if (te32.th32OwnerProcessID == pid)
                {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                    if (hThread)
                    {
                        if (ResumeThread(hThread) != (DWORD)-1)
                        {
                            success = true;
                        }
                        else
                        {
                            std::cerr << "Failed to resume thread " << te32.th32ThreadID << "\n";
                        }
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(hSnapshot, &te32));
        }

        CloseHandle(hSnapshot);
        return success;
    }

    bool execute_foreground(const char *command)
    {
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi = {};
        char cmdline[512];
        snprintf(cmdline, sizeof(cmdline), "cmd.exe /C \"%s\"", command);

        if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            std::cerr << "CreateProcess failed: " << GetLastError() << std::endl;
            return false;
        }

        // Cập nhật foregroundProcess trước khi chờ
        foregroundProcess = pi;

        WaitForSingleObject(pi.hProcess, INFINITE);

        // Sau khi tiến trình kết thúc, giải phóng handle và đặt lại
        if (foregroundProcess.hProcess == pi.hProcess)
        {
            CloseHandle(foregroundProcess.hProcess);
            CloseHandle(foregroundProcess.hThread);
            foregroundProcess.hProcess = NULL;
            foregroundProcess.hThread = NULL;
        }

        waitForChildProcesses(pi.dwProcessId);
        return true;
    }

    void execute_background(const char *command)
    {
        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi = {};
        char cmdline[512];
        snprintf(cmdline, sizeof(cmdline), "cmd.exe /C \"%s\"", command);

        if (!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE,
                            CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        {
            std::cerr << "Failed to start background process: " << GetLastError() << std::endl;
            return;
        }

        ProcessInfo info = {
            pi.dwProcessId,
            command,
            pi.hProcess,
            pi.hThread,
            ProcessStatus::Running};

        backgroundProcesses.push_back(info);
        std::cout << "Started background process: PID = " << pi.dwProcessId << "\n";
    }

    ProcessInfo *findProcess(DWORD pid)
    {
        for (auto &proc : backgroundProcesses)
        {
            if (proc.pid == pid)
                return &proc;
        }
        return nullptr;
    }

    void stop_process(DWORD pid)
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

    void resume_process(DWORD pid)
    {
        ProcessInfo *proc = findProcess(pid);
        if (!proc || proc->status != ProcessStatus::Stopped)
        {
            std::cerr << "Cannot resume process " << pid << ": Not found or not stopped.\n";
            return;
        }

        if (resumeAllThreads(pid))
        {
            proc->status = ProcessStatus::Running;
            std::cout << "Resumed all threads of process " << pid << ".\n";
        }
        else
        {
            std::cerr << "Failed to resume all threads of process " << pid << ".\n";
        }
    }

    void kill_process(DWORD pid)
    {
        ProcessInfo *proc = findProcess(pid);
        if (!proc)
        {
            std::cerr << "Cannot kill process " << pid << ": Not found.\n";
            return;
        }

        std::vector<DWORD> childPIDs;
        findAllChildProcesses(pid, childPIDs);

        for (DWORD childPID : childPIDs)
        {
            HANDLE hChild = OpenProcess(PROCESS_TERMINATE, FALSE, childPID);
            if (hChild)
            {
                TerminateProcess(hChild, 0);
                std::cout << "Terminated child process: " << childPID << "\n";
                CloseHandle(hChild);
            }
        }

        TerminateProcess(proc->handle, 0);
        proc->status = ProcessStatus::Terminated;
        cleanupProcessHandles(*proc);
        std::cout << "Terminated process " << pid << ".\n";
    }

    void list_process()
    {
        std::cout << "\nBackground Processes:\n";
        std::cout << std::left
                  << std::setw(10) << "PID"
                  << std::setw(15) << "Status"
                  << "Command\n";
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
                activeProcesses.push_back(proc);
            else
                cleanupProcessHandles(proc);
        }

        backgroundProcesses = std::move(activeProcesses);
    }

    void waitForChildProcesses(DWORD parentPID)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
            return;

        PROCESSENTRY32 pe32 = {sizeof(pe32)};
        std::vector<DWORD> childPIDs;

        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (pe32.th32ParentProcessID == parentPID)
                    childPIDs.push_back(pe32.th32ProcessID);
            } while (Process32Next(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);

        for (DWORD pid : childPIDs)
        {
            HANDLE hChild = OpenProcess(SYNCHRONIZE, FALSE, pid);
            if (hChild)
            {
                WaitForSingleObject(hChild, INFINITE);
                CloseHandle(hChild);
                waitForChildProcesses(pid);
            }
        }
    }

    void findAllChildProcesses(DWORD parentPID, std::vector<DWORD> &childPIDs)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
            return;

        PROCESSENTRY32 pe32 = {sizeof(pe32)};

        if (Process32First(hSnapshot, &pe32))
        {
            do
            {
                if (pe32.th32ParentProcessID == parentPID)
                {
                    DWORD childPID = pe32.th32ProcessID;
                    childPIDs.push_back(childPID);
                    findAllChildProcesses(childPID, childPIDs);
                }
            } while (Process32Next(hSnapshot, &pe32));
        }

        CloseHandle(hSnapshot);
    }
};