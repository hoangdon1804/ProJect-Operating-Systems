#ifndef PATH_H
#define PATH_H
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

// Hàm kiểm tra thư mục có tồn tại hay không
BOOL DirectoryExists(const char* path) {
    DWORD attrib = GetFileAttributesA(path); 
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
}

// Hàm xử lý lệnh "path"
void show_path() {
    // Lấy kích thước cần thiết cho PATH
    DWORD size = GetEnvironmentVariableA("PATH", nullptr, 0);
    if (size == 0) {
        printf("Error getting PATH size: %lu\n", GetLastError());
        return;
    }

    // Tạo bộ đệm để chứa PATH
    std::string path(size, '\0');
    DWORD result = GetEnvironmentVariableA("PATH", &path[0], size);
    if (result == 0) {
        printf("Error getting PATH: %lu\n", GetLastError());
        return;
    }

    // Xóa ký tự null ở cuối nếu có
    if (!path.empty() && path.back() == '\0') {
        path.pop_back();
    }

    // Tách PATH theo dấu ';' và in từng dòng
    std::stringstream ss(path);
    std::string item;
    std::cout << "Current PATH:\n";
    int index = 1;
    while (std::getline(ss, item, ';')) {
        if(item.empty()) continue; // Bỏ qua các mục rỗng
        std::cout << " [" << index++ << "] " << item << "\n";
    }
}

// Hàm xử lý lệnh "addpath"
bool add_path(const char* new_path) {
    // 1. Kiểm tra thư mục có tồn tại
    if (!DirectoryExists(new_path)) {
        printf("Error: Directory '%s' does not exist.\n", new_path);
        return false;
    }

    // 2. Lấy PATH hiện tại
    DWORD size = GetEnvironmentVariableA("PATH", nullptr, 0);
    if (size == 0) {
        printf("Error getting PATH size: %lu\n", GetLastError());
        return false;
    }

    std::string current_path(size, '\0');
    DWORD result = GetEnvironmentVariableA("PATH", &current_path[0], size);
    if (result == 0) {
        printf("Error getting PATH: %lu\n", GetLastError());
        return false;
    }

    if (!current_path.empty() && current_path.back() == '\0') {
        current_path.pop_back();
    }

    // 3. Kiểm tra xem new_path đã có trong PATH chưa (không phân biệt hoa thường)
    std::string path_lower = current_path;
    std::string new_path_lower = new_path;
    std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(), ::tolower);
    std::transform(new_path_lower.begin(), new_path_lower.end(), new_path_lower.begin(), ::tolower);

    if (path_lower.find(new_path_lower) != std::string::npos) {
        printf("'%s' is already in PATH.\n", new_path);
        return true;
    }

    // 4. Nối thêm new_path vào cuối PATH cũ
    std::string updated_path = current_path + ";" + new_path;

    // 5. Đặt biến PATH mới (tạm thời)
    if (!SetEnvironmentVariableA("PATH", updated_path.c_str())) {
        printf("Error setting PATH: %lu\n", GetLastError());
        return false;
    }

    printf("Added '%s' to PATH (temporary).\n", new_path);

    // 6. Ghi vào registry để thay đổi vĩnh viễn
    HKEY hKey;
    LONG regResult = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &hKey);
    if (regResult == ERROR_SUCCESS) {
        regResult = RegSetValueExA(hKey, "Path", 0, REG_EXPAND_SZ,
            (const BYTE*)updated_path.c_str(), (DWORD)(updated_path.size() + 1));
        if (regResult == ERROR_SUCCESS) {
            printf("PATH updated permanently in registry.\n");

            // Thông báo thay đổi cho hệ thống
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"Environment",
                SMTO_ABORTIFHUNG, 5000, nullptr);
        } else {
            printf("Error saving PATH to registry: %lu\n", regResult);
            RegCloseKey(hKey);
            return false;
        }
        RegCloseKey(hKey);
    } else {
        printf("Error opening registry key: %lu\n", regResult);
        return false;
    }

    return true;
}

// Hàm xử lý lệnh trong shell
void process_command(const char* command) {
    char cmd[256], arg[256];
    sscanf(command, "%s %s", cmd, arg);

    if (_stricmp(cmd, "path") == 0) {
        show_path();
    } else if (_stricmp(cmd, "addpath") == 0) {
        if (strlen(arg) == 0) {
            printf("Usage: addpath <directory>\n");
        } else {
            add_path(arg);
        }
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}
#endif // PATH_H
