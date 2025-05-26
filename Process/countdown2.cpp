#include <stdio.h>
#include "unistd.h"
#include <string.h>
#include <windows.h>
#include <iostream>

int main()
{
    int a = 10000;
    for(int i = a; i > 0; i--)
    {
        printf("%d\n", i);
        Sleep(1000); // Sleep for 1 second
    }
}