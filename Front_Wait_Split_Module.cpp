#include "Front_Wait_Split_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <utility>

extern SP_Module * write2buf_module;

Front_Wait_Split_Module::Front_Wait_Split_Module(int threads)
    :threads_num_(threads)
{
}


Front_Wait_Split_Module::~Front_Wait_Split_Module()
{
}


int
Front_Wait_Split_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Front_Wait_Split_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    Front_CRequest * next_data = NULL;
    SP_Message_Block_Base *next_msg = NULL;
    size_t end = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        Front_CRequest * data = NULL;
        data = reinterpret_cast<Front_CRequest *>(msg->data());
        data->request_->lock_.lock();
        ++data->request_->count_;
        data->request_->lock_.unlock();
        end = data->request_->end_;
        SP_DEBUG("count(%d),size_split_buf(%d),end(%d),length(%d)\n", data->request_->count_,data->request_->size_split_buf,end,data->request_->length_);
        if (data->request_->is_read_end_)
        {
            data->request_->send_split_count_++;
            put_next(msg);
            if (data->request_->count_ == data->request_->size_split_buf)
                data->request_->is_split_end_ = true;
        } else
        {
            SP_DES(msg);
            SP_DES(data);
        }
        gettimeofday(&t2,0);
        SP_DEBUG("Front_Wait_Split_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Front_Wait_Split_Module::init()
{
    return 0;
}

