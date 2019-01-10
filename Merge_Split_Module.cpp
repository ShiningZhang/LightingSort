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
        data = reinterpret_cast<Merge_CRequest *>(msg->data());
        uint8_t index = data->idx_[0].first;
        SP_LOGI("(%d)index(%d) begin\n", thread_num, index);
        for (int i = 0; i < data->idx_.size(); ++i)
        {
            begin = data->request_->vec_buf_[data->idx_[i].first][data->idx_[i].second]->rd_pos;
            end = begin;
            length = data->request_->vec_buf_[data->idx_[i].first][data->idx_[i].second]->wt_pos;
            buf = data->request_->vec_buf_[data->idx_[i].first][data->idx_[i].second]->ptr;
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
                    str_list[offset][offset1][offset2].emplace_back(std::make_pair(buf + begin + 3, index));
                }
                begin = ++end;
            }
            data->request_->vec_buf_[data->idx_[i].first][data->idx_[i].second]->rd_pos = end;
        }
        SP_LOGI("(%d)index(%d) mid\n", thread_num, index);
        for (size_t i = 0; i < 26; ++i)
        {
            data->request_->lock_single_str_[i].lock();
            data->request_->single_str_count_[i] += single_str_count[i];
            data->request_->lock_single_str_[i].unlock();
            single_str_count[i] = 0;
            for (size_t j = 0; j < 26; ++j)
            {
                data->request_->lock_double_str_[i][j].lock();
                data->request_->double_str_count_[i][j] += double_str_count[i][j];
                data->request_->lock_double_str_[i][j].unlock();
                double_str_count[i][j] = 0;
                for (size_t k = 0; k < 26; ++k)
                {
                    if (str_list[i][j][k].empty())
                        continue;
                    data->request_->lock_str_list_[i][j][k].lock();
                    if (data->request_->str_list_[i][j][k].empty())
                        data->request_->str_list_[i][j][k] = str_list[i][j][k];
                    else
                    {
                        temp_vec = data->request_->str_list_[i][j][k];
                        length = data->request_->str_list_[i][j][k].size() + str_list[i][j][k].size();
                        data->request_->str_list_[i][j][k].resize(length);
                        merge(str_list[i][j][k].begin(), str_list[i][j][k].end(),
                                temp_vec.begin(), temp_vec.end(),
                                data->request_->str_list_[i][j][k].begin(), compare_pair);
                    }
                    data->request_->lock_str_list_[i][j][k].unlock();
                    data->request_->lock_.lock();
                    SP_LOGI("2\n");
                    data->request_->vec_char_size_[i][j][index] += str_list[i][j][k].size();
                    SP_LOGI("3\n");
                    data->request_->lock_.unlock();
                    str_list[i][j][k].clear();
                   
                }
            }
        }
        SP_LOGI("(%d)index(%d) end\n", thread_num, index);
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

