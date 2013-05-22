/********************************************
*FileName: print_daemon.cpp
*Date: 2013年 05月 10日 星期五 14:19:33 CST
*Author: chenhuan
*Usage: the definitio of PrintDaemon
********************************************/

#include "print_daemon.h"

//===========================================
/**
*constructor of PrintDaemon
*/
PrintDaemon::PrintDaemon(char *processName)
  :jobNumber_(0) , reConfigure_(INITIAL_READ)
{
//  if(initializeDaemon(processName , 0) != 0)
//    error("error in initializeDaemon");
  initialize();
  run();
}

//=============================================
/**
*deconstructor
*/
PrintDaemon::~PrintDaemon()
{
  if(pthread_mutex_destroy(&job_number_lock_) != 0)
    error("error in pthread_mutex_destroy");

  if(pthread_mutex_destroy(&job_list_lock_) != 0)
    error("error in pthread_mutex_destroy");

  if(pthread_mutex_destroy(&thread_list_lock_) != 0)
    error("error in pthread_mutex_destroy");

  if(pthread_mutex_destroy(&re_configure_lock_) != 0)
    error("error in pthread_mutex_destroy");
}

//===========================================
/**
*initialize the data member
*/
void PrintDaemon::initialize()
{
  if(pthread_mutex_init(&job_number_lock_ , NULL) != 0)
    error("error in pthread_mutex_init");

  if(pthread_mutex_init(&job_list_lock_ , NULL) != 0)
    error("error in pthread_mutex_init");

  if(pthread_mutex_init(&thread_list_lock_ , NULL) != 0)
    error("error in pthread_mutex_init");

  if(pthread_mutex_init(&re_configure_lock_ , NULL) != 0)
    error("error in pthread_mutex_init");
}

//===========================================
/**
*run to complete the main flow of the print daemon
*1. make socket bind listen
*2. accept the connect
*3. receive the file
*4. tell client it is ok or bad
*/
void PrintDaemon::run()
{
  int sockFd ;
  struct sockaddr clientAddr;
  socklen_t clientAddrLen;
  pthread_t thread;
  pthread_t sigThread;
  pthread_t priThread;
  int *clientFd;
  ClientThreadParam *threadParam;

  if(signalInitialize() != 0)
    error("error in signalInitialize");

  if((sockFd = makeTcpListen(PRINTD_PORT , LISTEN_QUEUE)) < 0)
    error("error in makeListen");

  if(pthread_create(&sigThread,NULL,signalThread, (void*)this) != 0)
    error("error in pthread_create::signalThread");

  if(pthread_create(&priThread ,NULL, printerThread , (void*)this) != 0)
    error("error in pthread_create");

  while(true)
  {
    clientAddrLen = sizeof(clientAddr);

    clientFd = new int;
    threadParam = new ClientThreadParam;

    if((*clientFd = accept(sockFd , &clientAddr , &clientAddrLen)) < 0)
      continue;

     threadParam->this_ = this;
     threadParam->clientFd_ = *clientFd;
     delete clientFd; //new and delete

     if(pthread_create(&thread , NULL , clientThread ,
       (void*)threadParam) != 0)
       error("error in pthread_create::clientThread");
   }
}

//=============================================
/**
*intialize signal's process
*/
int PrintDaemon::signalInitialize()
{
  struct sigaction sa;

  //ingnore the SIGPIPE
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_IGN;
  if(sigaction(SIGPIPE , &sa , NULL) < 0)
    error("error in sigaction");
  //block the SIGHUP SIGTERM SIGUSR1
  sigemptyset(&mask_);
  sigaddset(&mask_ , SIGHUP);
  sigaddset(&mask_ , SIGTERM);
  sigaddset(&mask_ , SIGUSR1);
  if(pthread_sigmask(SIG_BLOCK ,&mask_ , NULL) != 0)
    error("error in pthread_sigmask");

  return 0;
}

//=============================================
/**
*signalThread to deal with the pointed signal
*it uses signalProcess() to do so
*/
void *PrintDaemon::signalThread(void *printd)
{
  PrintDaemon *thisOne = (PrintDaemon*)printd;

  thisOne->signalProcess();

  return NULL;
}

