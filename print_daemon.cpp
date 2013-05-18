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
  if(pthread_mutex_destroy(&work_list_lock_) != 0)
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
  if(pthread_mutex_init(&work_list_lock_ , NULL) != 0)
    error("error in pthread_mutex_init");

  reConfigure_ = INITIAL_READ;

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
  int *clientFd;
  ClientThreadParam *threadParam;

  if(signalInitialize() != 0)
    error("error in signalInitialize");

  if((sockFd = makeListen()) < 0)
    error("error in makeListen");

  if(pthread_create(&sigThread,NULL,signalThread, (void*)this) != 0)
    error("error in pthread_create::signalThread");

  while(true)
  {
    clientAddrLen = sizeof(clientAddr);
    /**********this place is fucking important**********/
    clientFd = new int;
    if(clientFd == NULL)
      error("error in new int");

    threadParam = new ClientThreadParam;
    if(threadParam == NULL)
      error("error in new ClientThreadParam");
    /**********this place is fucking important*********/
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
*SIGUSR1 : print the work list
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
        syslog(LOG_INFO , "terminatr with signal SIGTERM");
        exit(0);
      }
      case SIGUSR1:
      {
        printWorkList();
        break;
      }
      default:
      {
        syslog(LOG_ERR,"unexpected signal happened %m");
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
}

//============================================
/**
*print the workList
*using C++ ofstream to complete it
*/
void PrintDaemon::printWorkList()
{
  string fileName;
  stringstream ss;
  time_t ticks;
  char *theTime;

  ss<<DIRECTORY<<"/"<<PRINT_LIST_FILE;

  ofstream ofs(ss.str().c_str());
  if(!ofs)
    error("error in ofs");

  //write in the time
  ticks = time(NULL);
  theTime = ctime(&ticks);
  ofs << theTime <<endl;

  pthread_mutex_lock(&work_list_lock_);

  typedef list<WorkInfo>::const_iterator listCIter;
  for(listCIter iter = workList_.begin() ; iter != workList_.end() ;
      ++iter)
  {
    ofs<<(*iter).fileName_<<endl;
    if(ofs.bad() || ofs.fail())
      error("error in ofs during write");
  }

  pthread_mutex_unlock(&work_list_lock_);

  ofs.close();
}

//=============================================
/**
*clientThread using receiveFile
*It is the pthread function
*/
void *PrintDaemon::clientThread(void *threadParam)
{
  ClientThreadParam *param = (ClientThreadParam*)threadParam;

  if(pthread_mutex_lock(&(param->this_->work_list_lock_)) != 0)
    error("error in receivePrintRequest::pthread_mutex_lock");

  if(param->this_->receivePrintRequest(param->clientFd_) != 0)
    error("error in receivePrintRequest");

  if(param->this_->receiveFile(param->clientFd_) != 0)
    error("error in receiveFile");

  if(param->this_->sendPrintReply(param->clientFd_) != 0)
    error("error in sendPrintReply");

  close(param->clientFd_);

  if(pthread_mutex_unlock(&(param->this_->work_list_lock_)) != 0)
    error("error in receivePrintRequest::pthread_mutex_lock");

  delete param; 

  return NULL;
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
  ss<<DIRECTORY<<"/"<<PRINT_REQUEST_DIR<<"/"<<workList_.size();
  fileName = ss.str();

  ofstream ofs(fileName.c_str());
  if(!ofs)
    error("error in receivePrintRequest::ofs");

  if((length = read(clientFd , &printRequest , sizeof(printRequest)))
               < 0)
    error("error in receivePrintRequest::read");

  printRequest.size_ = ntohl(printRequest.size_);
  printRequest.flags_ = ntohl(printRequest.flags_);

  ofs<<printRequest<<endl;
  ofs.close();

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
  stringstream ss;
  int c;

  //bulid the file name and creat the file
  ss<<DIRECTORY<<"/"<<PRINT_FILE_DIR<<"/"<<workList_.size();
  fileName = ss.str();

  workList_.push_back(WorkInfo(fileName));

  ofstream ofs(fileName.c_str());
  if(!ofs)
    error("error in ifs");

  if((clientFp = fdopen(clientFd , "r")) == NULL)
    error("error in fdopen");

  while((c = getc(clientFp)) != END_SIGN)
    ofs.put(c);

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
  struct PrintReply printReply;

  printReply.resultCode_ = htonl(SUCCESS);

  printReply.jobNumber_ = htonl(workList_.size()-1);

  strcpy(printReply.errorMessage_ , "success");

  if(writen(clientFd , &printReply , sizeof(printReply))
     != sizeof(printReply))
    error("error in sendPrintReply::writen");
  return 0;
}

//===========================================
/**
*makeListen() to make the Print Daemon process
*complete socket , bind and  listen
*return value : 0 = success
*/
int PrintDaemon::makeListen()
{
  int sockFd;
  const int on = 1;
  struct addrinfo* addressList;
  struct addrinfo* address;

  addressList = getAddrInfo(NULL , IPP_PORT , AI_PASSIVE ,
                            AF_UNSPEC , SOCK_STREAM);
  if(addressList == NULL)
    error("error in getAddrInfo");

  for(address = addressList;address != NULL;address = address->ai_next)
  {
    if((sockFd = socket(address->ai_family , address->ai_socktype ,
                 address->ai_protocol)) < 0 )
      continue;
    else if(setsockopt(sockFd , SOL_SOCKET , SO_REUSEADDR , &on ,
            sizeof(on)) == -1)
    {
      close(sockFd);
      continue;
    }
    else if(bind(sockFd , address->ai_addr , address->ai_addrlen) != 0)
    {
      close(sockFd);
      continue;
    }
    else if(listen(sockFd , LISTEN_QUEUE) != 0)
    {
      close(sockFd);
      continue;
    }
    return sockFd;
  }
  return -1;
}

//============================================

int main(int ac , char *av[])
{
  PrintDaemon printd(av[0]);

  return 0;
}
