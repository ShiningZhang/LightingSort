#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <list>
#include <vector>
#include <algorithm>
#include<sys/time.h>
#include <mutex>
#include <sys/stat.h>


#include "SP_Stream.h"
#include "Global_Macros.h"
#include "Request.h"
#include "Global.h"
#include "ReadFile_Module.h"
#include "Split_Module.h"
#include "Wait_Split_Module.h"
#include "Sort_Module.h"
#include "Write2Buf_Module.h"
#include "Write2File_Module.h"

#include "Front_ReadFile_Module.h"
#include "Front_Split_Module.h"
#include "Front_Wait_Split_Module.h"
#include "Front_Write2File_Module.h"

#include "Back_ReadFile_Module.h"
#include "Back_Split_Module.h"
#include "Back_Wait_Split_Module.h"
#include "Back_Sort_Module.h"
#include "Back_Write2Buf_Module.h"
#include "Back_Write2File_Module.h"

using namespace std;

#define TIME_TEST

struct Element
{
    char * str;
    uint8_t size;
};

bool compare_char(const char *e1, const char *e2)
{
    while(*e1 == *e2 && *e1 != '\n')
    {
        ++e1;
        ++e2;
    }
    return *e1 == *e2 ? e1 < e2 : *e1 < *e2;
}

char * buffer;

SP_Message_Queue mem_pool_wt(BUFFER_ELEMENT_COUNT);
std::vector<SP_Message_Queue*> mem_pool_rd;

FILE * fp_in;
FILE * fp_out;

static SP_Stream * s_instance_stream = NULL;
static SP_Module * modules[6];
SP_Module * write2buf_module = NULL;

vector< Back_Request *> s_vec_back_request;

unsigned long long file_size( char * filename )
{
    unsigned long long size;
    struct stat st;
 
    stat( filename, &st );
    size = st.st_size;
 
   return size;
 
}//get_file_size


int main(int argc, char *argv[])
{
    char * in_file_name = argv[1];
    char * out_file_name = argv[2];
    char * tmp_file_name = argv[3];
    printf("%s,%s,%s\n", in_file_name,out_file_name, tmp_file_name);

#ifdef TIME_TEST
    timeval tm_start, tm_end;
    gettimeofday(&tm_start, NULL);
    
    timeval t1,t2;

    gettimeofday(&t1, NULL);    
#endif

    fp_in = NULL;
    unsigned long long in_size = file_size(in_file_name);
    buffer = NULL;

    fp_in = fopen(in_file_name, "rb");
    if (fp_in == NULL)
    {
        fputs ("File error",stderr); 
        exit (1);
    }

    fp_out = fopen(out_file_name, "wb");
    if (fp_out == NULL)
    {
        fputs ("File out error",stderr);
        exit (1);
    }

    //fseek (fp_in , 0 , SEEK_END);
    //in_size = ftell (fp_in);
    //rewind (fp_in);

    if (in_size < MAX_IN_SIZE)
    {

        buffer = (char*) malloc (sizeof(char)*in_size);
        if (buffer == NULL)
        {
            fputs ("Memory error",stderr); 
            exit (2);
        }

#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        //SP_LOGI("cost1 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif
#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        //SP_LOGI("cost2 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif

        SP_NEW_RETURN(s_instance_stream, SP_Stream, -1);
        SP_NEW_RETURN(modules[0], ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], Split_Module(6), -1);
        SP_NEW_RETURN(modules[2], Wait_Split_Module(1), -1);
        SP_NEW_RETURN(modules[3], Sort_Module(6), -1);
        SP_NEW_RETURN(modules[4], Write2Buf_Module(6), -1);
        SP_NEW_RETURN(modules[5], Write2File_Module(1), -1);

        for (int i = 5; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        write2buf_module = modules[5];

        Buffer_Element * buf;
        SP_Message_Block_Base * msg;
        for (size_t i = 0; i < BUFFER_ELEMENT_COUNT; ++i)
        {
            SP_NEW_RETURN(buf, Buffer_Element(BUFFER_ELEMENT_SIZE), -1);
            SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)buf), -1);
            mem_pool_wt.enqueue(msg);
        }