//=============================================
/**
*signalProcess to process signal:
*SIGHUP : reread the configure file
*SIGTERM : kill the whole process
*SIGUSR1 : print the job list
*/
void PrintDaemon::signalProcess()
{
  int sigInfo;

  while(true)
  {
    if(sigwait(&mask_ , &sigInfo) != 0)
      error("error in signalProcess::sigwait");

    switch(sigInfo)
    {
      case SIGHUP:
      {
        pthread_mutex_lock(&re_configure_lock_);
        reConfigure_ = RE_READ;
        pthread_mutex_unlock(&re_configure_lock_);
        break;
      }
      case SIGTERM:
      {
        killClientThreads();
        if(ERROR_FLAG == LOG_ON)
          syslog(LOG_INFO , "terminate with signal SIGTERM");
        else
          printf("Terminate with signal SIGTERM\n");
        exit(0);
      }
      case SIGUSR1:
      {
        printJobList();
        break;
      }
      default:
      {
        if(ERROR_FLAG == LOG_ON)
          syslog(LOG_INFO , "Unknown signal received");
        else
          printf("Unknown signal received\n");
        break;
      }
    }
  }
}

//============================================
/**
*killClientThreads - SIGTERM to kill all the childThread
*/
void PrintDaemon::killClientThreads()
{
  if(pthread_mutex_lock(&thread_list_lock_) != 0)
    error("error in pthread_mutex_lock");

  list<ClientThreadInfo>::iterator iter;

  iter=clientThreadList_.begin();
  for(;iter != clientThreadList_.end() ; ++iter)
    pthread_cancel(iter->threadNumber_);

  if(pthread_mutex_unlock(&thread_list_lock_) != 0)
    error("error in pthread_mutex_unlock");
}

//============================================
/**
*print the workList
*using C++ ofstream to complete it
*store the result into DIRECTORY/PRINT_LIST_FILE 
*/
void PrintDaemon::printJobList() 
{
  string fileName;
  stringstream ss;
  time_t ticks;
  char *theTime;

  ss<<DIRECTORY<<"/"<<PRINT_LIST_FILE;

  ofstream ofs(ss.str().c_str());
  if(!ofs)  error("error in ofs");

  //write in the time
  ticks = time(NULL);
  theTime = ctime(&ticks);
  ofs << theTime <<endl;

  /************deal with list<JobInfo>***********/
  if(pthread_mutex_lock(&job_list_lock_) != 0)
    error("error in pthread_mutex_lock");

  typedef list<JobInfo>::const_iterator listCIter;
  for(listCIter iter = jobList_.begin() ; iter != jobList_.end() ;
      ++iter)
  {
    ofs<<(*iter).jobNumber_<<endl;
    if(ofs.bad() || ofs.fail())
      error("error in ofs during write");
  }

  if(pthread_mutex_unlock(&job_list_lock_) != 0)
    error("error in pthread_mutex_lock");
  /*********************************************/
  ofs.close();
}

//==============================================
/**
*clientCleanUp to call clientThreadCleanup to
*remove the ThreadInfo from list and close the clientFd
*/
void PrintDaemon::clientCleanUp(void *clientCleanUpParam)
{
  ClientCleanUpParam *param;

  param = (ClientCleanUpParam*)clientCleanUpParam;

  param->this_->clientCleanUp(param->threadNumber_);
}

//=============================================
/**
*clientCleanUp - clean up the clientPthread
*remove the thread from threadList_
*/
void PrintDaemon::clientCleanUp(pthread_t threadNumber)
{
  list<ClientThreadInfo>::iterator iter;
  
  if(pthread_mutex_lock(&thread_list_lock_) != 0)
    error("error in pthread_mutex_lock");

  iter = clientThreadList_.begin();
  for( ; iter != clientThreadList_.end() ; ++iter)
  {
    if((*iter).threadNumber_ == threadNumber)
      break;
  }
  //close the clientFd here
  close((*iter).clientFd_);
  clientThreadList_.erase(iter);

  if(pthread_mutex_unlock(&thread_list_lock_) != 0)
    error("error in pthread_mutex_lock");
}

