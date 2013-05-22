/********************************************
*FileName: print_daemon.h
*Date: 2013年 05月 10日 星期五 14:19:22 CST
*Author: chenhuan
*Usage: the interface of PrintDaemon
********************************************/

#ifndef CHEN_HUAN_PRINT_DAEMON_HEADER
#define CHEN_HUAN_PRINT_DAEMON_HEADER

#include "public_function.h"

#define DIRECTORY "/home/chenhuan/Hello_World/internet_print"
#define PRINT_FILE_DIR "file"
#define PRINT_REQUEST_DIR "request"
#define PRINT_LIST_FILE "print_work_list"
#define PRINTD_CONFIG_FILE "printd.conf"

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
  struct ClientThreadInfo
  {
    pthread_t threadNumber_;
    int clientFd_;

    ClientThreadInfo(pthread_t threadNumber , int clientFd)
      :threadNumber_(threadNumber) , clientFd_(clientFd) {}
  };
private:
  struct JobInfo
  {
    int jobNumber_;
    struct PrintRequest printRequest_;

    JobInfo()
      :jobNumber_(0) {}
    JobInfo(int jobNumber , struct PrintRequest printRequest)
      :jobNumber_(jobNumber) ,printRequest_(printRequest) {}
  };
private:
  //using for threadFunction variable
  struct ClientThreadParam 
  {
    PrintDaemon *this_;
    int clientFd_;
  };
private:
  //using for thread_cleanup funcion
  struct ClientCleanUpParam
  {
    PrintDaemon *this_;
    pthread_t threadNumber_;
  };
public:
  PrintDaemon(char *processName);
  ~PrintDaemon();
private:
  void initialize();
  void run();
  int signalInitialize();

  //functions to deal with signal thread
  static void *signalThread(void *printd);
  void signalProcess();
  void killClientThreads();
  void printJobList(); 

  //functions to deal with client thread which communicate with print
  static void *clientThread(void *threadParam);
  void clientProcess(int clientFd);
  int receivePrintRequest(int clientFd);
  int receiveFile(int clientFd);
  int sendPrintReply(int clientFd);
  void sendErrorReply(int clientFd , const char *errorFile);
  static void clientCleanUp(void *clientCleanUpParam); 
  void clientCleanUp(pthread_t threadNumber);

  //functions to deal with printer thread which communicate with printer
  static void *printerThread(void *printd);
  void printerProcess();
  char *getPrinterHostName();
  struct JobInfo getNextJob();
  void reReadConfigFile(char *printerHostName);
  int openPrintFile(int jobNumber , struct stat &fileInfo);
  int submitFile(int fd , int sockFd);
  int receivePrinterReply(int sockFd);
private:
  int jobNumber_;  //number of jobs now
  pthread_mutex_t job_number_lock_; //thread lock to jobNumber_

  list<JobInfo> jobList_; //list to hold job informations
  pthread_mutex_t job_list_lock_;  //thread lock to jobkList_

  //list to hold thread information
  list<ClientThreadInfo> clientThreadList_; 
  pthread_mutex_t thread_list_lock_;  //thred lock to threadList_

  int reConfigure_; //RE_READ configure file in communcate with printer
  pthread_mutex_t re_configure_lock_; //thread lock to reRead_;

  sigset_t mask_;
}; //end of PrintDaemon

#endif
