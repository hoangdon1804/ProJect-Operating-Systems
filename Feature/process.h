#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <tlhelp32.h>
#include <map>
#include <utility>
#include <signal.h>
#include <sstream>
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
    std::map<DWORD, ProcessInfo> backgroundProcesses;

    void cleanupProcessHandles(ProcessInfo &proc)
    {
        if (proc.handle)
            CloseHandle(proc.handle);
        if (proc.thread)
            CloseHandle(proc.thread);
        proc.handle = nullptr;
        proc.thread = nullptr;
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

    void execute_background(const std::vector<std::string> &args)
    {
        if (args.size() < 1)
        {
            std::cout << "Usage: start <executable_path> [arguments...]" << std::endl;
            return;
        }

        std::string command = args[0];
        for (size_t i = 1; i < args.size(); ++i)
        {
            command += " " + args[i];
        }

        STARTUPINFOA si = {sizeof(si)};
        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        char *cmd = new char[command.length() + 1];
        strcpy(cmd, command.c_str());

        if (CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        {
            ProcessInfo procInfo;
            procInfo.pid = pi.dwProcessId;
            procInfo.command = command;
            procInfo.handle = pi.hProcess;
            procInfo.thread = pi.hThread;
            procInfo.status = ProcessStatus::Running;
            backgroundProcesses[pi.dwProcessId] = procInfo;
            std::cout << "Started process with PID: " << pi.dwProcessId << std::endl;
        }
        else
        {
            std::cerr << "Failed to start process: " << GetLastError() << std::endl;
            delete[] cmd; // Giải phóng bộ nhớ
            return;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        delete[] cmd; // Giải phóng bộ nhớ
    }

    ProcessInfo *findProcess(DWORD pid)
    {
        auto it = backgroundProcesses.find(pid);
        return (it != backgroundProcesses.end()) ? &it->second : nullptr;
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

        int suspendedThreads = 0;
        if (Thread32First(hSnapshot, &te))
        {
            do
            {
                if (te.th32OwnerProcessID == pid)
                {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                    if (hThread)
                    {
                        if (SuspendThread(hThread) != (DWORD)-1)
                        {
                            suspendedThreads++;
                        }
                        CloseHandle(hThread);
                    }
                    else
                    {
                        std::cerr << "Failed to open thread " << te.th32ThreadID
                                  << ": " << GetLastError() << std::endl;
                    }
                }
            } while (Thread32Next(hSnapshot, &te));
        }
        else
        {
            std::cerr << "Thread32First failed: " << GetLastError() << std::endl;
        }

        CloseHandle(hSnapshot);

        if (suspendedThreads > 0)
        {
            std::cout << "[Shell] Suspended " << suspendedThreads
                      << " thread(s) of process " << pid << ".\n";
            ProcessInfo *proc = findProcess(pid);
            if (proc)
            {
                proc->status = ProcessStatus::Stopped;
            }
        }
        else
        {
            std::cerr << "[Shell] Failed to suspend any thread in process " << pid << ".\n";
        }
    }

    void resume_process(DWORD pid)
    {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
        {
            std::cerr << "CreateToolhelp32Snapshot failed: " << GetLastError() << std::endl;
            return;
        }

        THREADENTRY32 te;
        te.dwSize = sizeof(THREADENTRY32);

        int resumedThreads = 0;
        if (Thread32First(hSnapshot, &te))
        {
            do
            {
                if (te.th32OwnerProcessID == pid)
                {
                    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                    if (hThread)
                    {
                        if (ResumeThread(hThread) != (DWORD)-1)
                        {
                            resumedThreads++;
                        }
                        CloseHandle(hThread);
                    }
                    else
                    {
                        std::cerr << "Failed to open thread " << te.th32ThreadID
                                  << ": " << GetLastError() << std::endl;
                    }
                }
            } while (Thread32Next(hSnapshot, &te));
        }
        else
        {
            std::cerr << "Thread32First failed: " << GetLastError() << std::endl;
        }

        CloseHandle(hSnapshot);

        if (resumedThreads > 0)
        {
            std::cout << "[Shell] Resumed " << resumedThreads
                      << " thread(s) of process " << pid << ".\n";
            ProcessInfo *proc = findProcess(pid);
            if (proc)
            {
                proc->status = ProcessStatus::Running;
            }
        }
        else
        {
            std::cerr << "[Shell] Failed to resume any thread in process " << pid << ".\n";
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

        std::vector<DWORD> toErase;

        for (auto &[pid, proc] : backgroundProcesses)
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

            if (proc.status == ProcessStatus::Terminated)
            {
                cleanupProcessHandles(proc);
                toErase.push_back(pid);
            }
        }

        for (DWORD pid : toErase)
        {
            backgroundProcesses.erase(pid);
        }
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

    void terminate_all_background()
    {
        for (const auto &proc : backgroundProcesses)
        {
            if (proc.second.status == ProcessStatus::Running)
            {
                kill_process(proc.second.pid);
            }
        }
    }
};