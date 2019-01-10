#include "Merge_Write2File_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

Merge_Write2File_Module::Merge_Write2File_Module(int threads)
    :threads_num_(threads)
{
}


Merge_Write2File_Module::~Merge_Write2File_Module()
{
}


int
Merge_Write2File_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Merge_Write2File_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    uint8_t offset = 0;
    uint8_t offset1 = 0;
    Merge_Request * data = NULL;
    Merge_CRequest * c_data = NULL;
    Buffer_Element *buf = NULL;
    char single_str_buf[256];
    size_t i = 0;
    size_t write_count = 0;
    size_t single_count = 0;
    size_t single_write_count = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<Merge_CRequest *>(msg->data());
        data = c_data->request_;
        data->recv_str_list_[c_data->idx_[0].first][c_data->idx_[0].second] = c_data;
        SP_DES(msg);
        while (offset < 26)
        {
            if (offset1 == 0 && data->single_str_count_[offset] != 0)
            {
                size_t size = 0;
                size_t count = 0;
                single_count += data->single_str_count_[offset];
                for (i = 0; i < data->single_str_count_[offset] && size < 256; ++i)
                {
                    single_str_buf[size++] = 'a' + offset;
                    single_str_buf[size++] = '\n';
                    ++count;
                }
                single_write_count += fwrite(single_str_buf, sizeof(char), size, data->fp_out_);
                data->single_str_count_[offset] -= count;
                while(data->single_str_count_[offset] != 0)
                {
                    if (count < data->single_str_count_[offset])
                    {
                        single_write_count += fwrite(single_str_buf, sizeof(char), size, data->fp_out_);
                    } else
                    {
                        count = data->single_str_count_[offset];
                        single_write_count += fwrite(single_str_buf, sizeof(char), count * 2, data->fp_out_);
                    }
                    data->single_str_count_[offset] -= count;
                }
            }
            if (data->send_str_list_[offset][offset1])
            {
                if (data->recv_str_list_[offset][offset1] == NULL)
                    break;
            } else
            {
                
                ++offset1;
                if (offset1 == 26)
                {
                    ++offset;
                    offset1 = 0;
                }
                //++offset;
                continue;
            }
            c_data = data->recv_str_list_[offset][offset1];
            for (i = 0; i < c_data->vec_buf_wt_.size(); ++i)
            {
                buf = c_data->vec_buf_wt_[i];
                if (buf->wt_pos != 0)
                    write_count += fwrite(buf->ptr, sizeof(char), buf->wt_pos, data->fp_out_);
                if (mem_pool_wt.is_full())
                    SP_DES(buf);
                else
                {
                    buf->wt_pos = 0;
                    buf->rd_pos = 0;
                    SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)buf));
                    mem_pool_wt.enqueue(msg);
                }
            }
            data->recv_str_list_[offset][offset1] = NULL;
            
            ++offset1;
            if (offset1 == 26)
            {
                ++offset;
                offset1 = 0;
            }
            //++offset;
            SP_DEBUG("write (%d,%d)\n", offset,offset1);
            SP_DES(c_data);
        }
        if (offset == 26)
        {
            offset = 0;
            offset1 = 0;
            SP_LOGI("write_count(%zu)\n",write_count);
            SP_LOGI("single_count(%zu)\n",single_count);
            SP_LOGI("single_write_count(%zu)\n",single_write_count);
            SP_NEW(msg, SP_Message_Block_Base(data));
            put_next(msg);
        }
        gettimeofday(&t2,0);
        SP_DEBUG("Merge_Write2File_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Merge_Write2File_Module::init()
{
    return 0;
}

