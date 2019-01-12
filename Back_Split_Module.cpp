#include "Back_Split_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

Back_Split_Module::Back_Split_Module(int threads)
    :threads_num_(threads)
{
}


Back_Split_Module::~Back_Split_Module()
{
}


int
Back_Split_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Back_Split_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    size_t begin, end, length;
    uint8_t offset, offset1, offset2;
    char * buf;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        Back_CRequest * data = NULL;
        data = reinterpret_cast<Back_CRequest *>(msg->data());
        begin = data->begin_;
        end = begin;
        length = data->end_;
        buf = data->buffer_;
        if (data != 0)
        {
            while(end < length)
            {
                if (buf[end] != '\n')
                {
                    ++end;
                    continue;
                }
                if (begin + 1 == end)
                {
                    offset = buf[begin]-'a';
                    data->request_->lock_single_str_[offset].lock();
                    ++(data->request_->single_str_count_[offset]);
                    data->request_->lock_single_str_[offset].unlock();
                } else
                {
                    offset = buf[begin]-'a';
                    offset1 = buf[begin+1]-'a';
                    data->request_->lock_str_list_[offset][offset1].lock();
                    data->request_->str_list_[offset][offset1].push_back(buf + begin + 2);
                    data->request_->lock_str_list_[offset][offset1].unlock();
                }
                begin = ++end;
            }
        }
        put_next(msg);
        gettimeofday(&t2,0);
        SP_DEBUG("split=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
        //SP_LOGI("split=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Back_Split_Module::init()
{
    return 0;
}

