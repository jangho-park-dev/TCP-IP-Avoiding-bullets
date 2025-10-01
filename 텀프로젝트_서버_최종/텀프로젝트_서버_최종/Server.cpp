#include "Server.h"
#include "Globals.h"
#include <iostream>
using namespace std;

// ��ֹ��� x, y��ǥ�� speed�� �����ϰ� �����ϱ� ���� �����̴�.
uniform_int_distribution<> uid(-300, 300);
uniform_int_distribution<> uidY(800, 1600);
uniform_int_distribution<> uidSpeed(1, 3);
default_random_engine dre;

CRITICAL_SECTION cs;
// ������ Ŭ�� ���� ��Ŷ �迭�̴�.
Packet g_packet[MAXPLAYER];

// �������� �����ϱ� ���� ��ֹ� �Ŵ��� �迭�̴�.
Obstacle_Info g_obstacleManager[MAXOBSTACLE];

// ���� ������ �ӽ� �����ϱ� ���� �迭�̴�.
int g_saveReady[MAXPLAYER] = { 0, };

// ���� �簳 �� �ʿ��� bool ������.
bool resume_flag = true;

// �����带 ����, �簳�ϱ� ���� ������ �ڵ� �迭�̴�.
HANDLE hThread[MAXPLAYER];

void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)& lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// ���� �Լ� ���� ��� 
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

void recv_Ready(SOCKET client_sock, int id)
{
	int retval;
	retval = recvn(client_sock, (char*)&g_saveReady[id], sizeof(int), 0);
	cout << id << " ��° Ŭ���̾�Ʈ recv_ready" << endl;
}

bool Collision_Check(float x1, float y1, float x2, float y2, int size1, int size2)
{
	if (x1 - size1 / 2 > x2 + size2 / 2)
		return false;
	if (x1 + size1 / 2 < x2 - size2 / 2)
		return false;
	if (y1 - size1 / 2 > y2 + size2 / 2)
		return false;
	if (y1 + size1 / 2 < y2 - size2 / 2)
		return false;
	return true;
}


BOOL AllReady()
{
	int cnt = 0;

	//��� �����̸� cnt�� 3�ϰ��̴�.
	for (int i = 0; i < MAXPLAYER; ++i)
		if (g_saveReady[i] == Ready)
			cnt++;

	
	if (cnt >= MAXPLAYER)
		return TRUE; // ��η��� �ߴٴ� ���̹Ƿ� TRUE�� ��ȯ
	else
		return FALSE; //��η��� �����ʾ������ FALSE�� ��ȯ
}

void InitObstacle()
{
	// Ŭ���̾�Ʈ���� ���� ��Ŷ�� ��ֹ� ������ �ʱ�ȭ ���ش�.
	for (int i = 0; i < MAXOBSTACLE; ++i)
	{
		g_obstacleManager[i].position.x = (float)uid(dre);
		g_obstacleManager[i].position.y = (float)uidY(dre);
		g_obstacleManager[i].size = 10;
		g_obstacleManager[i].speed = uidSpeed(dre);
		g_obstacleManager[i].damage = 10;
	}
}

Obstacle_Info hpUpdate(Obstacle_Info& o_info, int id)
{
	// id��° �÷��̾��� hp�� i��° ��ֹ��� damage��ŭ ����ش�.
	g_packet[id].p_info.hp -= o_info.damage;
	
	// �� �� hp�� ���� ��ֹ��� �ʱ�ȭ�� ������ damage�� 0���� �Ѵ�.
	o_info.damage = 0;

	return o_info;
}

void gameStateUpdate(int id)
{
	if (g_packet[id].p_info.hp <= 0)
		g_packet[id].gameState = End; 
	else
		g_packet[id].gameState = Progressing;
}

BOOL checkWin()
{
	int cnt = 0;
	int winner_id = 0;
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		if (g_packet[i].gameState == End)
			cnt++;
		else
			winner_id = i;
	}

	if (cnt == MAXPLAYER - 1)
	{
		g_packet[winner_id].gameState = Win;

		return TRUE;
	}

	return FALSE;
}

void obstacleUpdate()
{
	// �� ��ֹ����� ������Ʈ ó�����ش�.
	for (int i = 0; i < MAXOBSTACLE; ++i)
	{
		g_obstacleManager[i].position.y -= g_obstacleManager[i].speed;

		//��ֹ��� ������ ������
		if (g_obstacleManager[i].position.y < -100)
		{
			g_obstacleManager[i].position.y = (float)uidY(dre);
			g_obstacleManager[i].damage = 10;
		}
	}
}

void recv_PlayerPosInfo(int id)
{
	// �� �÷��̾��� �������� �޾� �´�.
	int retval;
	retval = recvn(g_packet[id].client_sock, (char*)&g_packet[id].p_info.position, sizeof(POSITION), 0);
}

