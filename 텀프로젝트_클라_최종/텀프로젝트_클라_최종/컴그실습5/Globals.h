#pragma once
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#define MAXOBSTACLE		200
#define MAXPLAYER		3
enum Gamestate_info
{
	Wait			=	200,
	Ready			=	201,
	Progressing		=	202,
	End				=	203,
	Win				=	204
};

typedef struct POSITION
{
	float x;
	float y;
}POSITION;

typedef struct COLOR
{
	COLOR(){} 
	COLOR(float r, float g, float b) : r{r}, g{g}, b{b}{}
	float r;
	float g;
	float b;
}COLOR;

typedef struct Player_Info
{
	POSITION position;
	COLOR color;
	int hp;
	int size;
	int speed;
}Player_Info;

typedef struct Obstacle_Info
{
	POSITION position;
	COLOR color;
	int damage;
	int size;
	int speed;
}Obstacle_Info;

typedef struct Packet
{
	SOCKET client_sock;
	Gamestate_info gameState;
	Player_Info p_info;
	Obstacle_Info o_info[MAXOBSTACLE];
}Packet;


