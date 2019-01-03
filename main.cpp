#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <list>
#include <vector>
#include <algorithm>
#include<sys/time.h>
#include <mutex>

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

FILE * fp_in;
FILE * fp_out;

static SP_Stream * s_instance_stream = NULL;
static SP_Module * modules[6];
SP_Module * write2buf_module = NULL;

int main(int argc, char *argv[])
{
    char * in_file_name = argv[1];
    char * out_file_name = argv[2];
    char * tmp_file_name = argv[3];

#ifdef TIME_TEST
    timeval tm_start, tm_end;
    gettimeofday(&tm_start, NULL);
    
    timeval t1,t2;

    gettimeofday(&t1, NULL);
#endif

    fp_in = NULL;
    size_t in_size = 0;
    buffer = NULL;

    fp_in = fopen(in_file_name, "rb");
    if (fp_in == NULL)
    {
        fputs ("File error",stderr); 
        exit (1);
    }

    fseek (fp_in , 0 , SEEK_END);
    in_size = ftell (fp_in);
    rewind (fp_in);

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

    fp_out = fopen(out_file_name, "wb");
    if (fp_out == NULL)
    {
        fputs ("File out error",stderr);
        exit (1);
    }
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
