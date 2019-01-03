#pragma once

#include "SP_Module.h"

#include <mutex>

class ReadFile_Module : public SP_Module
{
public:
    ReadFile_Module(int threads = 1);
    virtual ~ReadFile_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
