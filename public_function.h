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

using namespace std;

//===============================================
/**
*all kinds of define
*/
#define SIMPLE_SIZE 150 //simple buffer size to read simple message 

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
//    fputs(line , stdout);
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

#endif
