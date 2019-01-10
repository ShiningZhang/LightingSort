#include "Merge_Sort_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <algorithm>
#include "pdqsort.h"

Merge_Sort_Module::Merge_Sort_Module(int threads)
    :threads_num_(threads)
{
}


Merge_Sort_Module::~Merge_Sort_Module()
{
}


int
Merge_Sort_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Merge_Sort_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    Merge_CRequest * c_data = NULL;
    uint8_t idx, idx1;
    int i = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<Merge_CRequest *>(msg->data());
        idx = c_data->idx_[0].first;
        idx1 = c_data->idx_[0].second;
        //for(idx1=0;idx1<26;++idx1)
        {
            for (i = 0; i < 26; ++i)
            {
                if (c_data->request_->str_list_[idx][idx1][i].size() > 1)
                    pdqsort(c_data->request_->str_list_[idx][idx1][i].begin(), c_data->request_->str_list_[idx][idx1][i].end(), compare_pair);
            }
        }
        put_next(msg);
        gettimeofday(&t2,0);
        SP_DEBUG("Merge_Sort_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Merge_Sort_Module::init()
{
    return 0;
}

