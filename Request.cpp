#include "Request.h"
#include "Global_Macros.h"

#include <memory.h>

Request::Request()
    :buffer_(0)
    ,begin_(0)
    ,end_(0)
    ,length_(0)
    ,idx_(0)
    ,count_(0)
    ,size_split_buf(0)
{
    memset(single_str_count_, 0, sizeof(single_str_count_));
    memset(double_str_count_, 0, sizeof(double_str_count_));
    for (size_t i = 0; i < 26; ++i)
        for (size_t j = 0; j < 26; ++j)
            for (size_t k = 0; k < 26; ++k)
                str_list_[i][j][k].reserve(STR_LIST_SIZE);
    memset(send_str_list_, 0, sizeof(send_str_list_));
    memset(recv_str_list_, 0, sizeof(recv_str_list_));
}

Request::~Request()
{
}

Front_Request::Front_Request()
    :buffer_(0)
    ,begin_(0)
    ,end_(0)
    ,length_(0)
    ,idx_(0)
    ,count_(0)
    ,size_split_buf(0)
    ,split_size_(0)
    ,is_read_end_(false)
    ,is_split_end_(false)
    ,send_split_count_(0)
    ,recv_split_count_(0)
{
    memset(single_str_count_, 0, sizeof(single_str_count_));
    memset(double_str_count_, 0, sizeof(double_str_count_));
    memset(send_str_list_, 0, sizeof(send_str_list_));
    memset(recv_str_list_, 0, sizeof(recv_str_list_));
}

Back_Request::Back_Request()
    :buffer_(0)
    ,begin_(0)
    ,end_(0)
    ,length_(0)
    ,count_(0)
    ,size_split_buf(0)
    ,is_read_end_(false)
    ,head_single_str_(0)
    ,head_double_str_(0)
    ,head_str_size_(0)
{
    memset(single_str_count_, 0, sizeof(single_str_count_));
    str_list_.resize(26);
    for (size_t i = 0; i < 26; ++i)
        str_list_[i].resize(26);
    for (size_t i = 0; i < 26; ++i)
        for (size_t j = 0; j < 26; ++j)
                str_list_[i][j].reserve(STR_LIST_SIZE);
    memset(send_str_list_, 1, sizeof(send_str_list_));
    memset(recv_str_list_, 0, sizeof(recv_str_list_));
}

