#pragma once

#include "SP_Module.h"

#include <mutex>

class Merge_Sort_Module : public SP_Module
{
public:
    Merge_Sort_Module(int threads = 1);
    virtual ~Merge_Sort_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
