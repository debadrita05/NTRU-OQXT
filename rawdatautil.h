#ifndef RAWDATAUTIL_H
#define RAWDATAUTIL_H

#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <bits/stdc++.h>
#include <iostream>

#include "size_parameters.h"

using namespace  std;

int DB_StrToHex2(unsigned char *hexarr,unsigned char *text);
int DB_StrToHex(unsigned char *hexarr,unsigned char *text);
int DB_StrToHex8(unsigned char *hexarr,unsigned char *text);
int DB_StrToHex8(unsigned char *hexarr,const char *text);
int DB_StrToHex8(unsigned int *hexarr,const char *text);
int DB_StrToHex12(unsigned char *hexarr,unsigned char *text);
int DB_StrToHex16(unsigned char *hexarr,unsigned char *text);
int DB_StrToHex32(unsigned char *hexarr,unsigned char *text);
int DB_StrToHex48(unsigned char *hexarr,const char *text);
int DB_StrToHex49(unsigned char *hexarr,unsigned char *text);
int DB_StrToHexN(unsigned char *hexarr,const char *text, int N);
// std::string HexToStr(int *hexarr, int len);
// std::string HexToStr(unsigned char *hexarr, int len);
std::string DB_HexToStr(unsigned char *hexarr);
std::string DB_HexToStr_N(unsigned char *hexarr, unsigned int n);
int DB_StrToHex_N(unsigned char *hexarr,const char *text,int n);
std::string DB_HexToStr32(unsigned char *hexarr);
std::string DB_HexToStr2(unsigned char *hexarr);
std::string DB_HexToStr8(unsigned char *hexarr);
std::string DB_HexToStr12(unsigned char *hexarr);
int DB_Callback_W_IDs(void *ReturnArr, int argc, char **argv, char **azColName);
int DB_Callback_E_IDs(void *ReturnArr, int argc, char **argv, char **azColName);
int DB_Callback_T_IDs(void *ReturnArr, int argc, char **argv, char **azColName);

std::string NumToHexStr(int num);
int StrToHex(unsigned char *hexarr,string numin);
int StrToHexBVec(unsigned char *hexarr,string bvec);
std::string uint8ToString(const uint8_t* data, size_t length);

#endif // RAWDATAUTIL_H
