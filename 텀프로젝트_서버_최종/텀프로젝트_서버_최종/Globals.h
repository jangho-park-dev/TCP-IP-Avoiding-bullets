#pragma once

#define SERVERPORT	9000
#define MAXPLAYER	3
#define MAXOBSTACLE 200
#define HIDE		-10000

enum Gamestate_info
{
	Wait		=	200,
	Ready		=	201,
	Progressing =	202,
	End			=	203,
	Win			=	204,
	AllEnd		=	205
};

typedef struct POSITION
{
	float x;
	float y;
}POSITION;

typedef struct COLOR
{
	COLOR(){}
	COLOR(float r, float g, float b) : r{r}, g{g}, b{b} {}
	float r;
	float g;
	float b;
}COLOR;

typedef struct Player_Info
{
	POSITION position	=	{ 0.f,0.f };
	COLOR color			=	{1.f, 1.f, 0.f};
	int hp				=	100;
	int size			=	16;
	int speed			=	7;

	void Init()
	{
		position		=	{ 0.f, 0.f };
		hp				=	100;
		size			=	16;
		speed			=	7;
		color			=	{ 1.f,1.f,0.f };
	}
}Player_Info;

typedef struct Obstacle_Info
{
	POSITION position;
	COLOR color			=	{ 0.f, 0.f, 1.f };
	int damage;
	int size;
	int speed;
}Obstacle_Info;

typedef struct Packet
{
	int id = 0;
	SOCKET client_sock;
	Gamestate_info gameState;
	Player_Info p_info;
	Obstacle_Info o_info[MAXOBSTACLE];

}Packet;