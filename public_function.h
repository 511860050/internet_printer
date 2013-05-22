/********************************************
*FileName: public_function.h
*Date: 2013年 05月 09日 星期四 15:54:39 CST
*Author: chenhuan
*Usage: define the functions be used in common
********************************************/

#ifndef CHEN_HUAN_PUBLIC_FUNCITON_HEADER
#define CHEN_HUAN_PUBLIC_FUNCITON_HEADER

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ipp_header.h"

using namespace std;

//===============================================
/**
*all kinds of define
*/
#define SIMPLE_SIZE 150 //simple buffer size to read simple message 
#define IPP_HEADER_SIZE 512  //buffer size of ipp struct
#define HTTP_HEADER_SIZE 512  //buffer size of http struct

#define MSGLEN_MAX 512

#ifndef HOST_NAME_MAX  
#define HOST_NAME_MAX 256  //the max length of host name
#endif

#define MAX_KEY_LEN 16   //max key length in CONFIG_FILE
#define MAX_CONF_LEN 512  //max config line in CONFIG_FILE
#define MAX_FORM_LEN 16 //max format length in CONFIG_FILE

#define ROOT_DIRECTORY "/"  //root directory

#define SUCCESS 0  //for PrintReply.resultCode
#define FAILURE 1

int ERROR_FLAG;
#define LOG_ON 1

#define RECEIVE_HEAD "%%"
#define END_SIGN 255 

#define WAIT_TIME 10

#define PRINTD_PORT "13000"
#define PRINTER_PORT "14000"

#define LISTEN_QUEUE 50

//==============================================
/**
*PrintRequest is the header information
*print send to the print_daemon
*/
struct PrintRequest
{
  long size_;
  long flags_;
  char userName_[HOST_NAME_MAX];
  char fileName_[SIMPLE_SIZE];
};

//==============================================
/**
*overload the ostream PrintRequest
*/
ostream& operator<<(ostream& os , const PrintRequest& printRequest)
{
  os<<"size : "<<printRequest.size_<<endl;
  os<<"flags : "<<printRequest.flags_<<endl;
  os<<"user name : "<<printRequest.userName_<<endl;
  os<<"file name : "<<printRequest.fileName_;

  return os;
}

//==============================================
/**
*PrintReply is the header information
*printd send to the print
*/
struct PrintReply
{
  long resultCode_; //SUCCESS or FAILURE
  long jobNumber_; 
  char errorMessage_[SIMPLE_SIZE];
};

//==============================================
/**
*overload the ostream PrintReply
*/
ostream& operator<<(ostream& os , const PrintReply& printReply)
{
  os<<"result code : "<<printReply.resultCode_<<endl;
  os<<"job number : "<<printReply.jobNumber_<<endl;
  os<<"error message : "<<printReply.errorMessage_;

  return os;
}

//==============================================
/**
*send the error message then exit
*/
void error(const char* message)
{
  if(ERROR_FLAG == LOG_ON)
    syslog(LOG_ERR , "%s %m" , message);
  else
    perror(message);
  exit(-1);
}

//=============================================
/**
*string to int
*/
int stringToInt(const char *s)
{
  stringstream ss;
  int number;

  ss<<s;
  ss>>number;

  return number;
}

//==============================================
/**
*sig_chld to release all the child process 
*avoid to be zombie
*/
void sig_chld(int sigchld)
{
  pid_t pid;
  int stat;
  while(1)
  {
    pid = waitpid(-1 , &stat , WNOHANG);
    if(pid <= 0) break;
  }
}

//==============================================
/**
*get the IP address info
*host : the host name of host
*serv : the service or the post number of process
*flags : ai_flags in addrinfo eg. AI_CANONNAME
*family : address family eg. AF_INET
*sockType : socket type eg. SOCK_STREAM
*return value : the list of reasonable address
*/
struct addrinfo* getAddrInfo(char *host , const char *serv , int flags ,
                             int family, int sockType)
{
  struct addrinfo hints;
  struct addrinfo *addrList;

  //initialize hints
  bzero(&hints , sizeof(hints));
  hints.ai_flags = flags;
  hints.ai_family = family;
  hints.ai_socktype = sockType;

  if(getaddrinfo(host , serv , &hints , &addrList) != 0)
    error("error in getaddrinfo");
  return addrList;
}

//==================================================
/**
*scanCongfigFile to return the key's value in char*
*key : the key to find in config file
*return value : the value of key
*data format : key value
*/
char *scanConfigFile(const char* fileName , const char* key_)
{
  int matchFlag;  // 1 = match success
  int n;
  FILE *fp;
  char format[MAX_FORM_LEN];  
  char line[MAX_CONF_LEN];
  char key[MAX_KEY_LEN];
  char *value;

  value = (char *)malloc(MAX_CONF_LEN);

  if((fp = fopen(fileName , "r")) == NULL)
    error("error in scanConfidFile::fopen");
  //using [key] [value] format to read config file
  sprintf(format , "%%%ds %%%ds" , MAX_KEY_LEN-1 , MAX_CONF_LEN-1);
  matchFlag = 0;

  while(fgets(line , MAX_CONF_LEN , fp) != NULL)
  {
    n = sscanf(line , format , key , value);
    if(n == 2 && strcmp(key_ , key) == 0)
    {
      matchFlag = 1;
      break;
    }
  }

  fclose(fp);

  if(matchFlag == 1)
  {
    return value;
  }
  return NULL;
}

