// LocalClock.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <map>

std::map<std::string, std::string> ParseCommandLine(int argc, char ** argv)
{
    std::map<std::string, std::string> argPairs;
    std::string argName;
    std::string argValue;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' || argv[i][0] == '/')
        {
            argName = argv[i] + 1;
            for (auto & c : argName)
            {
                c = tolower(c);
            }

        }
        else if (argName.length() > 0)
        {
            argValue = std::string(argv[i]);
            for (auto & c : argValue)
            {
                c = tolower(c);
            }
            argPairs.insert(std::make_pair(argName, argValue));
            argName.clear();
        }
    }
    return argPairs;
}


int main(int argc, char ** argv)
{
    // Parse the command line
    std::map<std::string, std::string> args = ParseCommandLine(argc, argv);

    if (args.find("interval") == args.end())
    {
        fprintf(stderr, "usage: %s -interval <seconds>\n", argv[0]);
        exit(-1);
    }

    int interval = atoi(args["interval"].c_str());

    for (
        auto endTime = std::chrono::steady_clock::now() + std::chrono::seconds(interval);
        endTime > std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(5000))
        )
    {
        auto tsStart = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        auto time = std::chrono::system_clock::now().time_since_epoch().count();
        auto tsEnd = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::cout << tsStart << "," << tsEnd << "," << time << std::endl;
    }
    
    return 0;
}

