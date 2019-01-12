#pragma once

#include <vector>
#include <mutex>
#include <queue>
#include<sys/time.h>

#include "SP_Message_Queue.h"
#include "SP_Data_Block.h"

#define SPLIT_BUFFER_NUM 6
#define BUFFER_ELEMENT_SIZE 1024 * 1024
#define BUFFER_ELEMENT_SIZE_SPLIT 1024 * 1024 * 16
#define BUFFER_ELEMENT_COUNT 1024
#define STR_LIST_SIZE 1024 * 1024
#define BUFFER_ELEMENT_FRONT_SIZE 1024 * 1024
#define BUFFER_ELEMENT_BACK_SIZE 1024 * 512
#define BUFFER_ELEMENT_BACK_COUNT 2048


const static unsigned long long MAX_IN_SIZE = 1024 * 1024 * 1024;
const static unsigned long long MAX_FRONT_IN_SIZE = MAX_IN_SIZE * 1.5;
const static unsigned long long MAX_BACK_IN_SIZE = MAX_IN_SIZE;
bool compare_char(const char *e1, const char *e2);


class Buffer_Element : public SP_Data_Block
{
public:
    Buffer_Element(size_t size);
    virtual ~Buffer_Element();
    char *ptr;
    size_t length;
    size_t rd_pos;
    size_t wt_pos;
};

class Front_Request;
class Back_Request;

class File_Element
{
public:
    File_Element(FILE * fp = NULL);
    virtual ~File_Element();
    FILE * fp_;
    size_t size_;
    size_t rd_pos;
    size_t wt_pos;
    Front_Request *data;
};

typedef std::vector<char *> TEMPLIST;
typedef std::vector<std::pair<char *, uint8_t>> MERGE_TEMPLIST;

extern char * buffer;

extern SP_Message_Queue mem_pool_wt;

extern std::vector<SP_Message_Queue*> mem_pool_rd;

extern FILE * fp_in;
extern FILE * fp_out;

extern std::vector< Back_Request *> s_vec_back_request;

