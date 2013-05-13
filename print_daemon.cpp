/********************************************
*FileName: print_daemon.cpp
*Date: 2013年 05月 10日 星期五 14:19:33 CST
*Author: chenhuan
*Usage: the definitio of PrintDaemon
********************************************/

#include "print_daemon.h"

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
  int sockFd , clientFd;
  struct sockaddr clientAddr;
  socklen_t clientAddrLen;
  pthread_t thread;
  ThreadParam threadParam;  //too nice to say 
  extern pthread_mutex_t threadLock;

  if(initializeDaemon(processName_ , 0) != 0)
    error("error in initializeDaemon");

  if((sockFd = makeListen()) < 0)
    error("error in makeListen");
 
  if(pthread_mutex_init(&threadLock , NULL) != 0)
    error("error in pthread_mutex_init");

  signal(SIGUSR1 , sig_usr_1);

  //thread to receive the signal SIGUSR1 to print workList_ 
  if(pthread_create(&thread,NULL,printWorkListThread,(void*)this) != 0)
    error("error in pthread_create::printWorkListThread");

  while(true)
  {
    clientAddrLen = sizeof(clientAddr);
    if((clientFd = accept(sockFd , &clientAddr , &clientAddrLen)) < 0)
      continue;
    
     threadParam.me_ = this;  //too nice to say
     threadParam.clientFd_ = clientFd;
     
     if(pthread_create(&thread,NULL ,receiveFileThread ,
        (void*)&threadParam) != 0)
       error("error in pthread_create::receiveFileThread");
   }

   if(pthread_mutex_destroy(&threadLock) != 0)
     error("error in pthread_mutex_destroy");
}

//=============================================
/**
*deadSignal to deal with the singal
*/
void *PrintDaemon::printWorkListThread(void *printd)
{
  extern int PRINT_WORK_LIST;
  PrintDaemon *thisOne;

  PRINT_WORK_LIST = PRINT_OFF;
  thisOne = (PrintDaemon*)printd;

  while(true)
  {
    if(PRINT_WORK_LIST == PRINT_ON)
    {
      if(pthread_mutex_lock(&threadLock) != 0)
        error("error in pthread_mutex_lock");

      thisOne->printWorkList();
      PRINT_WORK_LIST = PRINT_OFF;

      if(pthread_mutex_unlock(&threadLock) != 0)
        error("error in pthread_mutex_unlock");
    }
  } 
  return NULL;
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

  ss<<FILE_DIRECTORY<<PRINT_LIST_FILE;

  ofstream ofs(ss.str().c_str());
  if(!ofs)
    error("error in ofs");

  typedef list<WorkInfo>::const_iterator listCIter;
  for(listCIter iter = workList_.begin() ; iter != workList_.end() ; 
      ++iter)
  {
    ofs<<(*iter).fileName_<<endl;
    if(ofs.bad() || ofs.fail())
      error("error in ofs during write");
  }
  ofs.close();
}

//=============================================
/**
*receiveFileThread using receiveFile 
*It is the pthread function
*/
void *PrintDaemon::receiveFileThread(void *threadParam)
{
  if(pthread_mutex_lock(&threadLock) != 0)
    error("error in pthread_mutex_lock");

  ThreadParam *param = (ThreadParam*)threadParam;
  param->me_->receiveFile(param->clientFd_); //the point

  if(pthread_mutex_unlock(&threadLock) != 0)
    error("error in pthread_mutex_unlock");

  return NULL;
}

//=============================================
/**
*receiveFile to receive the file from print process
*store the file in local directory
*return value : 0 = receive success
*/
int PrintDaemon::receiveFile(int clientFd)
{
  char buffer[IO_SIZE];
  string fileName;
  int fd;
  int readLen , writeLen;
  stringstream ss;

  ss << FILE_DIRECTORY<<pthread_self();
  fileName = ss.str();

  if((fd = creat(fileName.c_str() , S_IRUSR|S_IWUSR)) < 0) 
    error("error in create");

  workList_.push_back(WorkInfo(fileName));

  while((readLen = read(clientFd , buffer , IO_SIZE)) > 0)
  {
    writeLen = writen(fd , buffer , readLen);
    if(readLen != writeLen)
      error("error in writen");
  }
  close(clientFd);
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

  addressList = getAddrInfo(NULL,IPP_PORT,
                AI_PASSIVE , AF_UNSPEC , SOCK_STREAM);
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
  printd.run();

  return 0;
}