//=============================================
/**
*clientThread using receiveFile
*It is the pthread function
*/
void *PrintDaemon::clientThread(void *threadParam)
{
  ClientThreadParam *param;

  param = (ClientThreadParam*)threadParam;
  param->this_->clientProcess(param->clientFd_);

  delete param;
  return NULL;
}

//=============================================
/**
*clientProcess to complete the communicate 
*between print and printed
*/
void PrintDaemon::clientProcess(int clientFd )
{
  pthread_t threadNumber;
  ClientCleanUpParam clientCleanUpParam;

  //build the ClientCleanUpParam to clientCleanUp
  threadNumber = pthread_self();  
  clientCleanUpParam.this_ = this;
  clientCleanUpParam.threadNumber_ = threadNumber;

  pthread_cleanup_push(clientCleanUp, (void*)&clientCleanUpParam); 

  //push the ClientThreadInfo to clientThreadList_
  if(pthread_mutex_lock(&(thread_list_lock_)) != 0)
    error("error in receivePrintRequest::pthread_mutex_lock");

  clientThreadList_.push_back(ClientThreadInfo(threadNumber,clientFd));

  if(pthread_mutex_unlock(&(thread_list_lock_)) != 0)
    error("error in receivePrintRequest::pthread_mutex_lock");

  //communicate between print and printd
  if(pthread_mutex_lock(&(job_list_lock_)) != 0)
    error("error in receivePrintRequest::pthread_mutex_lock");

  if(receivePrintRequest(clientFd) != 0)
    error("error in receivePrintRequest");

  if(receiveFile(clientFd) != 0)
    error("error in receiveFile");

  if(sendPrintReply(clientFd) != 0)
    error("error in sendPrintReply");

  if(pthread_mutex_unlock(&(job_list_lock_)) != 0)
    error("error in receivePrintRequest::pthread_mutex_lock");

  pthread_cleanup_pop(1);  //clean up the thread
}

//=============================================
/***
*sendErrorReply send the errorMessage(PrintReply)
*to print
*/
void PrintDaemon::sendErrorReply(int clientFd , const char* errorFile)
{
  struct PrintReply printReply;

  //build the PrintReply and send to print
  printReply.jobNumber_ = 0;
  if(errno != EIO) 
    printReply.resultCode_ = htonl(errno);
  else
    printReply.resultCode_ = htonl(EIO);
  strncpy(printReply.errorMessage_ , strerror(printReply.resultCode_),
          MSGLEN_MAX);

  writen(clientFd , &printReply , sizeof(printReply));

  //report the error to server
  if(ERROR_FLAG == LOG_ON)
    syslog(LOG_ERR , "error in %s" , errorFile);
  else
    printf("error in %s\n" , errorFile);
}

//=============================================
/**
*receivePrintRequest to receive the print request
*from the print , and to save it in pointed file
return value : 0 = success
*/
int PrintDaemon::receivePrintRequest(int clientFd)
{
  int length;
  string fileName;
  stringstream ss;
  struct PrintRequest printRequest;

  /*********receive string format PrintRequest*******/
  //build the file name
  if(pthread_mutex_lock(&job_number_lock_) != 0)  //lock
    error("error in pthread_mutex_lock");

  ss<<DIRECTORY<<"/"<<PRINT_REQUEST_DIR<<"/"<<jobNumber_;

  if(pthread_mutex_unlock(&job_number_lock_) != 0)  //unlock
    error("error in pthread_mutex_lock");

  fileName = ss.str();
  ofstream ofs(fileName.c_str());
  if(!ofs) 
  {
    sendErrorReply(clientFd , "receivePrintRequest");
    pthread_exit((void*)1);
  }

  if((length=readnTime(clientFd , &printRequest , 
      sizeof(printRequest),WAIT_TIME))  != sizeof(printRequest))
  {
    sendErrorReply(clientFd , "receivePrintRequest");
    pthread_exit((void*)1);
  }

  printRequest.size_ = ntohl(printRequest.size_);
  printRequest.flags_ = ntohl(printRequest.flags_);

  ofs<<printRequest<<endl;
  ofs.close();

  /************push the JobInfo into jobList_***************/
  jobList_.push_back(JobInfo(this->jobNumber_ , printRequest));
  /*********************************************************/
  return 0;
}

