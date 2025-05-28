#ifndef DIR_H
#define DIR_H

#include <windows.h>
#include <iostream>
#include <string>

void showCurrentDirectoryContents()
{
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;

    // Lấy thư mục hiện tại
    char currentPath[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentPath);
    std::string searchPath = std::string(currentPath) + "\\*";

    hFind = FindFirstFileA(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::cerr << "[Shell] Failed to list directory contents.\n";
        return;
    }

    std::cout << "[Shell] Listing directory: " << currentPath << "\n";
    do
    {
        std::string name = findFileData.cFileName;
        if (name == "." || name == "..") continue;

        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            std::cout << "<DIR>   ";
        }
        else
        {
            std::cout << "        ";
        }

        std::cout << name << "\n";

    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

#endif // DIR_H
