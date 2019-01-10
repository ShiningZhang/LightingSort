#include "Merge_Wait_Split_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <utility>
#include <tuple>

extern SP_Module * write2buf_module;

Merge_Wait_Split_Module::Merge_Wait_Split_Module(int threads)
    :threads_num_(threads)
{
}


Merge_Wait_Split_Module::~Merge_Wait_Split_Module()
{
}


int
Merge_Wait_Split_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Merge_Wait_Split_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    Merge_CRequest * next_data = NULL;
    SP_Message_Block_Base *next_msg = NULL;
    size_t end = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        Merge_CRequest * data = NULL;
        data = reinterpret_cast<Merge_CRequest *>(msg->data());
        data->request_->lock_.lock();
        ++data->request_->count_;
        data->request_->lock_.unlock();
        end = end < data->idx_.back().first ? data->idx_.back().first : end;
        SP_LOGI("count=%d,size_split_buf=%d\n", data->request_->count_, data->request_->size_split_buf);
        if (data->request_->count_ == data->request_->size_split_buf && data->request_->is_read_end_)
        {
            data->request_->lock_.lock();
            data->request_->count_ = 0;
            data->request_->is_read_end_ = false;
            data->request_->lock_.unlock();
            uint8_t idx = 26;
            uint8_t idx1 = 26;
            for (uint8_t i = 0; i < 26; ++i)
            {
                for (uint8_t j = 0; j < 26; ++j)
                {
                    for (int k = 0; k < data->request_->vec_mid_fp_.size(); ++k)
                    {
                        if (data->request_->vec_char_size_[i][j][k] != 0)
                        {
                            if (std::tie(i, j) > std::tie(data->request_->vec_last_idx_[k].first, data->request_->vec_last_idx_[k].second))
                            {
                                data->request_->vec_last_idx_[k] = std::make_pair(i, j);
                            }
                        }
                    }
                }
            }
            for (int k = 0; k < data->request_->vec_last_idx_.size(); ++k)
            {
                if (std::tie(idx, idx1) > std::tie(data->request_->vec_last_idx_[k].first, data->request_->vec_last_idx_[k].second))
                {
                    idx = data->request_->vec_last_idx_[k].first;
                    idx1 = data->request_->vec_last_idx_[k].second;
                }
            }
            SP_LOGI("(%d,%d)\n", idx,idx1);
            data->request_->last_idx_[0] = idx;
            data->request_->last_idx_[1] = idx1;
            for (uint8_t i = 0; i <= idx; ++i)
            {
                for (uint8_t j = 0; j <= (i==idx?idx1:25); ++j)
                {
                    for (uint8_t k = 0; k < 26; ++k)
                    {
                        if (data->request_->str_list_[i][j][k].empty())
                        {
                            continue;
                        }
                        ++data->request_->count_;
                        data->request_->send_str_list_[i][j] = true;
                        SP_NEW(next_data, Merge_CRequest(data->request_));
                        next_data->idx_.emplace_back(std::make_pair(i,j));
                        SP_NEW(next_msg, SP_Message_Block_Base((SP_Data_Block *)next_data));
                        put_next(next_msg);
                        break;
                    }
                }
            }
        }
        {
            SP_DES(msg);
            SP_DES(data);
        }
        gettimeofday(&t2,0);
        SP_DEBUG("Merge_Wait_Split_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Merge_Wait_Split_Module::init()
{
    return 0;
}

