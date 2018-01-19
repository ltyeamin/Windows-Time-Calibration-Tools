// TscBroadcastTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <intrin.h>
#include <atomic>
#include <thread>
#include <vector>
#include <algorithm>
#include "platform.h"

void CollectSamples(std::atomic<bool> & Signal, bool Client, std::vector<unsigned long long> & Samples)
{
    unsigned int i;
    for (size_t index = 0; index < Samples.size(); index++)
    {
        while (Signal.load() != Client)
        {
        }
        unsigned long long ts = __rdtscp(&i);
        Signal.store(!Client);
        Samples[index] = ts;
    }
}

int main(int argc, char ** argv)
{
    size_t serverCpuId = atoi(argv[1]);
    size_t clientCpuId = atoi(argv[2]);
    size_t samples = atoi(argv[3]);
    __declspec(align(64)) std::vector<unsigned long long> tsClient(samples);
    __declspec(align(64))  std::vector<unsigned long long> tsServer(samples);
    __declspec(align(64)) std::atomic<bool> clientOwns;

    for (size_t i = 0; i < 10; i++)
    {
        clientOwns.store(false);
        auto client = std::thread([&tsClient, &clientOwns, samples, clientCpuId]() {
            SetThreadAffinity(clientCpuId);
            CollectSamples(clientOwns, true, tsClient);
        });
        auto server = std::thread([&tsServer, &clientOwns, samples, serverCpuId]() {
            SetThreadAffinity(serverCpuId);
            CollectSamples(clientOwns, false, tsServer);
        });
        client.join();
        server.join();

        std::vector<long long> offsets;
        long long avgOffset = 0;
        for (size_t i = 0; i < samples - 1; i++)
        {
            long long offset = (2 * (long long)tsClient[i] - (long long)tsServer[i] - (long long)tsServer[i + 1]) / 2;
            offsets.push_back(offset);
        }

        std::sort(offsets.begin(), offsets.end());
        printf("%I64i\n", offsets[offsets.size() / 2]);
    }
}