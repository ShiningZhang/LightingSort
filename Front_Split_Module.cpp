#include "Front_Split_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <cstring>

Front_Split_Module::Front_Split_Module(int threads)
    :threads_num_(threads)
{
}


Front_Split_Module::~Front_Split_Module()
{
}


int
Front_Split_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Front_Split_Module::svc()
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
        Front_CRequest * data = NULL;
        Front_Request * request = NULL;
        data = reinterpret_cast<Front_CRequest *>(msg->data());
        data->vec_buf_wt_.resize(26);
        for (int i = 0; i < 26; ++i)
        {
            for (int j = 0; j < 26; ++j)
                data->vec_buf_wt_[i].emplace_back(new Buffer_Element(BUFFER_ELEMENT_FRONT_SIZE));
        }
        request = data->request_;
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
                    request->lock_single_str_[offset].lock();
                    ++(request->single_str_count_[offset]);
                    request->lock_single_str_[offset].unlock();
                } else if (begin + 2 == end)
                {
                    offset = buf[begin]-'a';
                    offset1 = buf[begin+1]-'a';
                    request->lock_double_str_[offset][offset1].lock();
                    ++(request->double_str_count_[offset][offset1]);
                    request->lock_double_str_[offset][offset1].unlock();
                } else
                {
                    offset = buf[begin]-'a';
                    offset1 = buf[begin+1]-'a';
                    //offset2 = buf[begin+2]-'a';
                    if ((data->vec_buf_wt_[offset][offset1]->wt_pos + 130) > data->vec_buf_wt_[offset][offset1]->length)
                    {
                        request->lock_str_list_[offset][offset1][0].lock();
                        request->vec_buf_[offset][offset1].emplace_back(data->vec_buf_wt_[offset][offset1]);
                        request->lock_str_list_[offset][offset1][0].unlock();
                        data->vec_buf_wt_[offset][offset1] = new Buffer_Element(BUFFER_ELEMENT_FRONT_SIZE);
                    }
                    memcpy(data->vec_buf_wt_[offset][offset1]->ptr + data->vec_buf_wt_[offset][offset1]->wt_pos,
                            buf + begin, end - begin + 1);
                    data->vec_buf_wt_[offset][offset1]->wt_pos += end - begin + 1;
                }
                begin = ++end;
            }
        }
        for (int i = 0; i < 26; ++i)
        {
            for (int j = 0; j < 26; ++j)
            {
                if (data->vec_buf_wt_[i][j]->wt_pos != 0)
                {
                    request->lock_str_list_[i][j][0].lock();
                    request->vec_buf_[i][j].emplace_back(data->vec_buf_wt_[i][j]);
                    request->lock_str_list_[i][j][0].unlock();
                } else
                {
                    SP_DES(data->vec_buf_wt_[i][j]);
                }
            }
        }
        put_next(msg);
        gettimeofday(&t2,0);
        SP_DEBUG("split=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
        //SP_LOGI("split=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Front_Split_Module::init()
{
    return 0;
}

