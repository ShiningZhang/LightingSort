#pragma once

#include "SP_Module.h"

#include <mutex>

class Write2File_Module : public SP_Module
{
public:
    Write2File_Module(int threads = 1);
    virtual ~Write2File_Module();
    virtual int open();
    virtual void svc();
    virtual int init();
private:
    int threads_num_;
    std::mutex lock_;
};
