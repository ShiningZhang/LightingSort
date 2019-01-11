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
#include "Merge_Sort_Module.h"
#include "Merge_Write2Buf_Module.h"
#include "Merge_Write2File_Module.h"

#include "Final_Write2Buf_Module.h"
#include "Final_Write2File_Module.h"

#include "Front_ReadFile_Module.h"
#include "Front_Split_Module.h"
#include "Front_Wait_Split_Module.h"
#include "Front_Write2File_Module.h"



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
        vector<vector<File_Element>> mid_file_ptr;
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
        /*
        for (size_t i = 0; i < BUFFER_ELEMENT_COUNT; ++i)
        {
            SP_NEW_RETURN(buf, Buffer_Element(BUFFER_ELEMENT_SIZE), -1);
            SP_NEW_RETURN(msg, SP_Message_Block_Base((SP_Data_Block *)buf), -1);
            mem_pool_wt.enqueue(msg);
        }
*/

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
            vector<File_Element> temp;
            for (int j = 0; j < 26; ++j)
            {
                sprintf(tmp_char, "%s/%d_%d", tmp_file_name, i, j);
                tmp_fp = fopen(tmp_char, "wb+");
                temp.emplace_back(File_Element(tmp_fp));
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
            data->length_ = (rd_pos + MAX_IN_SIZE < in_size) ? MAX_IN_SIZE : in_size - rd_pos;
            data->fp_out_ = NULL;
            data->fp_in_ = fp_in;
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
        unsigned long long wt_size=0;
        size_t single_count=0;
        size_t double_count=0;
        for(int i=0;i<26;++i)
        {
            single_count +=data->single_str_count_[i];
            for(int j =0;j<26;++j)
            {
                double_count+=data->double_str_count_[i][j];
                wt_size +=data->vec_mid_fp_[i][j].size_;
            }
        }
        SP_LOGI("sigle=%d,double=%d\n", single_count, double_count);
        SP_LOGI("wt_size=%lld\n", wt_size);
        SP_DES(msg);
        SP_DES(data);
        s_instance_stream->close();
        SP_DES(s_instance_stream);
        SP_LOGI("1\n");

        

#ifdef TIME_TEST
        gettimeofday(&t2, NULL);
        SP_LOGI("cost4 =%ld.\n", (t2.tv_sec-t1.tv_sec)*1000+(t2.tv_usec-t1.tv_usec)/1000);
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
