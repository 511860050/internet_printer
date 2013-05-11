/********************************************
*FileName: print.cpp
*Date: 2013年 05月 09日 星期四 16:00:22 CST
*Author: chenhuan
*Usage: define the class of Print
********************************************/

#include "print.h"

//============================================
/**
*run ro run the main flow of Print
*1. get address info of printd
*2. connect to the printd
*3. submitFile to the printd
*4. get the return information from printd
*/
void Print::run()
{
  int sockFd; 

  if((sockFd = makeConnectToPrintd()) < 0)
    error("error in makeConenctToPrintd");

  if(submitFile(sockFd) < 0)
    error("error in submitFile");

  close(sockFd);
}

//============================================
/**
*getPrintdName to get the printd name in print.conf
*format : printd name
*return value : 0 = success
*/
char *Print::getPrintdName()
{ 
  return scanConfigFile(CONFIG_FILE, PRINTD_NAME);
}

//============================================
/**
*makeConnectToPrintd to make the connect to print daemon
*1. get the address of printd 
*2. make socket
*3. make conenct to printd
*return value : the sockFd
*/
int Print::makeConnectToPrintd()
{
  char *printdName;
  struct addrinfo *printdAddrList;
  struct addrinfo *printdAddr;
  int sockFd;

  //get host name of printd
  if((printdName = getPrintdName()) == NULL)
    error("error in getPrintdName");
  //get IP address of printd
  printdAddrList = getAddrInfo(printdName , IPP_PORT , 
                  AI_CANONNAME , AF_INET , SOCK_STREAM);
  //make socket and connect
  for(printdAddr = printdAddrList ; printdAddr != NULL ; 
      printdAddr = printdAddr->ai_next)
  {
    if((sockFd = socket(printdAddr->ai_family , 
                 printdAddr->ai_socktype, printdAddr->ai_protocol)) < 0)
      continue;
    else if(connect(sockFd,printdAddr->ai_addr,
            printdAddr->ai_addrlen) < 0)
    {
      close(sockFd); //fucking important
      continue;
    }
    else
    {
      freeaddrinfo(printdAddrList);
      return sockFd;
    }
  }
  return -1;
}

//==================================================
/**
*submitFile to submit the pointed file to printd
*sockFd : the socket to sumbit
*return value : 0 = operate success
*/
int Print::submitFile(int sockFd)
{

  int fd;
  int fileSize;
  int readLen;
  int writeLen;
  struct stat fileInfo;
  char buffer[IO_SIZE];


  //check if the file exist
  if(stat(fileName_ , &fileInfo) < 0)
  {
    perror("file is not exist");
    return -1;
  }
  if(!S_ISREG(fileInfo.st_mode))
  {
    perror("is not a regular file");
    return -1;
  }

  fileSize = fileInfo.st_size; 

  //sumbit the file to printd
  if((fd = open(fileName_ , O_RDONLY)) < 0)
  {
    perror("error in submitFile::open");
    return -1;
  }

  while((readLen = read(fd , buffer , IO_SIZE)) > 0)
  {
    writeLen = writen(sockFd , buffer , readLen);
    if(writeLen != readLen)
    {
      perror("error in submitFile::writen");
      return -1;
    }
  }
  close(fd);
  return 0;
}

//==================================================

int main(int ac , char *av[])
{
  if(ac != 2)
    error("Usage: command [fileName]");

  Print print(av[1]);
  print.run();

  return 0;
}
