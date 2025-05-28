#pragma once
#include <windows.h>
#include <iostream>
#include "process.h"

// Khai báo biến toàn cục lưu tiến trình foreground hiện tại
extern PROCESS_INFORMATION foregroundProcess;

BOOL WINAPI ConsoleCtrlHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT)
    {
        if (foregroundProcess.hProcess != NULL)
        {
            std::cout << "[!] Ctrl+C detected. Terminating foreground process (PID: "
                      << foregroundProcess.dwProcessId << ")..." << std::endl;

            TerminateProcess(foregroundProcess.hProcess, 1);

            if (foregroundProcess.hProcess)
                CloseHandle(foregroundProcess.hProcess);
            if (foregroundProcess.hThread)
                CloseHandle(foregroundProcess.hThread);
            foregroundProcess.hProcess = NULL;
            foregroundProcess.hThread = NULL;

            return TRUE;
        }
    }
    return FALSE;
}