//=============================================
/**
*receiveFile to receive the file from print process
*store the file in local directory
*return value : 0 = receive success
*/
int PrintDaemon::receiveFile(int clientFd)
{
  FILE *clientFp;
  string fileName;
  string requestName;
  stringstream ss_1;
  stringstream ss_2;
  int c;

  //bulid the file name and creat the file
  if(pthread_mutex_lock(&job_number_lock_) != 0)
    error("error in pthread_mutex_lock");

  ss_1<<DIRECTORY<<"/"<<PRINT_FILE_DIR<<"/"<<jobNumber_;
  ss_2<<DIRECTORY<<"/"<<PRINT_REQUEST_DIR<<"/"<<jobNumber_;

  if(pthread_mutex_unlock(&job_number_lock_) != 0)
    error("error in pthread_mutex_lock");

  fileName = ss_1.str();
  requestName = ss_2.str();

  ofstream ofs(fileName.c_str());
  if(!ofs) 
  {
    sendErrorReply(clientFd , "receiveFile");
    unlink(requestName.c_str()); //remove the request file
    pthread_exit((void*)1);
  }

  if((clientFp = fdopen(clientFd , "r")) == NULL)
    error("error in fdopen");

  while((c = getc(clientFp)) != END_SIGN)
    ofs.put(c);

  //check if error during read file
  if(c != END_SIGN)
  {
    sendErrorReply(clientFd , "receiveFile");
    //remove the file and the request
    unlink(fileName.c_str());
    unlink(requestName.c_str());
    ofs.close();
    pthread_exit((void*)1);
  }
  
  ofs.close();

  return 0;
}

//=============================================
/**
*sendPrintReply to send the PrintReply to print
*return value : 0 = success
*/
int PrintDaemon::sendPrintReply(int clientFd)
{
  string fileName;
  string requestName;
  stringstream ss_1;
  stringstream ss_2;
  struct PrintReply printReply;

  //bulid the file name and creat the file
  if(pthread_mutex_lock(&job_number_lock_) != 0)
    error("error in pthread_mutex_lock");

  ss_1<<DIRECTORY<<"/"<<PRINT_FILE_DIR<<"/"<<jobNumber_;
  ss_2<<DIRECTORY<<"/"<<PRINT_REQUEST_DIR<<"/"<<jobNumber_;

  if(pthread_mutex_unlock(&job_number_lock_) != 0)
    error("error in pthread_mutex_lock");

  printReply.resultCode_ = htonl(SUCCESS);

  printReply.jobNumber_ = htonl(jobNumber_);

  strcpy(printReply.errorMessage_ , "success");

  if(writen(clientFd , &printReply , sizeof(printReply))
     != sizeof(printReply))
  {
    sendErrorReply(clientFd , "sendPrintReply");
    //remove the file and the request
    unlink(fileName.c_str());
    unlink(requestName.c_str());
    pthread_exit((void*)1);
  }

  //***jobNumber_ update here***/
  jobNumber_++;
  //****************************/
  return 0;
}

//=============================================
/**
*printerThread to drive the printerProcess to
*complete the communication between printer and printd
*/
void *PrintDaemon::printerThread(void *printd)
{
  PrintDaemon *thisOne;

  thisOne = (PrintDaemon*)printd;
  thisOne->printerProcess(); 
  return NULL;
}

//=============================================
/**
*printerProcees to complete the communication
*between printer and printd
*/
void PrintDaemon::printerProcess()
{
  int fd, sockFd;
  int ippLength, httpLength;
  char *printerName;
  char ibuf[IPP_HEADER_SIZE];
  char hbuf[HTTP_HEADER_SIZE];
  struct JobInfo jobInfo;
  struct stat fileInfo;

  //get the printer host name
  if((printerName = getPrinterHostName()) == NULL)
    error("error in getPrinterHostName");

  while(true)
  {
    //get the next Job to print
    jobInfo = getNextJob();
    //check for reconstruct the printerName
    reReadConfigFile(printerName);
    //open and check print file
    if((fd = openPrintFile(jobInfo.jobNumber_ , fileInfo)) < 0)
      error("error in openPrintFile");
    //make connect to printer
    if((sockFd = makeTcpConnect(printerName , PRINTER_PORT)) < 0)
      error("error in makeConnect");
    //setup IPP header
    ippLength = setupIPPHeader(ibuf , jobInfo.jobNumber_ , printerName , 
                              stringToInt(PRINTER_PORT));
    //setup HTTP header
    httpLength = setupHTTPHeader(hbuf , ippLength+fileInfo.st_size ,
                                printerName , stringToInt(PRINTER_PORT));
    //send the HTTP and IPP header
    if(writen(sockFd , hbuf , httpLength) != httpLength)
      error("error in writen::HTTP");
    if(writen(sockFd , ibuf , ippLength) != ippLength)
      error("error in writen::IPP");
    //send the file to printer
    if(submitFile(fd , sockFd) < 0)
      error("error in sendFile");
    if(receivePrinterReply(sockFd) < 0)
      error("error in receivePrinterReply");
  }
}