void send_Allinfo(int id, bool collision)
{
	// ��� �������� Ŭ���̾�Ʈ���� �����Ѵ�.
	int retval;

	EnterCriticalSection(&cs);
	// ��ֹ� ���� ������
	for (int j = 0; j < MAXOBSTACLE; ++j)
	{
		retval = send(g_packet[id].client_sock, (char*)&g_packet[id].o_info[j], sizeof(Obstacle_Info), 0);
	}
	//LeaveCriticalSection(&cs);
	// �÷��̾� ���� ������
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		// HIDE�� �÷��̾ �׾��� �� ���� ������ ���� �����̴�.
		if (id == 0)
		{
			if(collision)
				g_packet[0].p_info.color = { 1.f, 1.f, 1.f };
			else
				g_packet[0].p_info.color = { 1.f, 0.f, 0.f };
			if (g_packet[0].gameState == End)
				g_packet[0].p_info.position = { HIDE, HIDE };
			retval = send(g_packet[0].client_sock, (char*)& g_packet[i].p_info, sizeof(Player_Info), 0);
		}
		else if (id == 1)
		{
			if (collision)
				g_packet[1].p_info.color = { 1.f, 1.f, 1.f };
			else
				g_packet[1].p_info.color = { 1.f, 1.f, 0.f };
			if (g_packet[1].gameState == End)
				g_packet[1].p_info.position = { HIDE, HIDE };
			retval = send(g_packet[1].client_sock, (char*)&g_packet[i].p_info, sizeof(Player_Info), 0);
		}
		else if (id == 2)
		{
			if (collision)
				g_packet[2].p_info.color = { 1.f, 1.f, 1.f };
			else
				g_packet[2].p_info.color = { 0.f, 1.f, 0.f };
			if (g_packet[2].gameState == End)
				g_packet[2].p_info.position = { HIDE, HIDE };
			retval = send(g_packet[2].client_sock, (char*)&g_packet[i].p_info, sizeof(Player_Info), 0);
		}
	}
	LeaveCriticalSection(&cs);

	// ������ ���� ������
	retval = send(g_packet[id].client_sock, (char*)&g_packet[id].gameState, sizeof(Gamestate_info), 0);
	
}

void resetInfo(int id)
{
	// Win ������ �÷��̾ ��ǥ�� ��� ������ �ʱ�ȭ�Ѵ�.
	if (g_packet[id].gameState == Win)
	{
		for (int i = 0; i < MAXPLAYER; ++i)
		{
			g_saveReady[i] = 0;
			g_packet[i].p_info.Init();
		}
		InitObstacle();
		cout << "��� ������ �����߽��ϴ�." << endl;
	}
}

void Resume_Check(int id)
{
	// ���� �������� id�� ���°� win�϶��� �����ϵ���
	if (g_packet[id].gameState == Win)
	{// ������ ���°� win�� �����尡 ������ ������ ��������� �����.
		for (int i = 0; i < MAXPLAYER; ++i)
		{
			if (g_packet[i].gameState != Win)
				ResumeThread(hThread[i]);
		}
		resume_flag = true;
	}
}

void Switching_Once()
{
	for (int i = 0; i < MAXPLAYER; ++i)
	{//��� Ŭ���� ���ӻ��¸� Progressing���� �ٱ����ְ� �̰� �� Ŭ�󿡰� �����ش�.
		g_packet[i].gameState = Progressing;
		send(g_packet[i].client_sock, (char*)& g_packet[i].gameState, sizeof(Gamestate_info), 0);
	}
	resume_flag = false; // 1,2,3�������� �ƹ������峪 1���� �����ϸ�Ǳ⋚���� false�����ش�.
}

void IF_Pos_is_Hide(int id)
{//�����÷��̾�� ��ǥ�� (HIDE,HIDE)�� �Ű����⶧���� 
	//������ ���°� �������ӿ��� �ұ��ϰ� HIDE,HIDE�̸� �ʱ�ȭ�� ���־���.
	//(������ ����� �Ұ�츦 ����.)
	if (g_packet[id].p_info.position.x == HIDE)
		g_packet[id].p_info.position.x = 0.f;
	if (g_packet[id].p_info.position.y == HIDE)
		g_packet[id].p_info.position.y = 0.f;
}

void CollisionCheck_And_HpUpdate(int id, bool& collision)
{
	Player_Info p_info;
	p_info = g_packet[id].p_info;
	Obstacle_Info o_info;

	// ������Ʈ�� g_obstacleManager�� �ϳ��� ������ o_info�� �ְ�,
	// ���� p_info�� �Բ� Collision_Check�Լ��� ���ڿ� �־� �浹 üũ�Ѵ�.
	collision = false;
	for (int i = 0; i < MAXOBSTACLE; ++i)
	{
		o_info = g_obstacleManager[i];

		if (Collision_Check(p_info.position.x, p_info.position.y,
			o_info.position.x, o_info.position.y,
			p_info.size, o_info.size))
		{
			// �浹 üũ�Ǿ��ٸ� hp�� ������Ʈ�Ѵ�.
			g_obstacleManager[i] = hpUpdate(o_info, id);
			collision = true;

		}
	}
}

void SetBeforeSendObstacleInfo()
{
	for (int i = 0; i < MAXPLAYER; ++i)
		for (int j = 0; j < MAXOBSTACLE; ++j)
			g_packet[i].o_info[j] = g_obstacleManager[j];
}

void WaitForRemainder(int id)
{
	if (g_packet[id].gameState != Win)
	{
		cout << id << " ��° Ŭ�� �����Ǿ� �ٸ� Ŭ�� ��ٸ��ϴ�." << endl;
		SuspendThread(hThread[id]);
	}
}

int getId(SOCKET& client_sock)
{
	for (int i = 0; i < MAXPLAYER; ++i)
		if (g_packet[i].client_sock == client_sock)
			return g_packet[i].id;

	return 0;
}