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

#define DIRECTORY "/home/chenhuan/Hello_World/internet_print"
#define PRINT_FILE "file"
#define PRINT_REQUEST "request"
#define PRINT_LIST_FILE "work_list"

int PRINT_WORK_LIST;
#define PRINT_ON 0
#define PRINT_OFF 1

pthread_mutex_t work_list_lock;  //thread lock

//=============================================
/**
*sig_usr_1 to receive the SIGUSR1
*set the PRINT_WORK_LIST to be OVER
*the PrintDaemon can be killed
*/
void sig_usr_1(int sigusr1)
{
  extern int PRINT_WORK_LIST;
  PRINT_WORK_LIST = PRINT_ON;
}

//=============================================
/**
*the class of PrintDaemon
*Accept the file from print
*send the file to internet printer
*/
class PrintDaemon
{
private:
  struct WorkInfo
  {
    string fileName_;

    WorkInfo(string fileName)
      :fileName_(fileName) {}
  }; 
private:
  struct ThreadParam
  {
    PrintDaemon *this_;
    int clientFd_;
  };
public:
  PrintDaemon(char *processName)
    :processName_(processName) {}

  void run();
private:
  int makeListen();
  
  static void *receiveFileThread(void *threadParam);
  int receivePrintRequest(int clientFd);
  int receiveFile(int clientFd);
  int sendPrintReply(int clientFd);

  static void *printWorkListThread(void *printd);
  void printWorkList();
private:
  char *processName_;  //name pf process to initialize daemon
  list<WorkInfo> workList_; //list to hold work informations
}; //end of PrintDaemon

#endif
