#pragma once

#include <vector>
#include <mutex>
#include <queue>
#include<sys/time.h>

#include "SP_Message_Queue.h"
#include "SP_Data_Block.h"

#define SPLIT_BUFFER_NUM 6
#define BUFFER_ELEMENT_SIZE 1024 * 1024 * 128
#define BUFFER_ELEMENT_SIZE_SPLIT 1024 * 1024 * 16
#define BUFFER_ELEMENT_COUNT 16
#define STR_LIST_SIZE 1024 * 1024

bool compare_char(const char *e1, const char *e2);

class Buffer_Element : public SP_Data_Block
{
public:
    Buffer_Element(size_t size)
        :ptr(NULL)
        ,length(size)
        ,rd_pos(0)
        ,wt_pos(0)
        {
            ptr = (char *)malloc(sizeof(char) * size); 
        };
    ~Buffer_Element(){ free(ptr); };
    char *ptr;
    size_t length;
    size_t rd_pos;
    size_t wt_pos;
};

typedef std::vector<char *> TEMPLIST;

extern char * buffer;

extern SP_Message_Queue mem_pool_wt;


extern FILE * fp_in;
extern FILE * fp_out;
