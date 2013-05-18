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

#define LISTEN_QUEUE 50

#define DIRECTORY "/home/chenhuan/Hello_World/internet_print"
#define PRINT_FILE_DIR "file"
#define PRINT_REQUEST_DIR "request"
#define PRINT_LIST_FILE "print_work_list"

#define INITIAL_READ 0
#define RE_READ 1

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
  struct ClientThreadParam
  {
    PrintDaemon *this_;
    int clientFd_;
  };
public:
  PrintDaemon(char *processName);
  ~PrintDaemon();

private:
  void initialize();

  void run();

  int signalInitialize();

  int makeListen();

  static void *signalThread(void *printd);
  void signalProcess();
  void killClientThreads();
  void printWorkList();

  static void *clientThread(void *threadParam);
  int receivePrintRequest(int clientFd);
  int receiveFile(int clientFd);
  int sendPrintReply(int clientFd);

private:
  list<WorkInfo> workList_; //list to hold work informations
  pthread_mutex_t work_list_lock_;  //thread lock to workList_

  int reConfigure_; //RE_READ configure file in communcate with printer
  pthread_mutex_t re_configure_lock_; //thread lock to reRead_;

  sigset_t mask_;
}; //end of PrintDaemon

#endif
