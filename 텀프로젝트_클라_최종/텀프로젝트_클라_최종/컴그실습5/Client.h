#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <GL/freeglut.h>
#define SERVERPORT 9000
void err_display(const char* msg);
void err_quit(const char* msg);
int recvn(SOCKET s, char* buf, int len, int flags);
void send_Ready();
void recv_initInfo();
void send_PosInfo();
void recv_Allinfo();
void Textprint(int x, int y, int z, std::string string);
void update();
bool compare(COLOR, COLOR);