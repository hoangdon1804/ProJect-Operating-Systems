#include <iostream>
#include <thread>
#include <chrono>

// Hàm cho luồng 1
void task1() {
    for (int i = 1; i <= 5; ++i) {
        std::cout << "[Task 1] Lần " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// Hàm cho luồng 2
void task2() {
    for (int i = 1; i <= 5; ++i) {
        std::cout << "[Task 2] Lần " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main() {
    std::cout << "Tiến trình bắt đầu\n";

    // Tạo hai luồng
    std::thread thread1(task1);
    std::thread thread2(task2);

    // Chờ hai luồng hoàn thành
    thread1.join();
    thread2.join();

    std::cout << "Tiến trình kết thúc\n";
    return 0;
}
