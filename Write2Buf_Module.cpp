#include "Write2Buf_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <algorithm>
#include <cstring>

Write2Buf_Module::Write2Buf_Module(int threads)
    :threads_num_(threads)
{
}


Write2Buf_Module::~Write2Buf_Module()
{
}


int
Write2Buf_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Write2Buf_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    CRequest * c_data = NULL;
    uint8_t idx, idx1;
    SP_Message_Block_Base *msg_mem_pool;
    Buffer_Element *buf;
    size_t i, j, k;
    size_t wt_length;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<CRequest *>(msg->data());
        idx = c_data->idx_[0];
        //idx1 = c_data->idx_[1];
        if (!mem_pool_wt.is_empty())
        {
            mem_pool_wt.dequeue(msg_mem_pool);
            buf = reinterpret_cast<Buffer_Element *>(msg_mem_pool->data());
            SP_DES(msg_mem_pool);
        } else
            buf = new Buffer_Element(BUFFER_ELEMENT_SIZE);
        for(idx1=0;idx1<26;++idx1)
        {
            for (i = 0; i < c_data->request_->double_str_count_[idx][idx1]; ++i)
            {
                if ((buf->wt_pos + 130) > buf->length)
                {
                    c_data->vec_buf_wt_.push_back(buf);
                    if (!mem_pool_wt.is_empty())
                    {
                        mem_pool_wt.dequeue(msg_mem_pool);
                        buf = reinterpret_cast<Buffer_Element *>(msg_mem_pool->data());
                        SP_DES(msg_mem_pool);
                    } else
                        buf = new Buffer_Element(BUFFER_ELEMENT_SIZE);
                }
                *(buf->ptr + buf->wt_pos++) = 'a' + idx;
                *(buf->ptr + buf->wt_pos++) = 'a' + idx1;
                *(buf->ptr + buf->wt_pos++) = '\n';
            }
            for (i = 0; i < 26; ++i)
            {
                if (c_data->request_->str_list_[idx][idx1][i].empty())
                    continue;
                for (j = 0; j < c_data->request_->str_list_[idx][idx1][i].size(); ++j)
                {
                    if ((buf->wt_pos + 130) > buf->length)
                    {
                        c_data->vec_buf_wt_.push_back(buf);
                        if (!mem_pool_wt.is_empty())
                        {
                            mem_pool_wt.dequeue(msg_mem_pool);
                            buf = reinterpret_cast<Buffer_Element *>(msg_mem_pool->data());
                            SP_DES(msg_mem_pool);
                        } else
                            buf = new Buffer_Element(BUFFER_ELEMENT_SIZE_SPLIT);
                    }
                    for (k = 0; k < 128; ++k)
                    {
                        if (*(c_data->request_->str_list_[idx][idx1][i][j] + k) == '\n')
                        {
                            break;
                        }
                    }
                    wt_length = k + 4;
                    memcpy(buf->ptr + buf->wt_pos, c_data->request_->str_list_[idx][idx1][i][j] - 3, wt_length);
                    buf->wt_pos += wt_length;
                }
                c_data->request_->str_list_[idx][idx1][i].clear();
            }
        }
        c_data->vec_buf_wt_.push_back(buf);
        put_next(msg);
        gettimeofday(&t2,0);
        SP_DEBUG("Write2Buf_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Write2Buf_Module::init()
{
    return 0;
}

