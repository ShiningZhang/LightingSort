#include "Wait_Split_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

extern SP_Module * write2buf_module;

Wait_Split_Module::Wait_Split_Module(int threads)
    :threads_num_(threads)
{
}


Wait_Split_Module::~Wait_Split_Module()
{
}


int
Wait_Split_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Wait_Split_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    CRequest * next_data = NULL;
    SP_Message_Block_Base *next_msg = NULL;
    size_t end = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        CRequest * data = NULL;
        data = reinterpret_cast<CRequest *>(msg->data());
        data->request_->lock_.lock();
        ++data->request_->count_;
        data->request_->lock_.unlock();
        end = end < data->end_ ? data->end_ : end;
        if (data->request_->count_ == data->request_->size_split_buf && end == data->request_->length_)
        {
            data->request_->lock_.lock();
            data->request_->count_ = 0;
            data->request_->lock_.unlock();
            for (uint8_t i = 0; i < 26; ++i)
            {
                //for (uint8_t j = 0; j < 26; ++j)
                uint8_t j=0;
                {
                        ++data->request_->count_;
                        data->request_->send_str_list_[i][j] = true;
                        SP_NEW(next_data, CRequest(data->request_));
                        next_data->idx_.emplace_back(i);
                        next_data->idx_.emplace_back(j);
                        SP_NEW(next_msg, SP_Message_Block_Base((SP_Data_Block *)next_data));
                        put_next(next_msg);
                }
            }
            if (data->request_->count_ == 0)
            {
                SP_NEW(next_data, CRequest(data->request_));
                next_data->idx_.resize(2, 0);
                SP_NEW(next_msg, SP_Message_Block_Base((SP_Data_Block *)next_data));
                write2buf_module->put(next_msg);
            }
        }
        {
            SP_DES(msg);
            SP_DES(data);
        }
        gettimeofday(&t2,0);
        SP_DEBUG("Wait_Split_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Wait_Split_Module::init()
{
    return 0;
}

