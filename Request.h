#pragma once

#include "SP_Data_Block.h"
#include "Global.h"

#include <vector>
#include <string>
#include <mutex>

using namespace std;

class CRequest;
class Back_CRequest;
class Front_CRequest;

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

class Front_Request : public SP_Data_Block
{
public:
    Front_Request();
    virtual ~Front_Request(){};

public:
    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    size_t length_;
    uint8_t idx_;
    size_t single_str_count_[26];
    size_t double_str_count_[26][26];
    std::mutex lock_str_list_[26][26];
    std::mutex lock_single_str_[26];
    std::mutex lock_double_str_[26][26];
    size_t count_;
    size_t size_split_buf;
    bool send_str_list_[26][26];
    Front_CRequest* recv_str_list_[26][26];
    FILE *fp_out_;
    FILE *fp_in_;
    vector<vector<File_Element*>> vec_mid_fp_;
    vector<vector<vector<Buffer_Element*>>> vec_buf_;
    int split_size_;
    bool is_read_end_;
    bool is_split_end_;
    size_t send_split_count_;
    size_t recv_split_count_;
};

class Front_CRequest : public SP_Data_Block
{
public:
    Front_CRequest(Front_Request *r)
        :buffer_(NULL)
        ,begin_(0)
        ,end_(0)
        ,idx_(0)
        ,request_(r)
        {};
    ~Front_CRequest(){};

    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    std::vector<std::pair<uint8_t, uint8_t>> idx_;
    Front_Request *request_;
    std::vector<std::vector<Buffer_Element*>> vec_buf_wt_;
};

class Back_Request : public SP_Data_Block
{
public:
    Back_Request();
    ~Back_Request(){};

    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    size_t length_;
    std::vector<uint8_t> idx_;
    Front_Request *request_;
    vector<vector<TEMPLIST>> str_list_;
    size_t single_str_count_[26];
    std::mutex lock_str_list_[26][26];
    std::mutex lock_single_str_[26];
    size_t count_;
    size_t size_split_buf;
    int is_read_end_;
    char head_str_[128];
    uint8_t head_str_size_;
    bool send_str_list_[26][26];
    Back_CRequest* recv_str_list_[26][26];
    FILE *fp_in_;
    FILE *fp_out_;
    size_t head_single_str_;
    size_t head_double_str_;
    vector<Buffer_Element*> head_buf_;
};

class Back_CRequest : public SP_Data_Block
{
public:
    Back_CRequest(Back_Request *r)
        :buffer_(NULL)
        ,begin_(0)
        ,end_(0)
        ,idx_(0)
        ,request_(r)
        {};
    ~Back_CRequest(){};

    std::mutex lock_;
    char * buffer_;
    size_t begin_;
    size_t end_;
    std::vector<uint8_t> idx_;
    Back_Request *request_;
    std::vector<Buffer_Element*> vec_buf_wt_;
};