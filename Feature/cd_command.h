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
    if (SetCurrentDirectoryA(path)) {
        std::cout << "Changed directory to: " << path << "\n";
    } else {
        std::cerr << "cd: failed to change directory to '" << path
                  << "'. Error code: " << GetLastError() << "\n";
    }
}
