#pragma once
#include <windows.h>
#include <iostream>
#include "process.h"

// Khai báo extern để dùng biến global manager từ main
extern ProcessManager manager;

BOOL WINAPI ConsoleCtrlHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
    {
        if (ProcessManager::pi.hProcess != NULL)
        {
            std::cout << "[!] Ctrl+C detected. Terminating foreground process (PID: "
                      << ProcessManager::pi.dwProcessId << ")..." << std::endl;

            TerminateProcess(ProcessManager::pi.hProcess, 1);

            if (ProcessManager::pi.hProcess)
                CloseHandle(ProcessManager::pi.hProcess);
            if (ProcessManager::pi.hThread)
                CloseHandle(ProcessManager::pi.hThread);
            ProcessManager::pi.hProcess = NULL;
            ProcessManager::pi.hThread = NULL;

            return TRUE;
        }
    }
    return FALSE;
}