//=============================================
/**
*send the file to printer
*return value : 0 = success
*/
int PrintDaemon::submitFile(int fd , int sockFd)
{
  FILE *fp;
  FILE *sockFp;
  int c;

  //sumbit the file to printd
  if((fp = fdopen(fd , "r")) == NULL)
    error("error in submitFile::fopen");

  if((sockFp = fdopen(sockFd , "w")) == NULL)
    error("error in submitFile::fdopen");

  while((c = getc(fp)) != EOF)
    fputc(c , sockFp);

  fputc('\r',sockFp);
  fputc('\n',sockFp);
  fputc(END_SIGN , sockFp);
  fflush(sockFp);
  fclose(fp);

  return 0;
}

//=============================================
/**
*receive the reply from the printer
*/
int PrintDaemon::receivePrinterReply(int sockFd)
{
  return 0;
}

//=============================================
/**
*openPrintFile to check if the jobNumber's file is exist 
*return value : file descriptor of the file
*/
int PrintDaemon::openPrintFile(int jobNumber , struct stat &fileInfo)
{
  stringstream ss;;
  string fileName;

  ss<<DIRECTORY<<"/"<<PRINT_FILE_DIR<<"/"<<jobNumber;
  fileName = ss.str();

  if(stat(fileName.c_str() , &fileInfo) != 0)
    error("error in checkPrintFile::stat");

  return open(fileName.c_str() , O_RDONLY);
}

//=============================================
/**
*read the printd.conf to read the printer hostname
*/
char *PrintDaemon::getPrinterHostName()
{
  char *printerHostName;

  printerHostName = scanConfigFile(PRINTD_CONFIG_FILE , "printer");

  return printerHostName;
}

//=============================================
/**
*checkConfigFile to check if the config file needed
*to reread , on recevie signal SIGHUP to reread the 
*configure file
*/
void PrintDaemon::reReadConfigFile(char *printerHostName)
{
  if(pthread_mutex_lock(&re_configure_lock_) != 0)
    error("error in pthread_mutex_lock");

  if(reConfigure_ == RE_READ)
  {
    free(printerHostName); //keep free in mind

    if((printerHostName = getPrinterHostName()) != NULL)
      error("error in reReadConfigFIle::getPrinterHostName");
  }

  if(pthread_mutex_unlock(&re_configure_lock_) != 0)
    error("error in pthread_mutex_lock");
}

//=============================================
/**
*getNextJob to get the next job to print
*return the struct JobInfo 
*/
struct PrintDaemon::JobInfo PrintDaemon::getNextJob()
{
  struct JobInfo jobInfo;

  while(jobList_.size() == 0)  //waiting until the jobList_.size() != 0
    ;
  //get the jobInfo to print
  if(pthread_mutex_lock(&job_list_lock_) != 0)
    error("error in pthread_mutex_lock");
  if(pthread_mutex_lock(&job_number_lock_) != 0)
    error("error in pthread_mutex_lock");

  jobInfo = jobList_.front();
  jobList_.erase(jobList_.begin());
  jobNumber_--;

  if(pthread_mutex_unlock(&job_number_lock_) != 0)
    error("error in pthread_mutex_unlock");
  if(pthread_mutex_unlock(&job_list_lock_) != 0)
    error("error in pthread_mutex_unlock");

  return jobInfo;
}

//============================================

int main(int ac , char *av[])
{
  PrintDaemon printd(av[0]);
  return 0;
}
