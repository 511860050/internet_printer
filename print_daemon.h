/********************************************
*FileName: print_daemon.h
*Date: 2013年 05月 10日 星期五 14:19:22 CST
*Author: chenhuan
*Usage: the interface of print_daemon
*receive the file from the print process
*then sumbit the file to printer
********************************************/

#ifndef CHEN_HUAN_PRINT_DAEMON_HEADER
#define CHEN_HUAN_PRINT_DAEMON_HEADER

#include "public_function.h"

#define IPP_PORT "13000"
#define SERVICE_NAME "ipp" 

#define LISTEN_QUEUE 10

#define FILE_DIRECTORY "/home/chenhuan/Hello_World/internet_print/"

//=============================================
/**
*the class of PrintDaemon
*Accept the file from print
*send the file to internet printer
*/
class PrintDaemon
{
public:
  PrintDaemon(char *processName)
    :processName_(processName) {}

  void run();
private:
  int makeListen();
  int receiveFile(int clientFd);
private:
  char *processName_;  //name pf process to initialize daemon
}; //end of PrintDaemon

#endif
