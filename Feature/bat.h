#ifndef BAT_H
#define BAT_H

#include <windows.h>
#include <string>
#include <iostream>

inline void execute_bat(const std::string& batPath)
{
    // Câu lệnh cmd /c "file.bat" sẽ chạy file .bat và thoát cmd khi kết thúc
    std::string command = "cmd /c \"" + batPath + "\"";

    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);

    // Tạo tiến trình chạy file .bat
    if (!CreateProcessA(
            NULL,                   // ứng dụng
            command.data(),         // lệnh command line
            NULL,                   // security attributes process
            NULL,                   // security attributes thread
            FALSE,                  // inherit handles
            0,                      // flags
            NULL,                   // environment
            NULL,                   // current directory
            &si,
            &pi))
    {
        std::cerr << "[Shell] Failed to run batch file: " << batPath 
                  << ". Error code: " << GetLastError() << "\n";
        return;
    }

    // Đợi tiến trình hoàn thành
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Đóng handle
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#endif // BAT_H
