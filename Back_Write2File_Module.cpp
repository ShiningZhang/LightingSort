#include "Back_Write2File_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

#include <cstring>

Back_Write2File_Module::Back_Write2File_Module(int threads)
    :threads_num_(threads)
{
}


Back_Write2File_Module::~Back_Write2File_Module()
{
}


int
Back_Write2File_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Back_Write2File_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    uint8_t offset = 0;
    uint8_t offset1 = 0;
    Back_Request * data = NULL;
    Back_CRequest * c_data = NULL;
    Buffer_Element *buf = NULL;
    char single_str_buf[256];
    size_t i = 0;
    size_t write_count = 0;
    size_t single_count = 0;
    size_t single_write_count = 0;
    int vec_back_request_count = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<Back_CRequest *>(msg->data());
        data = c_data->request_;
        data->recv_str_list_[c_data->idx_[0]][c_data->idx_[1]] = c_data;
        SP_DES(msg);
        while (vec_back_request_count < s_vec_back_request.size())
        {
            data = s_vec_back_request[vec_back_request_count];
            if (!data->head_buf_.empty())
            {
                for (i = 0; i < data->head_buf_.size(); ++i)
                {
                    if (data->head_buf_[i]->wt_pos != 0)
                        write_count += fwrite(data->head_buf_[i]->ptr, sizeof(char), data->head_buf_[i]->wt_pos, data->fp_out_);
                    SP_DES(data->head_buf_[i]);
                }
                data->head_buf_.clear();
            }
            while (offset < 26)
            {
                if (offset1 == 0 && data->single_str_count_[offset] != 0)
                {
                    char temp[128];
                    size_t count = 0;
                    memcpy(temp, data->head_str_, data->head_str_size_);
                    i = data->head_str_size_;
                    temp[i++] = 'a' + offset;
                    temp[i++] = '\n';
                    size_t size = i;
                    char * temp_buf = new char[data->single_str_count_[offset] * size];
                    for (i = 0; i < data->single_str_count_[offset]; ++i)
                    {
                        memcpy(temp_buf + count, temp, size);
                        count += size;
                    }
                    write_count += fwrite(temp_buf, sizeof(char), data->single_str_count_[offset] * size, data->fp_out_);
                    delete temp_buf;
                    data->single_str_count_[offset] = 0;
                }
                if (data->send_str_list_[offset][offset1])
                {
                    if (data->recv_str_list_[offset][offset1] == NULL)
                        break;
                } else
                {
                    /*
                    ++offset1;
                    if (offset1 == 26)
                    {
                        ++offset;
                        offset1 = 0;
                    }*/
                    ++offset;
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
                        SP_NEW(msg, SP_Message_Block_Base((SP_Data_Block *)buf));
                        mem_pool_wt.enqueue(msg);
                    }
                }
                data->recv_str_list_[offset][offset1] = NULL;
                /*
                ++offset1;
                if (offset1 == 26)
                {
                    ++offset;
                    offset1 = 0;
                }*/
                ++offset;
                SP_DES(c_data);
            }
            if (offset == 26)
            {
                offset = 0;
                offset1 = 0;
                vec_back_request_count++;
                SP_DEBUG("vec_back_request_count=%d\n", vec_back_request_count);
                SP_DEBUG("write_count(%zu)\n",write_count);
                SP_DEBUG("single_count(%zu)\n",single_count);
                SP_DEBUG("single_write_count(%zu)\n",single_write_count);
            } else
                break;
        }
        if (vec_back_request_count== s_vec_back_request.size())
        {
            vec_back_request_count = 0;
            SP_DEBUG("Back_Write2File_Module: END.\n");
            SP_DEBUG("write_count(%zu)\n",write_count);
            SP_DEBUG("single_count(%zu)\n",single_count);
            SP_DEBUG("single_write_count(%zu)\n",single_write_count);
            SP_NEW(msg, SP_Message_Block_Base(data));
            put_next(msg);
        }
        SP_DEBUG("Back_Write2File_Module:(%d)(%d)\n", offset, vec_back_request_count);
        gettimeofday(&t2,0);
        SP_DEBUG("Back_Write2File_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Back_Write2File_Module::init()
{
    return 0;
}

