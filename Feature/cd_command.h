#pragma once
#include <windows.h>
#include <iostream>
#include <string>

// Hàm xử lý lệnh cd trong shell
inline void handle_cd(const char* path) {
    if (path == nullptr || *path == '\0') {
        std::cerr << "cd: missing argument\n";
        return;
    }

    std::string newPath;
    if (strcmp(path, "..") == 0) {
        // Lấy thư mục hiện tại
        char currentDir[MAX_PATH];
        if (GetCurrentDirectoryA(MAX_PATH, currentDir)) {
            std::string current(currentDir);
            size_t pos = current.find_last_of("\\/");
            if (pos != std::string::npos) {
                newPath = current.substr(0, pos);
            } else {
                newPath = current; // Đã ở thư mục gốc
            }
        } else {
            std::cerr << "cd: failed to get current directory. Error: " << GetLastError() << "\n";
            return;
        }
    } else {
        newPath = path;
    }

    if (SetCurrentDirectoryA(newPath.c_str())) {
        // Lấy lại đường dẫn đầy đủ sau khi chuyển thành công
        char fullPath[MAX_PATH];
        if (GetCurrentDirectoryA(MAX_PATH, fullPath)) {
            std::cout << "Changed directory to: " << fullPath << "\n";
        } else {
            std::cout << "Directory changed, but failed to retrieve full path.\n";
        }
    } else {
        std::cerr << "cd: failed to change directory to '" << newPath
                  << "'. Error code: " << GetLastError() << "\n";
    }
}
