/********************************************
*FileName: print.h
*Date: 2013年 05月 09日 星期四 15:45:56 CST
*Author: chenhuan
*Usage: Print to declare a class to send the 
*file to be printed to print_daemon process 
*who communicate with internet printer
********************************************/

#ifndef CHEN_HUAN_PRINT_HEADER
#define CHEN_HUAN_PRINT_HEADER

#include "public_function.h"

//==========================================
/**
*all kinds of define
*/

//#define CONFIG_FILE "~/Hello_World/internet_print/print.conf" 
#define CONFIG_FILE "print.conf"

#define IPP_PORT "13000"
#define SERVICE_NAME "ipp"

#define PRINTD_NAME "printd"

#define TEXT 1 
#define PLAIN_TEXT 0x01 //treat file as plain text

//==========================================
/**
*Print to declare a class to send the file 
*to be printed to print_daemon process who
*communicate with internet printer
*/
class Print
{
public: 
Print(char *fileName , int textFormat)
  :fileName_(fileName) , textFormat_(textFormat) {}

  void run();
private:
  int makeConnectToPrintd();
  int sendPrintRequest(int sockFd);
  int submitFile(int sockFd);
  int receivePrintReply(int sockFd);

  int checkFile(char *fileName);
  char *getPrintdName();
private:
  char *fileName_;
  int textFormat_; //0 = print as PLAIN_TEXT 
}; //end of Print

#endif
