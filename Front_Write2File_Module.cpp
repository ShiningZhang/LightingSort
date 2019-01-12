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
    std::vector<Buffer_Element*> tmp_vec_buf;
    for (SP_Message_Block_Base *msg = 0; get(msg) != -1;)
    {
        timeval t2,start;
        gettimeofday(&start,0);
        c_data = reinterpret_cast<Front_CRequest *>(msg->data());
        data = c_data->request_;
        data->recv_split_count_++;
        SP_DES(msg);
        SP_DES(c_data);
        for (offset = 0; offset < 26; ++offset)
        {
            for (j = 0; j < 26; ++j)
            {
                data->lock_str_list_[offset][j].lock();
                tmp_vec_buf = data->vec_buf_[offset][j];
                data->vec_buf_[offset][j].clear();
                data->lock_str_list_[offset][j].unlock();
                for (i = 0; i < tmp_vec_buf.size(); ++i)
                {
                    buf = tmp_vec_buf[i];
                    if (buf->wt_pos != 0)
                        write_count += fwrite(buf->ptr, sizeof(char), buf->wt_pos, data->vec_mid_fp_[offset][j]->fp_);
                    data->vec_mid_fp_[offset][j]->size_ += buf->wt_pos;
                    SP_DEBUG("(%d,%d)i=%d,buf(%p),wt_pos(%d)\n",offset,j,i,buf,buf->wt_pos);
                    SP_DES(buf);
                }
            }
        }
        SP_DEBUG("is_split_end_=%d,recv_split_count_=%d,send_split_count_=%d\n",
            data->is_split_end_, data->recv_split_count_, data->send_split_count_);
        if (data->is_split_end_ && data->recv_split_count_ == data->send_split_count_)
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

