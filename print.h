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

//==========================================
/**
*Print to declare a class to send the file 
*to be printed to print_daemon process who
*communicate with internet printer
*/
class Print
{
public: 
  Print(char *fileName)
    :fileName_(fileName) {}

  void run();
private:
  int checkFile(char *fileName);
  int makeConnectToPrintd();
  char *getPrintdName();
  int submitFile(int sockFd);
private:
  char *fileName_;
}; //end of Print

#endif