#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        //SP_LOGI("cost3 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif

        Request * data;
        SP_NEW_RETURN(data, Request(), -1);
        data->size_split_buf = 0;
        data->buffer_ = buffer;
        data->begin_ = 0;
        data->end_ = 0;
        data->length_ = in_size;
        data->fp_out_ = fp_out;
        data->fp_in_ = fp_in;
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)data), -1);
        if (s_instance_stream->put(msg) == -1)
        {
            SP_LOGE("processing : Put Msg failed!\n");
        }
        if (s_instance_stream->get(msg) == -1)
        {
            SP_LOGE("processing : Get Msg failed!\n");
        }
        SP_DES(msg);

#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        //SP_LOGI("cost4 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif

    } else
    {
        buffer = (char*) malloc (sizeof(char)*MAX_FRONT_IN_SIZE);
        int split_size = 0;
        vector<vector<File_Element*>> mid_file_ptr;
        unsigned long long rd_pos = 0;
#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        //SP_LOGI("cost2 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif

        SP_NEW_RETURN(s_instance_stream, SP_Stream, -1);
        SP_NEW_RETURN(modules[0], Front_ReadFile_Module(1), -1);
        SP_NEW_RETURN(modules[1], Front_Split_Module(6), -1);
        SP_NEW_RETURN(modules[2], Front_Wait_Split_Module(1), -1);
        SP_NEW_RETURN(modules[3], Front_Write2File_Module(1), -1);

        for (int i = 3; i >= 0; --i)
        {
            s_instance_stream->push_module(modules[i]);
        }
        write2buf_module = modules[5];

        Buffer_Element * buf;
        SP_Message_Block_Base * msg;

#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        //SP_LOGI("cost3 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif

        Front_Request * data;
        SP_NEW_RETURN(data, Front_Request(), -1);
        FILE * tmp_fp;
        char tmp_char[256];
        split_size = 0;
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)data), -1);
        for (int i = 0; i < 26; ++i)
        {
            vector<File_Element*> temp;
            for (int j = 0; j < 26; ++j)
            {
                sprintf(tmp_char, "%s/%d_%d", tmp_file_name, i, j);
                tmp_fp = fopen(tmp_char, "wb+");
                temp.emplace_back(new File_Element(tmp_fp));
            }
            mid_file_ptr.emplace_back(temp);
        }
        data->vec_mid_fp_ = mid_file_ptr;
        data->vec_buf_.resize(26);
        for (int i = 0; i < 26; ++i)
            data->vec_buf_[i].resize(26);
        while (rd_pos < in_size)
        {
            data->size_split_buf = 0;
            data->count_ = 0;
            data->buffer_ = buffer;
            data->begin_ = 0;
            data->end_ = 0;
            data->length_ = (rd_pos + MAX_FRONT_IN_SIZE < in_size) ? MAX_FRONT_IN_SIZE : in_size - rd_pos;
            data->fp_out_ = NULL;
            data->fp_in_ = fp_in;
            data->is_read_end_ = false;
            data->is_split_end_ = false;
            data->send_split_count_ = 0;
            data->recv_split_count_ = 0;
            if (s_instance_stream->put(msg) == -1)
            {
                SP_LOGE("processing : Put Msg failed!\n");
            }
            if (s_instance_stream->get(msg) == -1)
            {
                SP_LOGE("processing : Get Msg failed!\n");
            }
            rd_pos += data->length_;
            split_size++;
        }
        SP_DES(msg);
        for(int i=0;i<26;++i)
        {
            for(int j =0;j<26;++j)
            {
                data->vec_mid_fp_[i][j]->size_ = ftell(data->vec_mid_fp_[i][j]->fp_);
                rewind (data->vec_mid_fp_[i][j]->fp_);
                if (data->vec_mid_fp_[i][j]->size_ > MAX_BACK_IN_SIZE)
                {
                    Front_Request * front_data = new Front_Request();
                    data->vec_mid_fp_[i][j]->data = front_data;
                    for (int m = 0; m < 26; ++m)
                    {
                        vector<File_Element*> temp;
                        for (int n = 0; n < 26; ++n)
                        {
                            sprintf(tmp_char, "%s/%d_%d_%d_%d", tmp_file_name, i, j, m, n);
                            tmp_fp = fopen(tmp_char, "wb+");
                            temp.emplace_back(new File_Element(tmp_fp));
                        }
                        front_data->vec_mid_fp_.emplace_back(temp);
                    }
                    front_data->vec_buf_.resize(26);
                    for (int m = 0; m < 26; ++m)
                        front_data->vec_buf_[m].resize(26);
                    unsigned long long tmp_rd_pos = 0;
                    unsigned long long tmp_in_size = data->vec_mid_fp_[i][j]->size_;
                    SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)front_data), -1);
                    while (tmp_rd_pos < tmp_in_size)
                    {
                        front_data->size_split_buf = 0;
                        front_data->count_ = 0;
                        front_data->buffer_ = buffer;
                        front_data->begin_ = 0;
                        front_data->end_ = 0;
                        front_data->length_ = (tmp_rd_pos + MAX_FRONT_IN_SIZE < tmp_in_size) ? MAX_FRONT_IN_SIZE : tmp_in_size - tmp_rd_pos;
                        front_data->fp_out_ = NULL;
                        front_data->fp_in_ = data->vec_mid_fp_[i][j]->fp_;
                        front_data->is_read_end_ = false;
                        front_data->is_split_end_ = false;
                        front_data->send_split_count_ = 0;
                        front_data->recv_split_count_ = 0;
                        if (s_instance_stream->put(msg) == -1)
                        {
                            SP_LOGE("processing : Put Msg failed!\n");
                        }
                        if (s_instance_stream->get(msg) == -1)
                        {
                            SP_LOGE("processing : Get Msg failed!\n");
                        }
                        tmp_rd_pos += front_data->length_;
                    }
                    SP_DES(msg);
                    for (int m = 0; m < 26; ++m)
                    {
                        for (int n = 0; n < 26; ++n)
                        {
                            front_data->vec_mid_fp_[m][n]->size_ = ftell(front_data->vec_mid_fp_[m][n]->fp_);
                            rewind (front_data->vec_mid_fp_[m][n]->fp_);
                        }
                    }
                }
            }
        }
        unsigned long long wt_size=0;
        size_t single_count=0;
        size_t double_count=0;
        for(int i=0;i<26;++i)
        {
            single_count +=data->single_str_count_[i];
            for(int j =0;j<26;++j)
            {
                double_count+=data->double_str_count_[i][j];
                wt_size +=data->vec_mid_fp_[i][j]->size_;
            }
        }
        SP_LOGI("sigle=%d,double=%d\n", single_count, double_count);
        SP_LOGI("wt_size=%lld\n", wt_size);
        //SP_DES(data);
        //s_instance_stream->close();
        //SP_DES(s_instance_stream);
