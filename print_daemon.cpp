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
  extern pthread_mutex_t work_list_lock;

  if(initializeDaemon(processName_ , 0) != 0)
    error("error in initializeDaemon");

  if((sockFd = makeListen()) < 0)
    error("error in makeListen");
 
  if(pthread_mutex_init(&work_list_lock , NULL) != 0)
    error("error in pthread_mutex_init");

  //SIGUSR1 to print the work list
  signal(SIGUSR1 , sig_usr_1); 
  if(pthread_create(&thread,NULL,printWorkListThread,(void*)this) != 0)
    error("error in pthread_create::printWorkListThread");

  while(true)
  {
    clientAddrLen = sizeof(clientAddr);
    if((clientFd = accept(sockFd , &clientAddr , &clientAddrLen)) < 0)
      continue;
    
     threadParam.this_ = this;  //too nice to say
     threadParam.clientFd_ = clientFd;
     
     if(pthread_create(&thread,NULL ,receiveFileThread ,
        (void*)&threadParam) != 0)
       error("error in pthread_create::receiveFileThread");
   }

   if(pthread_mutex_destroy(&work_list_lock) != 0)
     error("error in pthread_mutex_destroy");
}

//=============================================
/**
*receiveFileThread using receiveFile 
*It is the pthread function
*/
void *PrintDaemon::receiveFileThread(void *threadParam)
{
  if(pthread_mutex_lock(&work_list_lock) != 0)
    error("error in pthread_mutex_lock");

  ThreadParam *param = (ThreadParam*)threadParam;

  if(param->this_->receivePrintRequest(param->clientFd_) != 0)
    error("error in receivePrintRequest");

  if(param->this_->receiveFile(param->clientFd_) != 0)
    error("error in receiveFile");

  if(param->this_->sendPrintReply(param->clientFd_) != 0)
    error("error in sendPrintReply");

  close(param->clientFd_);

  if(pthread_mutex_unlock(&work_list_lock) != 0)
    error("error in pthread_mutex_unlock");

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
  int jobNumber;
  string fileName;
  stringstream ss;
  struct PrintRequest printRequest;
  char receiveReply[SIMPLE_SIZE];
  
  /*********receive string format PrintRequest*******/
  //build the file name 
  jobNumber = workList_.size();
  ss<<DIRECTORY<<"/"<<PRINT_REQUEST<<"/"<<jobNumber;
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

  //send a reply : RECEIVE_HEAD
  strcpy(receiveReply , RECEIVE_HEAD);
  if(writen(clientFd , receiveReply , strlen(receiveReply)) 
            < 0)
    error("error in receivePrintRequest::writen");

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
  ss<<DIRECTORY<<"/"<<PRINT_FILE<<"/"<<pthread_self();
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
    error("error in sendPrintRequest::writen");
  return 0;
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
      if(pthread_mutex_lock(&work_list_lock) != 0)
        error("error in pthread_mutex_lock");

      thisOne->printWorkList();
      PRINT_WORK_LIST = PRINT_OFF;

      if(pthread_mutex_unlock(&work_list_lock) != 0)
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

  ss<<DIRECTORY<<PRINT_LIST_FILE;

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
