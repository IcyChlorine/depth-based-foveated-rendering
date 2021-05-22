#pragma once

#define __COMPILE_HTTP_PART

#ifdef __COMPILE_HTTP_PART
#include<Winsock2.h>
#include<string>
#include<iostream>
#include<string.h>
#include<math.h>
#include<time.h>
using namespace std;
#pragma warning(disable:4996)

int send_http(int id, float *http_x, float *http_y);

#endif