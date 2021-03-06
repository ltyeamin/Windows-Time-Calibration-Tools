// TscOffset.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <intrin.h>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include "platform.h"

typedef signed long long TTsc;
typedef std::pair<TTsc, TTsc> TSample;

volatile struct Message {
    enum eState {
        Idle = 0,
        ClientPrep = 1,
        ClientDone = 2,
        ServerPrep = 3,
        ServerDone = 4
    };

    __declspec(align(64)) struct {
        TTsc T1;
        TTsc T4;
    };
    __declspec(align(64)) struct {
        TTsc T2;
        TTsc T3;
    };
    __declspec(align(64))struct {
        std::atomic<size_t> State;
    };
} Msg = {};

void Server(size_t CpuId)
{
    SetThreadAffinity(CpuId);
    unsigned int i;
    for (;;)
    {
        size_t test = Message::ClientDone;
        if (!Msg.State.compare_exchange_weak(test, Message::ServerPrep))
        {
            continue;
        }
        Msg.T2 = __rdtscp(&i);
        Msg.T3 = __rdtscp(&i);
        Msg.State.exchange(Message::ServerDone);
    }
}

void Client(size_t CpuId, long RttBound, TTsc & Offset, TTsc & Rtt)
{
    SetThreadAffinity(CpuId);
    unsigned int i;
    for (;;)
    {
        size_t test = Message::Idle;
        if (!Msg.State.compare_exchange_weak(test, Message::ClientPrep))
        {
            continue;
        }
        Msg.T1 = __rdtscp(&i);
        Msg.State.exchange(Message::ClientDone);
        test = Message::ServerDone;
        while (!Msg.State.compare_exchange_weak(test, Message::Idle))
        {
            test = Message::ServerDone;
        }

        Msg.T4 = __rdtscp(&i);

        Offset = ((Msg.T2 - Msg.T1) + (Msg.T3 - Msg.T4)) / 2;
        Rtt = (Msg.T4 - Msg.T1) - (Msg.T3 - Msg.T2);

        if (Msg.T2 > Msg.T3)
        {
            continue;
        }

        if (Msg.T1 > Msg.T4)
        {
            continue;
        }

        if (Rtt > RttBound)
        {
            continue;
        }
        break;
    }
}


void ComputeStats(std::vector<TSample> & Samples, TSample & Median, TSample & StdDeviation)
{
    std::sort(Samples.begin(), Samples.end(), [](const TSample & a, TSample & b) -> bool {
        return a.first > b.first;
    });

    TSample average = { 0, 0 };
    TSample stdev = { 0, 0 };
    std::for_each(Samples.begin(), Samples.end(), [&average](const TSample & s) {
        average.first = average.first + s.first;
        average.second += s.second;
    });
    average.first /= static_cast<TTsc>(Samples.size());
    average.second /= static_cast<TTsc>(Samples.size());
    std::for_each(Samples.begin(), Samples.end(), [average, &stdev](const TSample & s) {
        TTsc deltaFirst = s.first - average.first;
        TTsc deltaSecond = s.second - average.second;
        TTsc deltaFirstSq = deltaFirst * deltaFirst;
        TTsc deltaSecondSq = deltaSecond * deltaSecond;
        stdev.first += deltaFirstSq;
        stdev.second += deltaSecondSq;
    });
    stdev.first /= static_cast<TTsc>(Samples.size());
    stdev.second /= static_cast<TTsc>(Samples.size());
    stdev.first = std::sqrt(stdev.first);
    stdev.second = std::sqrt(stdev.second);
    StdDeviation = stdev;
    Median = Samples[Samples.size() / 2];
}

int main(int argc, char ** argv)
{ 
    if (argc != 5)
    {
        printf("Usage: %s Server Client Iterations Cutoff\n", argv[0]);
        exit(-1);
    }

    int serverCpuId = atoi(argv[1]);
    int clientCpuId = atoi(argv[2]);
    int iterations = atoi(argv[3]);
    int rttBounds = atoi(argv[4]);
    std::vector<TSample> samples(iterations);
    

    printf("Offset\tRTT\tO-STDEV\tR-STDEV\n");

    std::thread t([&]() { Server(serverCpuId);  });
    for (size_t i = 0; i < 10; i++)
    {
        for (size_t j = 0; j < iterations; j++)
        {
            TTsc offset = 0;
            TTsc rtt = 0;
            Client(clientCpuId, rttBounds, offset, rtt);
            
            samples[j].first = offset;
            samples[j].second = rtt;
        }
        TSample median, stdev;
        ComputeStats(samples, median, stdev);

        printf("%lld\t%lld\t%lld\t%lld\n", median.first, median.second, stdev.first, stdev.second);
    }
    exit(0);
    return 0;
}

