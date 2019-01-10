#pragma once

#include "SP_Data_Block.h"
#include "Global.h"

#include <vector>
#include <string>
#include <mutex>

class CRequest;
class Merge_CRequest;

class Request : public SP_Data_Block
{
public:
    Request();
    virtual ~Request();

public:
    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    size_t length_;
    uint8_t idx_;
    TEMPLIST str_list_[26][26][26];
    size_t single_str_count_[26];
    size_t double_str_count_[26][26];
    std::mutex lock_str_list_[26][26][26];
    std::mutex lock_single_str_[26];
    std::mutex lock_double_str_[26][26];
    size_t count_;
    size_t size_split_buf;
    bool send_str_list_[26][26];
    CRequest* recv_str_list_[26][26];
    FILE *fp_out_;
    FILE *fp_in_;
};

struct Last_Ptr
{
    uint8_t i;
    uint8_t j;
    char * ptr;
};

class Merge_Request : public SP_Data_Block
{
public:
    Merge_Request();
    virtual ~Merge_Request();

public:
    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    size_t length_;
    uint8_t idx_;
    MERGE_TEMPLIST str_list_[26][26][26];
    size_t single_str_count_[26];
    size_t double_str_count_[26][26];
    std::mutex lock_str_list_[26][26][26];
    std::mutex lock_single_str_[26];
    std::mutex lock_double_str_[26][26];
    size_t count_;
    size_t size_split_buf;
    bool send_str_list_[26][26];
    Merge_CRequest* recv_str_list_[26][26];
    FILE *fp_out_;
    FILE *fp_in_;
    std::vector<File_Element> vec_mid_fp_;
    std::vector<std::vector<Buffer_Element*>> vec_buf_;
    size_t vec_char_size_[26][26][32];
    std::vector<std::pair<uint8_t, uint8_t>> vec_last_idx_;
    int split_size_;
    std::vector<Last_Ptr> vec_last_ptr_;
    uint8_t last_idx_[2];
    bool is_read_end_;
};

class CRequest : public SP_Data_Block
{
public:
    CRequest(Request *r)
        :buffer_(NULL)
        ,begin_(0)
        ,end_(0)
        ,idx_(0)
        ,request_(r)
        {};
    ~CRequest(){};

    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    std::vector<uint8_t> idx_;
    Request *request_;
    std::vector<Buffer_Element*> vec_buf_wt_;
};

class Merge_CRequest : public SP_Data_Block
{
public:
    Merge_CRequest(Merge_Request *r)
        :buffer_(NULL)
        ,begin_(0)
        ,end_(0)
        ,idx_(0)
        ,request_(r)
        {};
    ~Merge_CRequest(){};

    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    std::vector<std::pair<uint8_t, uint8_t>> idx_;
    Merge_Request *request_;
    std::vector<Buffer_Element*> vec_buf_wt_;
};