#include "Global.h"

Buffer_Element::Buffer_Element(size_t size)
    :ptr(NULL)
    ,length(size)
    ,rd_pos(0)
    ,wt_pos(0)
{
    ptr = (char *)malloc(sizeof(char) * size); 
}

Buffer_Element::~Buffer_Element()
{
    free(ptr);
}

File_Element::File_Element(FILE * fp)
    :fp_(fp)
    ,size_(0)
    ,rd_pos(0)
    ,wt_pos(0)
    ,data(NULL)
{
}

File_Element::~File_Element()
    {
}