#pragma once
#include <thread>
class test
{
    //Test³éÏóÀà
public:
    virtual bool initialize() = 0;
    virtual void run() = 0;
    virtual void shutdown() = 0;
};