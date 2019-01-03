#pragma once

#include "SP_Data_Block.h"
#include "Global.h"

#include <vector>
#include <string>
#include <mutex>

class CRequest;

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