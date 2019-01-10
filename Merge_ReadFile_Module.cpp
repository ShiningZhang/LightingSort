#include "Merge_ReadFile_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"
#include <string.h>
#include <math.h>

Merge_ReadFile_Module::Merge_ReadFile_Module(int threads)
    :threads_num_(threads)
{
}


Merge_ReadFile_Module::~Merge_ReadFile_Module()
{
}


int
Merge_ReadFile_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Merge_ReadFile_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    size_t begin, end, length, wt_begin;
    char * buf;
    Merge_Request * data = NULL;
    Merge_CRequest * c_data = NULL;
    Buffer_Element *buf_element;
    size_t line_size = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        data = reinterpret_cast<Merge_Request *>(msg->data());
        SP_DES(msg);
        int inline_split_size = data->split_size_ < 12 ?  12 / data->split_size_ : 1;
        int inline_size, count;
        for (int i = 0; i < data->vec_mid_fp_.size(); ++i)
        {
            if (data->vec_mid_fp_[i].size_ == data->vec_mid_fp_[i].wt_pos)
                continue;
            inline_size = ceil((double)mem_pool_rd[i]->size() / inline_split_size);
            count = 0;
            SP_LOGI("i=%d,split_size_=%d,inline_split_size=%d,inline_size=%d,size=%d\n",
                i,data->split_size_,inline_split_size,inline_size,mem_pool_rd[i]->size());
            while(!mem_pool_rd[i]->is_empty())
            {
                mem_pool_rd[i]->dequeue(msg);
                buf_element = reinterpret_cast<Buffer_Element *>(msg->data());
                SP_DES(msg);
                line_size = buf_element->length + data->vec_mid_fp_[i].wt_pos < data->vec_mid_fp_[i].size_ ?
                                buf_element->length : data->vec_mid_fp_[i].size_ - data->vec_mid_fp_[i].wt_pos;
                size_t result = fread (buf_element->ptr,1,line_size,data->vec_mid_fp_[i].fp_);
                buf_element->wt_pos = line_size;
                end = buf_element->wt_pos;
                SP_LOGI("end(%d)\n", end);
                while(end!=0&&*(buf_element->ptr+ --end)!= '\n');
                if (end != 0)
                    ++end;
                data->vec_mid_fp_[i].wt_pos += line_size;
                data->vec_mid_fp_[i].wt_pos -= buf_element->wt_pos - end;
                fseek (data->vec_mid_fp_[i].fp_, -(buf_element->wt_pos - end), SEEK_CUR);
                buf_element->wt_pos = end;
                data->vec_buf_[i].emplace_back(buf_element);
                count++;
                SP_LOGI("count(%d),end(%d),wt_pos(%zu),size(%zu)\n",count,end,data->vec_mid_fp_[i].wt_pos,data->vec_mid_fp_[i].size_);
                if (data->vec_mid_fp_[i].wt_pos == data->vec_mid_fp_[i].size_)
                    break;
                if (count == inline_size)
                {
                    SP_NEW(c_data, Merge_CRequest(data));
                    for(int j =  data->vec_buf_[i].size() - count; j < data->vec_buf_[i].size(); ++j)
                        c_data->idx_.emplace_back(std::make_pair(i, j));
                    SP_LOGI("i=%d,count=%d,size=%d\n",i,count,c_data->idx_.size());
                    SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
                    ++data->size_split_buf;
                    put_next(msg);
                    count = 0;
                }
            }
            if (count != 0)
            {
                end = data->vec_buf_[i][data->vec_buf_[i].size()-1]->wt_pos;
                while(*(data->vec_buf_[i][data->vec_buf_[i].size()-1]->ptr+ --end)!= '\n');
                ++end;
                data->vec_mid_fp_[i].wt_pos -= data->vec_buf_[i][data->vec_buf_[i].size()-1]->wt_pos - end;
                fseek (data->vec_mid_fp_[i].fp_, -(data->vec_buf_[i][data->vec_buf_[i].size()-1]->wt_pos - end), SEEK_CUR);
                data->vec_buf_[i][data->vec_buf_[i].size()-1]->wt_pos = end;
                SP_NEW(c_data, Merge_CRequest(data));
                for(int j =  data->vec_buf_[i].size() - count; j < data->vec_buf_[i].size(); ++j)
                    c_data->idx_.emplace_back(std::make_pair(i, j));
                SP_LOGI("i=%d,count=%d,size=%d\n",i,count,c_data->idx_.size());
                SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
                ++data->size_split_buf;
                put_next(msg);
                count = 0;
            }
        }
        data->is_read_end_ = true;
        gettimeofday(&t2,0);
        SP_DEBUG("Merge_ReadFile_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
        SP_LOGI("Merge_ReadFile_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Merge_ReadFile_Module::init()
{
    return 0;
}

