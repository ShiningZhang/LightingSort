#include "Merge_Write2Buf_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <algorithm>
#include <cstring>
#include <tuple>

Merge_Write2Buf_Module::Merge_Write2Buf_Module(int threads)
    :threads_num_(threads)
{
}


Merge_Write2Buf_Module::~Merge_Write2Buf_Module()
{
}


int
Merge_Write2Buf_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Merge_Write2Buf_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    Merge_CRequest * c_data = NULL;
    Merge_Request * data = NULL;
    uint8_t idx, idx1;
    SP_Message_Block_Base *msg_mem_pool;
    Buffer_Element *buf;
    size_t i, j, k;
    size_t wt_length;
    bool is_loop = true;
    char * last_ptr[32];
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<Merge_CRequest *>(msg->data());
        idx = c_data->idx_[0].first;
        idx1 = c_data->idx_[0].second;
        is_loop = true;
        data = c_data->request_;
        memset(last_ptr, 0, sizeof(last_ptr));
        if (!mem_pool_wt.is_empty())
        {
            mem_pool_wt.dequeue(msg_mem_pool);
            buf = reinterpret_cast<Buffer_Element *>(msg_mem_pool->data());
            SP_DES(msg_mem_pool);
        } else
            buf = new Buffer_Element(BUFFER_ELEMENT_SIZE);
        //for(idx1=0;idx1<26;++idx1)
        {
            for (i = 0; i < data->double_str_count_[idx][idx1]; ++i)
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
            data->double_str_count_[idx][idx1] = 0;
            for (i = 0; i < 26 && is_loop; ++i)
            {
                if (data->str_list_[idx][idx1][i].empty())
                    continue;
                for (j = 0; j < data->str_list_[idx][idx1][i].size() && is_loop; ++j)
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
                    for (k = 0; k < 128; ++k)
                    {
                        if (*(data->str_list_[idx][idx1][i][j].first + k) == '\n')
                        {
                            break;
                        }
                    }
                    wt_length = k + 4;
                    memcpy(buf->ptr + buf->wt_pos, data->str_list_[idx][idx1][i][j].first - 3, wt_length);
                    buf->wt_pos += wt_length;
                    --data->vec_char_size_[idx][idx1][data->str_list_[idx][idx1][i][j].second];
                    last_ptr[data->str_list_[idx][idx1][i][j].second] = data->str_list_[idx][idx1][i][j].first;
                    if (data->vec_char_size_[idx][idx1][data->str_list_[idx][idx1][i][j].second] == 0)
                    {
                        if (data->last_idx_[0] == idx && data->last_idx_[1] == idx1)
                        {
                            is_loop = false;
                            break;
                        }
                    }
                }
                if (j != data->str_list_[idx][idx1][i].size())
                {
                    data->str_list_[idx][idx1][i].erase(data->str_list_[idx][idx1][i].begin(),
                                                        data->str_list_[idx][idx1][i].begin() + j +1);
                } else
                {
                    MERGE_TEMPLIST().swap(data->str_list_[idx][idx1][i]);
                    //data->str_list_[idx][idx1][i].clear();
                }
                if (data->str_list_[idx][idx1][i].size()!= 0)
                {
                    SP_LOGI("(%d,%d,%d;size(%d)\n", idx,idx1,i,data->str_list_[idx][idx1][i].size());
                    for(int k=0;k<2;++k)
                        SP_LOGI("(%d,%d,%d)(%d)size(%d)\n",idx,idx1,i,k,data->vec_char_size_[idx][idx1][k]);
                }
            }
        }
        c_data->vec_buf_wt_.push_back(buf);
        for (k = 0; k < data->vec_last_idx_.size(); ++k)
        {
            data->lock_.lock();
            if (last_ptr[k] != NULL && std::tie(idx,idx1) > std::tie(data->vec_last_ptr_[k].i, data->vec_last_ptr_[k].j))
            {
                data->vec_last_ptr_[k].i = idx;
                data->vec_last_ptr_[k].j = idx1;
                data->vec_last_ptr_[k].ptr = last_ptr[k];
            }
            data->lock_.unlock();
        }
        if (i!=26)
        {
            SP_LOGI("put buf (%d,%d,%d)\n", idx,idx1,i);
        }
        SP_DEBUG("put buf size(%d) idx(%d,%d)\n",c_data->vec_buf_wt_.size(),idx,idx1);
        put_next(msg);
        gettimeofday(&t2,0);
        SP_DEBUG("Merge_Write2Buf_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Merge_Write2Buf_Module::init()
{
    return 0;
}

