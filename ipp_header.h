//============================================
//FileName: ipp_header.h
//Date: 2013年 05月 21日 星期二 14:31:53 CST
//Author: chenhuan
//Usage: IPP protocol header
//============================================

#ifndef CHEN_HUAN_IPP_HEADER_HEADER
#define CHEN_HUAN_IPP_HEADER_HEADER

#include <stdlib.h>

//==============================================
/**
*define Operation IDs
*/
#define OP_PRINT_JOB 0x02

//==============================================
/**
*define Attribute Tags
*/
#define TAG_OPERATION_ATTR 0x01
#define TAG_CHARSET 0x47
#define TAG_NATULANG 0x48
#define TAG_URI 0X45
#define TAG_END_OF_ATTR 0x03

//==============================================
/**
*IPP header 
*/
struct ipp_hdr
{
  int8_t major_version;  //always 1
  int8_t minor_version;  //always 1
  union
  {
    int16_t op;  //operation ID
    int16_t st;  //status
  } u;
  int32_t request_id;  //request ID
  char attr_group[1];  //start of optional arrtibutes group
  /*optional data follows*/
};

#define operation u.op
#define status u.st

#endif
