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

#include "Merge_ReadFile_Module.h"
#include "Merge_Split_Module.h"
#include "Merge_Wait_Split_Module.h"
#include "Merge_Write2Buf_Module.h"
#include "Merge_Write2File_Module.h"

#include "Final_Write2Buf_Module.h"
#include "Final_Write2File_Module.h"


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

uint8_t com_offset;
bool compare_pair(const std::pair<char *, uint8_t> &e1, const  std::pair<char *, uint8_t> &e2)
{
    com_offset = 0;
    while(*(e1.first + com_offset) == *(e2.first + com_offset) && *(e1.first + com_offset) != '\n')
    {
        ++com_offset;
    }
    return *(e1.first + com_offset) == *(e2.first + com_offset) ? (e1.first) < (e2.first) : *(e1.first + com_offset) < *(e2.first + com_offset);
}


char * buffer;

SP_Message_Queue mem_pool_wt(BUFFER_ELEMENT_COUNT);
std::vector<SP_Message_Queue*> mem_pool_rd;

FILE * fp_in;
FILE * fp_out;

static SP_Stream * s_instance_stream = NULL;
static SP_Module * modules[6];
SP_Module * write2buf_module = NULL;

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
        buffer = (char*) malloc (sizeof(char)*MAX_IN_SIZE);
        int split_size = 0;
        vector<File_Element> mid_file_ptr;
        unsigned long long rd_pos = 0;
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
        FILE * tmp_fp;
        char tmp_char[256];
        split_size = 0;
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)data), -1);
        while (rd_pos < in_size)
        {
            sprintf(tmp_char, "%s/%d", tmp_file_name, split_size);
            SP_LOGI("tmp_char=%s\n", tmp_char);
            tmp_fp = fopen(tmp_char, "wb+");
            mid_file_ptr.emplace_back(File_Element(tmp_fp));

            data->size_split_buf = 0;
            data->count_ = 0;
            data->buffer_ = buffer;
            data->begin_ = 0;
            data->end_ = 0;
            data->length_ = (rd_pos + MAX_IN_SIZE < in_size) ? MAX_IN_SIZE : in_size - rd_pos;
            data->fp_out_ = tmp_fp;
            data->fp_in_ = fp_in;
            if (s_instance_stream->put(msg) == -1)
            {
                SP_LOGE("processing : Put Msg failed!\n");
            }
            if (s_instance_stream->get(msg) == -1)
            {
                SP_LOGE("processing : Get Msg failed!\n");
            }
            mid_file_ptr[split_size].size_ = ftell(mid_file_ptr[split_size].fp_);
            rewind (mid_file_ptr[split_size].fp_);
            rd_pos += data->length_;
            split_size++;
        }
        SP_DES(msg);
        SP_DES(data);
        s_instance_stream->close();
        SP_DES(s_instance_stream);
        SP_LOGI("1\n");

        size_t size = MAX_IN_SIZE / mid_file_ptr.size() / 16;
        size_t offset = 0;
        SP_Message_Queue * q = NULL;
        SP_Message_Block_Base * buf_msg = NULL;
        int i, j, k;
        for(i = 0; i < mid_file_ptr.size(); ++i)
        {
            q = new SP_Message_Queue;
            for (j = 0; j < 16; ++j)
            {
                SP_NEW_RETURN(buf, Buffer_Element(buffer + offset, size), -1);
                SP_NEW_RETURN(buf_msg, SP_Message_Block_Base((SP_Data_Block *)buf), -1);
                q->enqueue(buf_msg);
                offset += size;
            }
            mem_pool_rd.emplace_back(q);
        }

        SP_Stream * s_instance_mergestream = NULL;
        SP_Module * merge_modules[6];
        SP_NEW_RETURN(s_instance_mergestream, SP_Stream, -1);
        SP_NEW_RETURN(merge_modules[0], Merge_ReadFile_Module(1), -1);
        SP_NEW_RETURN(merge_modules[1], Merge_Split_Module(1), -1);
        SP_NEW_RETURN(merge_modules[2], Merge_Wait_Split_Module(1), -1);
        SP_NEW_RETURN(merge_modules[3], Merge_Write2Buf_Module(6), -1);
        SP_NEW_RETURN(merge_modules[4], Merge_Write2File_Module(1), -1);
        for (i = 4; i >= 0; --i)
        {
            s_instance_mergestream->push_module(merge_modules[i]);
        }
        Merge_Request * merge_data;
        SP_NEW_RETURN(merge_data, Merge_Request(), -1);
        SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)merge_data), -1);
        merge_data->vec_mid_fp_ = mid_file_ptr;
        merge_data->fp_in_ = fp_in;
        merge_data->fp_out_ = fp_out;
        merge_data->split_size_ = split_size;
        merge_data->vec_buf_.resize(split_size);
        merge_data->vec_last_idx_.resize(split_size);
        merge_data->vec_last_ptr_.resize(split_size);
        std::vector<size_t> alive_fp(split_size, 0);
        for(i = 0; i < split_size; ++i)
            alive_fp[i] = i;
        while(merge_data->split_size_ != 1)
        {
            merge_data->size_split_buf = 0;
            merge_data->count_ = 0;
            if (s_instance_mergestream->put(msg) == -1)
            {
                SP_LOGE("processing : Put Msg failed!\n");
            }
            if (s_instance_mergestream->get(msg) == -1)
            {
                SP_LOGE("processing : Get Msg failed!\n");
            }
            for (i = 0; i < alive_fp.size(); ++i)
            {
                for (j = 0; j < merge_data->vec_buf_[alive_fp[i]].size(); ++j)
                {
                    if (merge_data->vec_last_ptr_[alive_fp[i]].ptr - merge_data->vec_buf_[alive_fp[i]][j]->ptr < merge_data->vec_buf_[alive_fp[i]][j]->length)
                        break;
                }
                if (j != 0)
                {
                    for (k = 0; k < j; ++k)
                    {
                        buf = merge_data->vec_buf_[alive_fp[i]][k];
                        buf->wt_pos = 0;
                        SP_NEW_RETURN(buf_msg, SP_Message_Block_Base((SP_Data_Block *)buf), -1);
                        mem_pool_rd[alive_fp[i]]->enqueue(buf_msg);
                    }
                    merge_data->vec_buf_[alive_fp[i]].erase(merge_data->vec_buf_[alive_fp[i]].begin(),
                                                            merge_data->vec_buf_[alive_fp[i]].begin() + j);
                }
                if (merge_data->vec_mid_fp_[alive_fp[i]].wt_pos == merge_data->vec_mid_fp_[alive_fp[i]].size_)
                {
                    merge_data->split_size_--;
                    fclose(merge_data->vec_mid_fp_[alive_fp[i]].fp_);
                    merge_data->vec_last_idx_[alive_fp[i]] = make_pair(26,26);
                    offset = 0;
                    while(!mem_pool_rd[alive_fp[i]]->is_empty())
                    {
                        mem_pool_rd[alive_fp[i]]->dequeue(buf_msg);
                        if (offset == alive_fp[i])
                            offset = (offset + 1) % alive_fp.size();
                        mem_pool_rd[offset]->dequeue(buf_msg);
                        offset = (offset + 1) % alive_fp.size();
                    }
                    alive_fp.erase(alive_fp.begin()+i);
                    i--;
                }
            }
        }

        SP_Stream * s_instance_finalstream = NULL;
        SP_Module * final_modules[2];
        SP_NEW_RETURN(s_instance_finalstream, SP_Stream, -1);
        SP_NEW_RETURN(final_modules[0], Final_Write2Buf_Module(6), -1);
        SP_NEW_RETURN(final_modules[1], Final_Write2File_Module(1), -1);
        for (i = 1; i >= 0; --i)
        {
            s_instance_finalstream->push_module(final_modules[i]);
        }
        merge_data->count_ = 0;
        Merge_CRequest * next_data = NULL;
        SP_Message_Block_Base *next_msg = NULL;
        memset(merge_data->send_str_list_, 0, sizeof(merge_data->send_str_list_));
        for (i = 0; i < 26; ++i)
        {
            for (j = 0; j < 26; ++j)
            {
                for (k = 0; k < 26; ++k)
                {
                    if (merge_data->str_list_[i][j][k].empty())
                    {
                        continue;
                    }
                    ++merge_data->count_;
                    merge_data->send_str_list_[i][j] = true;
                    SP_NEW_RETURN(next_data, Merge_CRequest(merge_data), -1);
                    next_data->idx_.emplace_back(std::make_pair(i,j));
                    SP_NEW_RETURN(next_msg, SP_Message_Block_Base((SP_Data_Block *)next_data), -1);
                    s_instance_finalstream->put(next_msg);
                    break;
                }
            }
        }
        if (s_instance_finalstream->get(next_msg) == -1)
        {
            SP_LOGE("processing : Get Msg failed!\n");
        }
        size_t line_size = merge_data->vec_mid_fp_[alive_fp[0]].size_ - merge_data->vec_mid_fp_[alive_fp[0]].wt_pos;
        size_t result = fread (buffer,1,line_size,merge_data->vec_mid_fp_[alive_fp[0]].fp_);
        fwrite(buffer, sizeof(char), line_size, merge_data->fp_out_);

#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        //SP_LOGI("cost4 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
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