#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        SP_LOGI("cost front =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif

        if (MAX_FRONT_IN_SIZE != MAX_BACK_IN_SIZE)
        {
            free(buffer);
            buffer = (char*) malloc (sizeof(char)*MAX_BACK_IN_SIZE);
        }

        SP_Stream *back_stream = NULL;
        SP_Module *back_modules[6];
        SP_NEW_RETURN(back_stream, SP_Stream, -1);
        SP_NEW_RETURN(back_modules[0], Back_ReadFile_Module(1), -1);
        SP_NEW_RETURN(back_modules[1], Back_Split_Module(6), -1);
        SP_NEW_RETURN(back_modules[2], Back_Wait_Split_Module(1), -1);
        SP_NEW_RETURN(back_modules[3], Back_Sort_Module(6), -1);
        SP_NEW_RETURN(back_modules[4], Back_Write2Buf_Module(6), -1);
        SP_NEW_RETURN(back_modules[5], Back_Write2File_Module(1), -1);

        for (int i = 5; i >= 0; --i)
        {
            back_stream->push_module(back_modules[i]);
        }

        for (size_t i = 0; i < BUFFER_ELEMENT_COUNT; ++i)
        {
            SP_NEW_RETURN(buf, Buffer_Element(BUFFER_ELEMENT_SIZE), -1);
            SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)buf), -1);
            mem_pool_wt.enqueue(msg);
        }
        
        Back_Request * back_data = new Back_Request();
        msg = new SP_Message_Block_Base((SP_Data_Block *)back_data);
        size_t total_perpared_size = 0;
        for (int i = 0; i < 26; ++i)
        {
            for (int j = 0; j < 26; ++j)
            {
                if (j == 0 && data->single_str_count_[i] != 0)
                {
                    char temp[128];
                    int k = 0;
                    temp[k++] = 'a'+i;
                    temp[k++] = '\n';
                    int size = k;
                    buf = new Buffer_Element(data->single_str_count_[i] * size);
                    for (k = 0; k < data->single_str_count_[i]; ++k)
                    {
                        memcpy(buf->ptr + buf->wt_pos, temp, size);
                        buf->wt_pos += size;
                    }
                    back_data->head_buf_.emplace_back(buf);
                    data->single_str_count_[i] = 0;
                }
                if (data->double_str_count_[i][j] != 0)
                {
                    char temp[128];
                    int k = 0;
                    temp[k++] = 'a'+i;
                    temp[k++] = 'a'+j;
                    temp[k++] = '\n';
                    int size = k;
                    buf = new Buffer_Element(data->double_str_count_[i][j] * size);
                    for (k = 0; k < data->double_str_count_[i][j]; ++k)
                    {
                        memcpy(buf->ptr + buf->wt_pos, temp, size);
                        buf->wt_pos += size;
                    }
                    back_data->head_buf_.emplace_back(buf);
                    data->double_str_count_[i][j] = 0;
                }
                if (data->vec_mid_fp_[i][j]->size_ == 0)
                    continue;
                //SP_LOGI("(%d,%d)after. size(%d), total_perpared_size(%zu)\n",i,j,data->vec_mid_fp_[i][j]->size_,total_perpared_size);
                if (total_perpared_size + data->vec_mid_fp_[i][j]->size_ < MAX_BACK_IN_SIZE
                    && s_vec_back_request.size() < 26)
                {
                    //SP_LOGI("(%d,%d)(%p)total=%zu,fp_size=%d\n",i,j,back_data,total_perpared_size,data->vec_mid_fp_[i][j]->size_);
                    back_data->idx_.clear();
                    back_data->idx_.emplace_back(i);
                    back_data->idx_.emplace_back(j);
                    back_data->size_split_buf = 0;
                    back_data->count_ = 0;
                    back_data->buffer_ = buffer + total_perpared_size;
                    back_data->begin_ = 0;
                    back_data->end_ = 0;
                    back_data->length_ = data->vec_mid_fp_[i][j]->size_;
                    back_data->fp_out_ = fp_out;
                    back_data->fp_in_ = data->vec_mid_fp_[i][j]->fp_;
                    back_data->is_read_end_ = false;
                    back_data->head_str_size_ = 2;
                    back_data->head_str_[0] = 'a' + i;
                    back_data->head_str_[1] = 'a' + j;
                    total_perpared_size += data->vec_mid_fp_[i][j]->size_;
                    s_vec_back_request.emplace_back(back_data);
                    back_data = new Back_Request();
                    SP_DEBUG("1:(%d,%d)total_perpared_size=%lld,size=%d\n",
                        i,j,total_perpared_size,s_vec_back_request.size());
                    continue;
                } else
                {
                    if (!s_vec_back_request.empty())
                    {
                        SP_DEBUG("in:total_perpared_size=%lld,size=%d\n",total_perpared_size,s_vec_back_request.size());
                        msg->data((SP_Data_Block *)(s_vec_back_request.back()));
                        if (back_stream->put(msg) == -1)
                        {
                            SP_LOGE("processing : Put Msg failed!\n");
                        }
                        if (back_stream->get(msg) == -1)
                        {
                            SP_LOGE("processing : Get Msg failed!\n");
                        }
                        for (int k = 0; k < s_vec_back_request.size(); ++k)
                            delete s_vec_back_request[k];
                        s_vec_back_request.clear();
                        total_perpared_size = 0;
                    }
                    if (data->vec_mid_fp_[i][j]->size_ < MAX_BACK_IN_SIZE)
                    {
                        SP_DEBUG("3.1: (%d,%d)(%p),total=%zu,fp_size=%d\n",i,j,back_data,total_perpared_size,data->vec_mid_fp_[i][j]->size_);
                        back_data->idx_.clear();
                        back_data->idx_.emplace_back(i);
                        back_data->idx_.emplace_back(j);
                        back_data->size_split_buf = 0;
                        back_data->count_ = 0;
                        back_data->buffer_ = buffer + total_perpared_size;
                        back_data->begin_ = 0;
                        back_data->end_ = 0;
                        back_data->length_ = data->vec_mid_fp_[i][j]->size_;
                        back_data->fp_out_ = fp_out;
                        back_data->fp_in_ = data->vec_mid_fp_[i][j]->fp_;
                        back_data->is_read_end_ = false;
                        back_data->head_str_size_ = 2;
                        back_data->head_str_[0] = 'a' + i;
                        back_data->head_str_[1] = 'a' + j;
                        total_perpared_size += data->vec_mid_fp_[i][j]->size_;
                        s_vec_back_request.emplace_back(back_data);
                        back_data = new Back_Request();
                        continue;
                    }
                }
                if (data->vec_mid_fp_[i][j]->size_ < MAX_BACK_IN_SIZE)
                {
                    SP_LOGE("main: error wrong way!");
                } else
                {
                    SP_DEBUG("(%d,%d)Large file.\n", i,j);
                    Front_Request *front_data = data->vec_mid_fp_[i][j]->data;
                    for (int m = 0; m < 26; ++m)
                    {
                        for (int n = 0; n < 26; ++n)
                        {
                            if (n == 0 && front_data->single_str_count_[m] != 0)
                            {
                                char temp[128];
                                int k = 0;
                                temp[k++] = 'a'+i;
                                temp[k++] = 'a'+j;
                                temp[k++] = 'a'+m;
                                temp[k++] = '\n';
                                int size = k;
                                buf = new Buffer_Element(front_data->single_str_count_[m] * size);
                                for (k = 0; k < front_data->single_str_count_[m]; ++k)
                                {
                                    memcpy(buf->ptr + buf->wt_pos, temp, size);
                                    buf->wt_pos += size;
                                }
                                back_data->head_buf_.emplace_back(buf);
                                front_data->single_str_count_[m] = 0;
                            }
                            if (front_data->double_str_count_[m][n] != 0)
                            {
                                char temp[128];
                                int k = 0;
                                temp[k++] = 'a'+i;
                                temp[k++] = 'a'+j;
                                temp[k++] = 'a'+m;
                                temp[k++] = 'a'+n;
                                temp[k++] = '\n';
                                int size = k;
                                buf = new Buffer_Element(front_data->double_str_count_[m][n] * size);
                                for (k = 0; k < front_data->double_str_count_[m][n]; ++k)
                                {
                                    memcpy(buf->ptr + buf->wt_pos, temp, size);
                                    buf->wt_pos += size;
                                }
                                back_data->head_buf_.emplace_back(buf);
                                front_data->double_str_count_[m][n] = 0;
                            }
                            if (front_data->vec_mid_fp_[m][n]->size_ == 0)
                                continue;
                            if (total_perpared_size + front_data->vec_mid_fp_[m][n]->size_ < MAX_BACK_IN_SIZE
                                && s_vec_back_request.size() < 26)
                            {
                                back_data->idx_.clear();
                                back_data->idx_.emplace_back(i);
                                back_data->idx_.emplace_back(j);
                                back_data->idx_.emplace_back(m);
                                back_data->idx_.emplace_back(n);
                                back_data->size_split_buf = 0;
                                back_data->count_ = 0;
                                back_data->buffer_ = buffer + total_perpared_size;
                                back_data->begin_ = 0;
                                back_data->end_ = 0;
                                back_data->length_ = front_data->vec_mid_fp_[m][n]->size_;
                                back_data->fp_out_ = fp_out;
                                back_data->fp_in_ = front_data->vec_mid_fp_[m][n]->fp_;
                                back_data->is_read_end_ = false;
                                back_data->head_str_size_ = 4;
                                back_data->head_str_[0] = 'a' + i;
                                back_data->head_str_[1] = 'a' + j;
                                back_data->head_str_[2] = 'a' + m;
                                back_data->head_str_[3] = 'a' + n;
                                total_perpared_size += data->vec_mid_fp_[i][j]->size_;
                                s_vec_back_request.emplace_back(back_data);
                                back_data = new Back_Request();
                                continue;
                            } else
                            {
                                total_perpared_size = 0;
                                msg->data((SP_Data_Block *)(s_vec_back_request.back()));
                                if (back_stream->put(msg) == -1)
                                {
                                    SP_LOGE("processing : Put Msg failed!\n");
                                }
                                if (back_stream->get(msg) == -1)
                                {
                                    SP_LOGE("processing : Get Msg failed!\n");
                                }
                                for (int k = 0; k < s_vec_back_request.size(); ++k)
                                    delete s_vec_back_request[k];
                                s_vec_back_request.clear();
                                back_data->idx_.clear();
                                back_data->idx_.emplace_back(i);
                                back_data->idx_.emplace_back(j);
                                back_data->idx_.emplace_back(m);
                                back_data->idx_.emplace_back(n);
                                back_data->size_split_buf = 0;
                                back_data->count_ = 0;
                                back_data->buffer_ = buffer + total_perpared_size;
                                back_data->begin_ = 0;
                                back_data->end_ = 0;
                                back_data->length_ = front_data->vec_mid_fp_[m][n]->size_;
                                back_data->fp_out_ = fp_out;
                                back_data->fp_in_ = front_data->vec_mid_fp_[m][n]->fp_;
                                back_data->is_read_end_ = false;
                                back_data->head_str_size_ = 4;
                                back_data->head_str_[0] = 'a' + i;
                                back_data->head_str_[1] = 'a' + j;
                                back_data->head_str_[2] = 'a' + m;
                                back_data->head_str_[3] = 'a' + n;
                                total_perpared_size += data->vec_mid_fp_[i][j]->size_;
                                s_vec_back_request.emplace_back(back_data);
                                back_data = new Back_Request();
                            }
                        }
                    }
                }
            }
        }
        if (!s_vec_back_request.empty())
        {
            SP_DEBUG("in:total_perpared_size=%lld,size=%d",total_perpared_size,s_vec_back_request.size());
            msg->data((SP_Data_Block *)(s_vec_back_request.back()));
            if (back_stream->put(msg) == -1)
            {
                SP_LOGE("processing : Put Msg failed!\n");
            }
            if (back_stream->get(msg) == -1)
            {
                SP_LOGE("processing : Get Msg failed!\n");
            }
            for (int k = 0; k < s_vec_back_request.size(); ++k)
                delete s_vec_back_request[k];
            s_vec_back_request.clear();
            total_perpared_size = 0;
        }

#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        SP_LOGI("cost back =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
        gettimeofday(&t1, NULL);
#endif
    }

    //s_instance_stream->close();
    //SP_DES(s_instance_stream);

    fclose(fp_in);
    fclose(fp_out);

    //free(buffer);
#ifdef TIME_TEST
    gettimeofday(&t2, NULL);
    //SP_LOGI("cost5 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
    gettimeofday(&tm_end, NULL);
    SP_LOGI("total cost =%ld.\n", (tm_end.tv_sec-tm_start.tv_sec)*1000+(tm_end.tv_usec-tm_start.tv_usec)/1000);
#endif
    return 0;
}
