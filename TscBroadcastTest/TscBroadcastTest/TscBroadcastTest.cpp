// TscBroadcastTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <intrin.h>
#include <atomic>
#include <thread>
#include <vector>
#include <algorithm>
#include "platform.h"

/*enum eStates {
    Idle,
    ServerReady,
    ClientReady,
    ServerSignalled,
    ClientAck,
    ClientDone,
    ServerDone
};

void WaitForState(std::atomic<size_t> & Signal, size_t OldState, size_t NewState)
{
    auto expected = OldState;
    while (!Signal.compare_exchange_weak(expected, NewState))
    {
        expected = OldState;
    }
}

void Server(size_t CpuId, std::atomic<size_t> & Signal, std::vector<unsigned long long> & Ts)
{
    SetThreadAffinity(CpuId);
    std::for_each(Ts.begin(), Ts.end(), [&Signal](unsigned long long & Now)
    {
        unsigned int i;
        unsigned long long val;
        WaitForState(Signal, Idle, ServerReady);
        WaitForState(Signal, ClientReady, ServerSignalled);
        val = __rdtscp(&i);
        WaitForState(Signal, ClientDone, ServerDone);
        Now = val;
        WaitForState(Signal, ServerDone, Idle);
    });
}

void Client(size_t CpuId, std::atomic<size_t> & Signal, std::vector<unsigned long long> & Ts)
{
    SetThreadAffinity(CpuId);
    std::for_each(Ts.begin(), Ts.end(), [&Signal](unsigned long long & Now)
    {
        unsigned int i;
        unsigned long long val;
        WaitForState(Signal, ServerReady, ClientReady);
        WaitForState(Signal, ServerSignalled, ClientAck);
        val = __rdtscp(&i);
        Now = val;
        WaitForState(Signal, ClientAck, ClientDone);
    });
}

int main()
{
    __declspec(align(64)) struct {
        std::atomic<size_t> signal;
    } b;
    b.signal.store(Idle);
    std::vector<unsigned long long> tsClient(1024);
    std::vector<unsigned long long> tsServer(1024);
    auto s = std::thread([&b, &tsServer]() { Server(1, b.signal, tsServer); });
    auto c = std::thread([&b, &tsClient]() { Client(0, b.signal, tsClient); });
    s.join();
    c.join();

    for (size_t i = 0; i < tsClient.size(); i++)
    {
        printf("%I64i\n", tsClient[i] - tsServer[i]);
    }

    return 0;
}

*/


int main()
{
    unsigned long long tsClient[65] = {};
    unsigned long long tsServer[65] = {};
    __declspec(align(64)) struct {
        std::atomic<size_t> index;
        std::atomic<bool> clientOwns;
    } b;
    b.index.store(0);
    b.clientOwns = 0;
    auto client = std::thread([&tsClient, &b]() {
        unsigned int i;
        for(;;)
        {
            while (b.clientOwns.load() == false)
            {
            }
            size_t index = b.index.load();
            if (index == 64)
            {
                break;
            }
            tsClient[index] = __rdtscp(&i);
            b.clientOwns.store(false);
        }
    });
    auto server = std::thread([&tsServer, &b]() {
        unsigned int i;
        for (;;)
        {
            size_t index = b.index.load();
            if (index == 64)
            {
                break;
            }
            tsServer[index] = __rdtscp(&i);
            b.clientOwns.store(true);
            while (b.clientOwns.load() == true)
            {
            }
            b.index++;
        }
        b.clientOwns.store(true);
    });
    client.join();
    server.join();

    for (size_t i = 0; i < 63; i++)
    {
        printf("%I64i\t%I64i\t%I64i\n", tsClient[i], tsServer[i], 2 * tsClient[i] - tsServer[i] - tsServer[i+1]);
    }
}