#include "Back_Wait_Split_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <cstring>
#include <algorithm>

extern SP_Module * write2buf_module;

Back_Wait_Split_Module::Back_Wait_Split_Module(int threads)
    :threads_num_(threads)
{
}


Back_Wait_Split_Module::~Back_Wait_Split_Module()
{
}


int
Back_Wait_Split_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Back_Wait_Split_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    Back_CRequest * next_data = NULL;
    Back_Request * request = NULL;
    SP_Message_Block_Base *next_msg = NULL;
    size_t end = 0;
    bool recv_data[256];
    memset(recv_data, 0, sizeof(recv_data));
    int count = 0;
    uint8_t i,j;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        Back_CRequest * data = NULL;
        data = reinterpret_cast<Back_CRequest *>(msg->data());
        request = data->request_;
        data->request_->lock_.lock();
        ++data->request_->count_;
        data->request_->end_ = data->request_->end_ < data->end_ ? data->end_ : data->request_->end_;
        data->request_->lock_.unlock();
        end = data->request_->end_;
        //SP_LOGI("(%p)count(%d),size_split_buf(%d),end(%d),length(%d)\n",data->request_, data->request_->count_,data->request_->size_split_buf,end,data->request_->length_);
        if (data->request_->count_ == data->request_->size_split_buf && data->request_->is_read_end_)
        {
            data->request_->is_read_end_ = false;
            data->request_->lock_.lock();
            data->request_->count_ = 0;
            data->request_->lock_.unlock();
            i = std::find(s_vec_back_request.begin(), s_vec_back_request.end(), data->request_) - s_vec_back_request.begin();
            recv_data[i] = true;
            while(recv_data[count])
            {
                request = s_vec_back_request[count];
                for (i = 0; i < 26; ++i)
                {
                    //for (uint8_t j = 0; j < 26; ++j)
                    j=0;
                    for (j = 0; j < 26; ++j)
                    {
                        if (!request->str_list_[i][j].empty())
                            break;
                    }
                    if (j == 26)
                    {
                        request->send_str_list_[i][0] = false;
                        continue;
                    } else
                    {
                        j = 0;
                        request->send_str_list_[i][j] = true;
                        SP_NEW(next_data, Back_CRequest(request));
                        next_data->idx_.emplace_back(i);
                        next_data->idx_.emplace_back(j);
                        SP_NEW(next_msg, SP_Message_Block_Base((SP_Data_Block *)next_data));
                        put_next(next_msg);
                    }
                }
                count++;
            }
            if (count == s_vec_back_request.size())
            {
                count = 0;
                memset(recv_data, 0, sizeof(recv_data));
            }
        }
        {
            delete msg;
            delete data;
        }
        gettimeofday(&t2,0);
        SP_DEBUG("Back_Wait_Split_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Back_Wait_Split_Module::init()
{
    return 0;
}

