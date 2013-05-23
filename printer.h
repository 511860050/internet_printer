//============================================
//FileName: printer.h
//Date: 2013年 05月 21日 星期二 15:42:48 CST
//Author: chenhuan
//Usage: the interface of Printer
//============================================

#ifndef CHEN_HUAN_PRINTER_HEADER 
#define CHEN_HUAN_PRINTER_HEADER 

#include "public_function.h"

#define DIRECTORY "/home/chenhuan/Hello_World/internet_print/printer"

//============================================
/**
*the class of Printer
*Accept the file from printd
*/
class Printer
{
public:
  void run();
private:
  int receiveHTTPHeader(FILE *clientFp , ofstream &ofs);
  int receiveIPPHeader(FILE *clientFp , ofstream &ofs);
  int receiveFile(FILE *clientFp , ofstream &ofs);
  int sendReply(FILE *clientFp);
};  //end if Printer

#endif
