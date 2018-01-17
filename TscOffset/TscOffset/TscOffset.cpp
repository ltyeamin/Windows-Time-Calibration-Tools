// TscOffset.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <intrin.h>
#include <thread>


struct Message {
    unsigned long long ClientTx;
    unsigned long long ClientRx;
    unsigned long long ServerRx;
    unsigned long long ServerTx;
};

void Server(size_t CpuId, Message * msg)
{
    //SetThreadAffinity(CpuId);
    unsigned long long oldClientTx = 0;
    for (;;)
    {
        unsigned int i;
        unsigned long long ts = __rdtscp(&i);
        unsigned long long currentClientTx = msg->ClientTx;
        if (currentClientTx == oldClientTx)
        {
            continue;
        }
        oldClientTx = currentClientTx;
        msg->ServerRx = ts;
        msg->ServerTx = __rdtscp(&i);
    }
}

int main()
{
    return 0;
}

