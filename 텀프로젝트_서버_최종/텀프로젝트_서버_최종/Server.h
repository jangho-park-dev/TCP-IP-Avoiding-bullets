#pragma once
#pragma comment(lib, "ws2_32")
#include <string>
#include <WinSock2.h>
#include <random>
#include "Globals.h"
using namespace std;

void err_quit(const char* msg);
void err_display(const char* msg);

int recvn(SOCKET s, char* buf, int len, int flags);
bool Collision_Check(float x1, float y1, float x2, float y2, int size1, int size2);
BOOL AllReady();
void InitObstacle();
void recv_Ready(SOCKET,int);
Obstacle_Info hpUpdate(Obstacle_Info&, int);
void gameStateUpdate(int);
void obstacleUpdate();
void recv_PlayerPosInfo(int);
void send_Allinfo(int,bool);
void resetInfo(int id);
BOOL checkWin();
int getId(SOCKET& client_sock);
void Resume_Check(int id);
void Switching_Once();
void IF_Pos_is_Hide(int id);
void CollisionCheck_And_HpUpdate(int id, bool& collision);
void SetBeforeSendObstacleInfo();
void WaitForRemainder(int id);
