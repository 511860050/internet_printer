//============================================
//FileName: printer.cpp
//Date: 2013年 05月 21日 星期二 22:19:56 CST
//Author: chenhuan
//Usage: the defination of Printer
//============================================

#include "printer.h"

//============================================
/**
*run() to run the main flow of the Printer
*1. make listen
*2. receive the HTTP and IPP header
*3. receive file 
*4. send the reply back
*/
void Printer::run()
{
  int sockFd;
  int clientFd;
  struct sockaddr clientAddr;
  socklen_t clientAddrLen;

  if((sockFd = makeTcpListen(PRINTER_PORT , LISTEN_QUEUE)) < 0)
    error("error in makeTcpListen");

  while(true)
  {
    if((clientFd = accept(sockFd , &clientAddr , &clientAddrLen)) < 0)
      error("error in clientFd");

    FILE *clientFp;
    char line[SIMPLE_SIZE];

    if((clientFp = fdopen(clientFd , "r")) == NULL)
      error("error in fdopen");

    //read the HTTP Header
    while(fgets(line , SIMPLE_SIZE , clientFp) != NULL && 
          strcmp(line , "\r\n") != 0)
      cout<<line;
    cout<<"=========================================="<<endl;

    //read the IPP Header
    while(fgets(line , SIMPLE_SIZE , clientFp) != NULL && 
          strcmp(line , "\r\n") != 0)
      cout<<line;
    cout<<"=========================================="<<endl;

    //read the file
    while(fgets(line , SIMPLE_SIZE , clientFp) != NULL && 
          strcmp(line , "255") != 0)
      cout<<line;
    cout<<"=========================================="<<endl;

    close(clientFd);
  }
}

//==============================================
/**
*receiveHeader to receive the HTTP header from printd
*The point is what do I use this message for?
*return value : 0 = success
*/
int Printer::receiveHTTPHeader(int clientFd)
{
  FILE *clientFp;
  char line[SIMPLE_SIZE];

  if((clientFp = fdopen(clientFd , "r")) == NULL)
    error("error in fdopen");

  while(fgets(line , SIMPLE_SIZE , clientFp) != NULL && 
        strcmp(line , "\r\n") != 0)
    cout<<line;

  return 0;
}

//==============================================
/**
*receiveHeader to receive the IPP header from printd
*return value : 0 = success
*/
int Printer::receiveIPPHeader(int clientFd)
{
  FILE *clientFp;
  char line[SIMPLE_SIZE];

  if((clientFp = fdopen(clientFd , "r")) == NULL)
    error("error in fdopen");

  while(fgets(line , SIMPLE_SIZE , clientFp) != NULL && 
        strcmp(line , "\r\n") != 0)
    cout<<line;

  return 0;
}

//=================================================
/**
*receiveFile to receive the main text of HTTP
*return value : 0 = success
*/
int Printer::receiveFile(int clientFd)
{
  FILE *clientFp;
  char line[SIMPLE_SIZE];

  if((clientFp = fdopen(clientFd , "r")) == NULL)
    error("error in fdopen");

  while(fgets(line , SIMPLE_SIZE , clientFp) != NULL && 
        strcmp(line , "255") == 0) ;
  cout<<line; 

  return 0;
}

//=====================================================

int main()
{
  Printer printer;
  printer.run();

  return 0;
}
