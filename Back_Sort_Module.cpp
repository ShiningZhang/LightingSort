#include "Back_Sort_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <algorithm>
#include "pdqsort.h"

Back_Sort_Module::Back_Sort_Module(int threads)
    :threads_num_(threads)
{
}


Back_Sort_Module::~Back_Sort_Module()
{
}


int
Back_Sort_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Back_Sort_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    Back_CRequest * c_data = NULL;
    uint8_t idx, idx1;
    int i = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<Back_CRequest *>(msg->data());
        idx = c_data->idx_[0];
        //idx1 = c_data->idx_[1];
        for(idx1=0;idx1<26;++idx1)
        {
            //for (i = 0; i < 26; ++i)
            {
                if (c_data->request_->str_list_[idx][idx1].size() > 1)
                    pdqsort(c_data->request_->str_list_[idx][idx1].begin(), c_data->request_->str_list_[idx][idx1].end(), compare_char);
            }
        }
        put_next(msg);
        gettimeofday(&t2,0);
        SP_DEBUG("Back_Sort_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Back_Sort_Module::init()
{
    return 0;
}

