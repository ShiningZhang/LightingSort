#pragma once

#include "SP_Module.h"

#include <mutex>

class Sort_Module : public SP_Module
{
public:
    Sort_Module(int threads = 1);
    virtual ~Sort_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
