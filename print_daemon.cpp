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
  int pid;
  struct sockaddr clientAddr;
  socklen_t clientAddrLen;

  if(initializeDaemon(processName_ , 0) != 0)
    error("error in initializeDaemon");

  if((sockFd = makeListen()) < 0)
    error("error in makeListen");
 
  signal(SIGCHLD , sig_chld);
  while(true)
  {
    clientAddrLen = sizeof(clientAddr);
    if((clientFd = accept(sockFd , &clientAddr , &clientAddrLen)) < 0)
      continue;
    
    if((pid = fork()) == 0)
    {//child to receive file
      close(sockFd);
      if(receiveFile(clientFd) < 0)
        error("error in receiveFile");
      close(clientFd);
      exit(0);
    }
    close(clientFd);
  }
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
    else if(setsockopt(sockFd , SOL_SOCKET , SO_REUSEADDR ,
            &on , sizeof(on)) == -1)
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

//=============================================
/**
*reveiceFile to receive the file from print process
*store the file in local directory
*return value : 0 = receive success
*/
int PrintDaemon::receiveFile(int clientFd)
{
  char buffer[IO_SIZE];
  char fileName[SIMPLE_SIZE];
  int fd;
  int readLen , writeLen;
  stringstream ss;

  ss << getpid();
  strcpy(fileName , FILE_DIRECTORY);
  strcat(fileName , ss.str().c_str());

  if((fd = creat(fileName , S_IRUSR|S_IWUSR)) < 0) //user read and write
    error("error in create");

  while((readLen = read(clientFd , buffer , IO_SIZE)) > 0)
  {
    writeLen = writen(fd , buffer , readLen);
    if(readLen != writeLen)
      error("error in writen");
  }
  return 0;
}

//============================================

int main(int ac , char *av[])
{
  PrintDaemon printd(av[0]);
  printd.run();

  return 0;
}
