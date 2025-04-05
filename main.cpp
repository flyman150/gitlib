#include <chrono>
#include <iostream>
#include <thread>

#include "tcp_client.h"

int main()
{
    int a = 1;
    int b = 2;
    for (int i = 0; i < 10; i++)
    {
        if (i > 5)
        {
            a += b;
        }
    }
    return 0;
}
