#include "Front_Write2File_Module.h"
#include "Global_Macros.h"
#include "SP_Message_Block_Base.h"
#include "Request.h"
#include "Global.h"

Front_Write2File_Module::Front_Write2File_Module(int threads)
    :threads_num_(threads)
{
}


Front_Write2File_Module::~Front_Write2File_Module()
{
}


int
Front_Write2File_Module::open()
{
    activate(threads_num_);
    return 0;
}

void
Front_Write2File_Module::svc()
{
    static int sthread_num = 0;
    int thread_num;
    lock_.lock();
    thread_num = sthread_num++;
    lock_.unlock();
    uint8_t offset = 0;
    uint8_t offset1 = 0;
    Front_Request * data = NULL;
    Front_CRequest * c_data = NULL;
    Buffer_Element *buf = NULL;
    size_t i = 0, j;
    size_t write_count = 0;
    size_t single_count = 0;
    size_t single_write_count = 0;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<Front_CRequest *>(msg->data());
        data = c_data->request_;
        data->recv_str_list_[c_data->idx_[0].first][c_data->idx_[0].second] = c_data;
        SP_DES(msg);
        while (offset < 26)
        {
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
            for (j = 0; j < 26; ++j)
            {
                for (i = 0; i < data->vec_buf_[offset][j].size(); ++i)
                {
                    buf = data->vec_buf_[offset][j][i];
                    if (buf->wt_pos != 0)
                        write_count += fwrite(buf->ptr, sizeof(char), buf->wt_pos, data->vec_mid_fp_[offset][j]->fp_);
                    data->vec_mid_fp_[offset][j]->size_ += buf->wt_pos;
                    SP_DEBUG("(%d,%d)i=%d,buf(%p),wt_pos(%d)\n",offset,j,i,buf,buf->wt_pos);
                    SP_DES(buf);
                }
                data->vec_buf_[offset][j].clear();
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
            SP_DEBUG("write_count(%zu)\n",write_count);
            SP_DEBUG("single_count(%zu)\n",single_count);
            SP_DEBUG("single_write_count(%zu)\n",single_write_count);
            SP_NEW(msg, SP_Message_Block_Base(data));
            put_next(msg);
        }
        gettimeofday(&t2,0);
        SP_DEBUG("Front_Write2File_Module=%ldms.\n", (t2.tv_sec-start.tv_sec)*1000+(t2.tv_usec-start.tv_usec)/1000);
    }
}

int
Front_Write2File_Module::init()
{
    return 0;
}

