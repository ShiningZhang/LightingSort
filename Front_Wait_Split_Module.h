#pragma once

#include "SP_Module.h"

#include <mutex>

class Front_Wait_Split_Module : public SP_Module
{
public:
    Front_Wait_Split_Module(int threads = 1);
    virtual ~Front_Wait_Split_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
