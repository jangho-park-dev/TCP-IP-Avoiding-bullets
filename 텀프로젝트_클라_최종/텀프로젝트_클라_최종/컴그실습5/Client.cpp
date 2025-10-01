#pragma once
#include "Globals.h"
#include "Client.h"
#include "Player.h"
#include "Enemy.h"

using namespace std;

// ������ Ŭ�� ���� ��Ŷ ������.
Packet g_packet;

// Ŭ�󿡼� ��� �÷��̾��� ������ ������ �迭�̴�.
Player *player[MAXPLAYER];

// �������� Obstacle ������ �޾ƿ� ������ �迭�̴�.
Enemy *enemies[MAXOBSTACLE];

// ��Ŷ�� ���� �����̱⿡ ��� Ŭ���� ������ �޾ƿ� �迭�� �����Ѵ�.
Player_Info players_info[MAXPLAYER];

// �������� ������� �ѱ�� id����� ������ Ŭ���� id ������ ���� ������.
int p_num;

void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)& lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)& lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCWSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

int recvn(SOCKET s, char* buf, int len, int flags)
{
	int received;
	char* ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

void send_Ready()
{
	send(g_packet.client_sock, (char*)& g_packet.gameState, sizeof(Gamestate_info), 0);
}

void recv_initInfo()
{
	recvn(g_packet.client_sock, (char*)& g_packet.gameState, sizeof(Gamestate_info), 0);
}

void send_PosInfo()
{
	send(g_packet.client_sock, (char*)& player[p_num]->pos, sizeof(POSITION), 0);
}

void recv_Allinfo()
{
	int retval;

	for (int i = 0; i < MAXOBSTACLE; ++i)
		retval = recvn(g_packet.client_sock, (char*)&g_packet.o_info[i], sizeof(Obstacle_Info), 0);

	for (int i = 0; i < MAXPLAYER; ++i)
		retval = recvn(g_packet.client_sock, (char*)&players_info[i], sizeof(Player_Info), 0);

	
	//g_packet.p_info = players_info[p_num];


	retval = recvn(g_packet.client_sock, (char*)&g_packet.gameState, sizeof(Gamestate_info), 0);
}

void Textprint(int x, int y, int z, string string)
{
	
	glRasterPos2f((GLfloat)x , (GLfloat)y);
	int len = string.length();
	for (int i = 0; i < len; i++)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
}

void update()
{
	for (int i = 0; i < MAXOBSTACLE; ++i)
	{// ��ֹ� ������ �޾ƿ� �� �迭�� set����~
		enemies[i]->color = g_packet.o_info[i].color;
		enemies[i]->damage = g_packet.o_info[i].damage;
		enemies[i]->pos = g_packet.o_info[i].position;
		enemies[i]->size = g_packet.o_info[i].size;
		enemies[i]->speed = g_packet.o_info[i].speed;
	}
	
	//�÷��̾� ������ �޾ƿ� ��Ŷ�� �ɰ��� set����~
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		player[i]->color = players_info[i].color;
		player[i]->hp = players_info[i].hp;
		player[i]->pos = players_info[i].position;
		player[i]->size = players_info[i].size;
		player[i]->speed = players_info[i].speed;
	}
}

bool compareColor(COLOR a, COLOR b)
{
	if (a.r == b.r && a.g == b.g && a.b == b.b)
		return true;
	return false;
}