//=============================================
/**
*make connect to the pointed hostname
*2. make socket
*3. make conenct 
*return value : the sockFd
*/
int makeTcpConnect(char *hostname ,const char *portName )
{
  struct addrinfo *printdAddrList;
  struct addrinfo *printdAddr;
  int sockFd;

  //get IP address of printd
  printdAddrList = getAddrInfo(hostname , portName , 
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

//===========================================
/**
*makeListen() to build the tcp socket and allow to listen
*complete socket , bind and  listen
*return value : 0 = success
*/
int makeTcpListen(const char *portName , int listenQueue)
{
  int sockFd;
  const int on = 1;
  struct addrinfo* addressList;
  struct addrinfo* address;

  addressList = getAddrInfo(NULL , portName , AI_PASSIVE ,
                            AF_UNSPEC , SOCK_STREAM);
  if(addressList == NULL)  error("error in getAddrInfo");

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
    else if(listen(sockFd , listenQueue) != 0)
    {
      close(sockFd);
      continue;
    }
    return sockFd;
  }
  return -1;
}

//====================================================
/**
*readn to read length of size char from fd
*fd : the file descriptor
*line : buffer to hold the input
*size ; number of char to read
*return value : the number of char has been read
*/
int readn(int fd , void *line , int size)
{
  int left; //how mant char left to read
  int length;
  char *ptr;
  ptr = (char *)line;

  left = size;
  while(left > 0)
  {
    length = read(fd , ptr , left);
    if(length > 0) 
    {
      ptr += length;
      left -= length;
    }
    else if(length == 0) 
      break;
    else
      return -1;
  }
  return size-left;
}

//===================================================
/**
*readTime exit when no input after thmeOut second
*fd : the file descriptor
*line : buffer to hold the input
*size : line size
*timeOut : waiting time
*return value : the number of char has been read
*/
int readTime(int fd , void *line , int size , unsigned int timeOut)
{
  int nfds;
  fd_set readFds;
  struct timeval theTime;

  theTime.tv_sec = timeOut;
  theTime.tv_usec = 0;

  FD_ZERO(&readFds);
  FD_SET(fd , &readFds);

  nfds = select(fd+1 , &readFds , NULL , NULL , &theTime);
  if(nfds == 0)
  {
    errno = ETIME;
    return -1;
  }
  else if(nfds < 0)
    return -1;

  return (read(fd , line ,size));
}

//===================================================
/**
*readnTime read n chars from fd , no input after timeOut second
*fd : the file descriptor
*line : buffer to hold the input
*size ; number of char to read
*timeOut : waiting time
*return value : the number of char has been read
*/
int readnTime(int fd , void *line , int size , unsigned int timeOut)
{
  int left; //how many char left to read
  int length;
  char *ptr;

  ptr = (char *)line;

  left = size;
  while(left > 0)
  {
    //quit after timeOut second no input
    length = readTime(fd , ptr , left , timeOut); 
    if(length > 0) 
    {
      ptr += length;
      left -= length;
    }
    else if(length == 0) 
      break;
    else
      return -1;
  }
  return size-left;
}

//===================================================
/**
*writen to write n char to fd
*fd : the file descriptor to write
*buffer : the buffer to hold the char to write
*size : the number of char need to write
*return value : the number of char has been writen
*/
int writen(int fd , void *buffer , int size)
{
  int left; //how mant char left to write
  int length;
  char *ptr;
  ptr = (char *)buffer;

  left = size;
  while(left > 0)
  {
    length = write(fd , ptr , left);
    if(length > 0) 
    {
      ptr += length;
      left -= length;
    }
    else if(length == 0) 
      break;
    else
      return -1;
  }
  return size-left;
}

//====================================================
/**
*intializeDaemon to intialize the process to be daemon
*1. fork and kill parent
*2. setsid and set signal 
*3. fork and kill parent
*4. close all fd
*5. redirect stdin stdout stderr to /dev/null
*6. openlog 
*process : the process name
*facility : var for openlig
return value : 0 = initialize success
*/
int initializeDaemon(char *process , int facility)
{
  int pid;
  int n;
  extern int ERROR_FLAG;

  if((pid = fork()) < 0)
    error("error in initializeDaemon::first fork");
  else if(pid > 0)
    exit(0);

  if(setsid() < 0)
    error("error in initializeDaemon::setsid");
  signal(SIGHUP , SIG_IGN);

  if((pid = fork()) < 0)
    error("error in initializeDaemon::second fork");
  else if(pid > 0)
    exit(0);

  chdir(ROOT_DIRECTORY); 

  for(n = 0 ; n < sysconf(_SC_OPEN_MAX) ; ++n)
    close(n);

  open("/dev/null" , O_RDONLY);
  open("/dev/null" , O_WRONLY);
  open("/dev/null" , O_RDWR);

  openlog(process , LOG_PID , facility);
  ERROR_FLAG = LOG_ON; //set the error to be syslog

  return 0;
}

//=====================================================
/**
*Add an option to the IPP header
*/
char * addOption(char *cp , int tag , const char *optname , 
                 const char *optval)
{
  int n;
  union
  {
    int16_t s;
    char c[2];
  } u;
  //add the tag
  *cp++ = tag;

  //add the option name
  n = strlen(optname);
  u.s = htons(n);
  *cp++ = u.c[0];
  *cp++ = u.c[1];
  strcpy(cp , optname);
  cp += n;

  //add the option value
  n= strlen(optval);
  u.s = htons(n);
  *cp++ = u.c[0];
  *cp++ = u.c[1];
  strcpy(cp , optval);
  cp += n;

  return cp;
}

//=====================================================
/**
*set IPP header as binary format
*ibuf : the ipp buffer to hold the struct
*requestNumber : the request number of IPP
*printerName : the printer's name
*port : the printer's service port
*/
int setIPPHeader(char ibuf[] , int requestNumber , char *printerName ,
                   int port )
{
  char *icp;
  struct ipp_hdr *hp;
  char uri[SIMPLE_SIZE];

  icp = ibuf;
  hp = (struct ipp_hdr*)icp;

  hp->major_version = 1;
  hp->minor_version = 1;
  hp->operation = htons(OP_PRINT_JOB);
  hp->request_id = htonl(requestNumber);
  //build the attribute
  icp += offsetof(struct ipp_hdr , attr_group);
  *icp++ = TAG_OPERATION_ATTR;
  //set char set
  icp = addOption(icp , TAG_CHARSET , "attributes-charset" , "utf-8");
  //set language
  icp=addOption(icp,TAG_NATULANG,"attributes-natural-language","en-us");
  //set URI
  sprintf(uri , "http://%s:%d" , printerName , port);
  icp = addOption(icp , TAG_URI , "printer_uri" , uri);
  //the end of attribute
  *icp++ = TAG_END_OF_ATTR;

  //so it is easy to separate from main text
  *icp++ = '\r'; 
  *icp++ = '\n';

  *icp++ = '\r'; 
  *icp++ = '\n';
  
  return (icp - ibuf);
}

//=====================================================
/**
*set up HTTP header
*length : the length of file + IPP_header
*printerName : the name of printer
*port : the printer's service port
*/
int setupHTTPHeader(char hbuf[] , long length , char *printerName , 
                    int port)
{
  char *hcp;
  hcp = hbuf;

  sprintf(hcp , "POST /%s/ipp HTTP/1.1\r\n" , printerName);
  hcp += strlen(hcp);

  sprintf(hcp , "Content-Length: %ld\r\n" , length);
  hcp += strlen(hcp);

  strcpy(hcp , "Content-Type: application/ipp\r\n");
  hcp += strlen(hcp);

  sprintf(hcp , "Host: %s:%d\r\n" , printerName , port);
  hcp += strlen(hcp);

  *hcp++ = '\r';
  *hcp++ = '\n';

  return (hcp - hbuf);
}

//=====================================================
/**
*set up IPP header as string format
*ibuf : the ipp buffer to hold the struct
*requestNumber : the request number of IPP
*printerName : the printer's name
*port : the printer's service port
*/
int setupIPPHeader(char ibuf[] , int requestNumber , char *printerName ,
                   int port )
{
  char *icp;
  char uri[SIMPLE_SIZE];

  icp = ibuf;

  sprintf(icp , "major_version: 1\r\n");
  icp += strlen(icp);

  sprintf(icp , "minor_version: 1\r\n");
  icp += strlen(icp);

  sprintf(icp , "operation: %d\r\n" , OP_PRINT_JOB);
  icp += strlen(icp);

  sprintf(icp , "request_id: %d\r\n" , requestNumber);
  icp += strlen(icp);

  sprintf(icp , "attributes-begin\r\n");
  icp += strlen(icp);

  sprintf(icp , "attributes-charset: utf-8\r\n");
  icp += strlen(icp);

  sprintf(icp , "attributes-natural-language: en-us\r\n");
  icp += strlen(icp);

  sprintf(uri , "http://%s:%d" , printerName , port);
  sprintf(icp , "attributes-uri: %s\r\n" , uri);
  icp += strlen(icp);

  sprintf(icp , "attributes-end\r\n");
  icp += strlen(icp);

  *icp++ = '\r'; 
  *icp++ = '\n';
  
  return (icp - ibuf);
}

#endif
