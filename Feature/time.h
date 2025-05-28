#ifndef TIME_H
#define TIME_H

#include <windows.h>
#include <iostream>
#include <iomanip>

namespace ShellTime
{

    // Hàm tiện ích dùng chung
    SYSTEMTIME getSystemTime()
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        return st;
    }

    // Hiển thị ngày
    void showDate()
    {
        SYSTEMTIME st = getSystemTime();
        std::cout << "Current date: "
                  << std::setfill('0') << std::setw(2) << st.wDay << "/"
                  << std::setfill('0') << std::setw(2) << st.wMonth << "/"
                  << st.wYear << std::endl;
    }

    // Hiển thị giờ
    void showTime()
    {
        SYSTEMTIME st = getSystemTime();
        std::cout << "Current time: "
                  << std::setfill('0') << std::setw(2) << st.wHour << ":"
                  << std::setfill('0') << std::setw(2) << st.wMinute << ":"
                  << std::setfill('0') << std::setw(2) << st.wSecond << std::endl;
    }

    // Hiển thị cả ngày và giờ
    void showDateTime()
    {
        SYSTEMTIME st = getSystemTime();
        std::cout << "Current date and time: "
                  << std::setfill('0') << std::setw(2) << st.wDay << "/"
                  << std::setfill('0') << std::setw(2) << st.wMonth << "/"
                  << st.wYear << " "
                  << std::setfill('0') << std::setw(2) << st.wHour << ":"
                  << std::setfill('0') << std::setw(2) << st.wMinute << ":"
                  << std::setfill('0') << std::setw(2) << st.wSecond << std::endl;
    }

} // namespace ShellTime

#endif // TIME_H
