#include "Merge_Split_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <algorithm>
#include <memory.h>

Merge_Split_Module::Merge_Split_Module(int threads)
    :threads_num_(threads)
{
}


Merge_Split_Module::~Merge_Split_Module()
{
}


int
Merge_Split_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Merge_Split_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    size_t begin, end, length;
    uint8_t offset, offset1, offset2;
    char * buf;
    size_t single_str_count[26];
    size_t double_str_count[26][26];
    MERGE_TEMPLIST str_list[26][26][26];
    for (size_t i = 0; i < 26; ++i)
        for (size_t j = 0; j < 26; ++j)
            for (size_t k = 0; k < 26; ++k)
                str_list[i][j][k].reserve(1024 * 16);
    memset(single_str_count, 0, sizeof(single_str_count));
    memset(double_str_count, 0, sizeof(double_str_count));
    MERGE_TEMPLIST temp_vec;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        Merge_CRequest * data = NULL;
        Merge_Request * request = NULL;
        data = reinterpret_cast<Merge_CRequest *>(msg->data());
        request = data->request_;
        uint8_t index = data->idx_[0].first;
        SP_DEBUG("(%d)index(%d) begin\n", thread_num, index);
        for (int i = 0; i < data->idx_.size(); ++i)
        {
            begin = data->request_->vec_buf_[data->idx_[i].first][data->idx_[i].second]->rd_pos;
            end = begin;
            length = data->request_->vec_buf_[data->idx_[i].first][data->idx_[i].second]->wt_pos;
            buf = data->request_->vec_buf_[data->idx_[i].first][data->idx_[i].second]->ptr;
            SP_LOGI("Merge_Split_Module:begin(%zu),length(%zu)\n", begin, length);
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
                    ++(single_str_count[offset]);
                } else if (begin + 2 == end)
                {
                    offset = buf[begin]-'a';
                    offset1 = buf[begin+1]-'a';
                    ++(double_str_count[offset][offset1]);
                } else
                {
                    offset = buf[begin]-'a';
                    offset1 = buf[begin+1]-'a';
                    offset2 = buf[begin+2]-'a';
                    if (offset>25||offset1>25||offset2>25)
                    {
                        SP_LOGI("begin(%zu)end(%zu)length(%zu)\n",begin,end,length);
                        SP_LOGI("(%d,%d,%d)%s",offset,offset1,offset2,buf);
                    }
                    request->lock_str_list_[offset][offset1][offset2].lock();
                    request->str_list_[offset][offset1][offset2].emplace_back(std::make_pair(buf + begin + 3, index));
                    ++request->vec_char_size_[offset][offset1][index];
                    request->lock_str_list_[offset][offset1][offset2].unlock();
                }
                begin = ++end;
            }
            request->vec_buf_[data->idx_[i].first][data->idx_[i].second]->rd_pos = end;
        }
        for (size_t i = 0; i < 26; ++i)
        {
            request->lock_single_str_[i].lock();
            request->single_str_count_[i] += single_str_count[i];
            request->lock_single_str_[i].unlock();
            single_str_count[i] = 0;
            for (size_t j = 0; j < 26; ++j)
            {
                request->lock_double_str_[i][j].lock();
                request->double_str_count_[i][j] += double_str_count[i][j];
                request->lock_double_str_[i][j].unlock();
                double_str_count[i][j] = 0;
            }
        }
        SP_DEBUG("(%d)index(%d) end\n", thread_num, index);
        put_next(msg);
        gettimeofday(&t2,0);
        SP_DEBUG("split=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
        //SP_LOGI("split=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Merge_Split_Module::init()
{
    return 0;
}

