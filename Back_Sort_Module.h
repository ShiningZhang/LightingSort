#pragma once

#include "SP_Module.h"

#include <mutex>

class Back_Sort_Module : public SP_Module
{
public:
    Back_Sort_Module(int threads = 1);
    virtual ~Back_Sort_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
