#include "Back_ReadFile_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"
#include <string.h>
#include <math.h>

Back_ReadFile_Module::Back_ReadFile_Module(int threads)
    :threads_num_(threads)
{
}


Back_ReadFile_Module::~Back_ReadFile_Module()
{
}


int
Back_ReadFile_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Back_ReadFile_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    size_t begin, end, length, wt_begin;
    char * buf;
    Back_Request * data = NULL;
    Back_CRequest * c_data = NULL;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        data = reinterpret_cast<Back_Request *>(msg->data());
        SP_DES(msg);
        begin = data->begin_;
        end = data->end_;
        length = data->length_;
        size_t line_size = length < 1024*6 ? length : ceil((double)length / 6);
        buf = data->buffer_;
        wt_begin = begin;
        if (length > MAX_BACK_IN_SIZE)
        {
            SP_LOGE("Read file failed! File size(%lld) > buf(%lld)\n", length, MAX_BACK_IN_SIZE);
            exit(1);
        }
        while (wt_begin < length)
        {
            if (wt_begin + line_size > length)
            {
                line_size = length - wt_begin;
            }
            size_t result = fread (buf + wt_begin,1,line_size,data->fp_in_);
            if (result != line_size)
            {
                if (data->idx_.size() == 2)
                    SP_LOGE("Back_ReadFile_Module:Read file failed!(%d,%d)line_size=%d,length=%d,wt_begin=%d\n",
                        data->idx_[0], data->idx_[1],line_size,length,wt_begin);
                else if (data->idx_.size() == 4)
                    SP_LOGE("Back_ReadFile_Module:Read file failed!(%d,%d,%d,%d)line_size=%d,length=%d,wt_begin=%d\n",
                        data->idx_[0], data->idx_[1],data->idx_[2], data->idx_[3],line_size,length,wt_begin);
                else
                    SP_LOGE("Back_ReadFile_Module:Read file failed! idx.size=%d\n", data->idx_.size());
                //fseek (data->fp_in_, -result, SEEK_CUR);
                exit(1);
            }
            end = wt_begin + line_size;
            SP_DEBUG("wt_begin(%zu),line_size(%zu),length(%zu), end(%zu), data->end(%zu)\n", wt_begin,line_size,length,end,data->end_);
            wt_begin = wt_begin + line_size;
            SP_NEW(c_data, Back_CRequest(data));
            c_data->buffer_ = data->buffer_;
            if (end != 0)
            {
                if (end != length)
                {
                    while(end != 0 && *(c_data->buffer_ + --end) != '\n');
                    if (end != 0)
                        ++end;
                } else
                {
                    while(end != 0 && *(c_data->buffer_ + --end) != '\n');
                    if (end != 0)
                        ++end;
                    fseek (data->fp_in_, -(length - end), SEEK_CUR);
                    data->length_ = end;
                }
            }
            c_data->begin_ = begin;
            c_data->end_ = end;
            SP_DEBUG("begin(%zu),end(%zu)\n",begin,end);
            begin = end;
            SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)c_data));
            ++data->size_split_buf;
            put_next(msg);
        }
        data->is_read_end_ = true;
        gettimeofday(&t2,0);
        SP_DEBUG("Back_ReadFile_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
        //SP_LOGI("Back_ReadFile_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Back_ReadFile_Module::init()
{
    return 0;
}

