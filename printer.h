//============================================
//FileName: printer.h
//Date: 2013年 05月 21日 星期二 15:42:48 CST
//Author: chenhuan
//Usage: the interface of Printer
//============================================

#ifndef CHEN_HUAN_PRINTER_HEADER 
#define CHEN_HUAN_PRINTER_HEADER 

#include "public_function.h"

//============================================
/**
*the class of Printer
*Accept the file from printd
*/
class Printer
{
public:
  void run();
private:
  int receiveHTTPHeader(int clientFd);
  int receiveIPPHeader(int clientFd);
  int receiveFile(int clientFd);
  int sendReply(int clientFd);
};  //end if Printer

#endif
