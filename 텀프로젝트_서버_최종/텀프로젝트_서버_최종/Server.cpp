#include "Server.h"
#include "Globals.h"
#include <iostream>
using namespace std;

// 장애물의 x, y좌표와 speed를 랜덤하게 설정하기 위한 선언이다.
uniform_int_distribution<> uid(-300, 300);
uniform_int_distribution<> uidY(800, 1600);
uniform_int_distribution<> uidSpeed(1, 3);
default_random_engine dre;

CRITICAL_SECTION cs;
// 서버와 클라 간의 패킷 배열이다.
Packet g_packet[MAXPLAYER];

// 서버에서 관리하기 위한 장애물 매니저 배열이다.
Obstacle_Info g_obstacleManager[MAXOBSTACLE];

// 레디 정보를 임시 저장하기 위한 배열이다.
int g_saveReady[MAXPLAYER] = { 0, };

// 게임 재개 시 필요한 bool 변수다.
bool resume_flag = true;

// 스레드를 중지, 재개하기 위한 스레드 핸들 배열이다.
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

// 소켓 함수 오류 출력 
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
	cout << id << " 번째 클라이언트 recv_ready" << endl;
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

	//모두 레디이면 cnt가 3일것이다.
	for (int i = 0; i < MAXPLAYER; ++i)
		if (g_saveReady[i] == Ready)
			cnt++;

	
	if (cnt >= MAXPLAYER)
		return TRUE; // 모두레디를 했다는 것이므로 TRUE를 반환
	else
		return FALSE; //모두레디를 하지않았을경우 FALSE를 반환
}

void InitObstacle()
{
	// 클라이언트에게 보낼 패킷에 장애물 정보를 초기화 해준다.
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
	// id번째 플레이어의 hp를 i번째 장애물의 damage만큼 깎아준다.
	g_packet[id].p_info.hp -= o_info.damage;
	
	// 한 번 hp를 깎은 장애물은 초기화될 때까지 damage를 0으로 한다.
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
	// 각 장애물들을 업데이트 처리해준다.
	for (int i = 0; i < MAXOBSTACLE; ++i)
	{
		g_obstacleManager[i].position.y -= g_obstacleManager[i].speed;

		//장애물의 스폰을 위함임
		if (g_obstacleManager[i].position.y < -100)
		{
			g_obstacleManager[i].position.y = (float)uidY(dre);
			g_obstacleManager[i].damage = 10;
		}
	}
}

void recv_PlayerPosInfo(int id)
{
	// 각 플레이어의 포지션을 받아 온다.
	int retval;
	retval = recvn(g_packet[id].client_sock, (char*)&g_packet[id].p_info.position, sizeof(POSITION), 0);
}

void send_Allinfo(int id, bool collision)
{
	// 모든 정보들을 클라이언트에게 전송한다.
	int retval;

	EnterCriticalSection(&cs);
	// 장애물 정보 보내기
	for (int j = 0; j < MAXOBSTACLE; ++j)
	{
		retval = send(g_packet[id].client_sock, (char*)&g_packet[id].o_info[j], sizeof(Obstacle_Info), 0);
	}
	//LeaveCriticalSection(&cs);
	// 플레이어 정보 보내기
	for (int i = 0; i < MAXPLAYER; ++i)
	{
		// HIDE는 플레이어가 죽었을 때 숨겨 보내기 위한 설정이다.
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

	// 게임의 상태 보내기
	retval = send(g_packet[id].client_sock, (char*)&g_packet[id].gameState, sizeof(Gamestate_info), 0);
	
}

void resetInfo(int id)
{
	// Win 상태인 플레이어가 대표로 모든 정보를 초기화한다.
	if (g_packet[id].gameState == Win)
	{
		for (int i = 0; i < MAXPLAYER; ++i)
		{
			g_saveReady[i] = 0;
			g_packet[i].p_info.Init();
		}
		InitObstacle();
		cout << "모든 정보를 리셋했습니다." << endl;
	}
}

void Resume_Check(int id)
{
	// 현재 쓰레드의 id의 상태가 win일때만 실행하도록
	if (g_packet[id].gameState == Win)
	{// 게임의 상태가 win인 쓰레드가 나머지 상태의 쓰레드들을 깨운다.
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
	{//모든 클라의 게임상태를 Progressing으로 바구어주고 이걸 각 클라에게 보내준다.
		g_packet[i].gameState = Progressing;
		send(g_packet[i].client_sock, (char*)& g_packet[i].gameState, sizeof(Gamestate_info), 0);
	}
	resume_flag = false; // 1,2,3쓰레드중 아무쓰레드나 1번만 실행하면되기떄문에 false로해준다.
}

void IF_Pos_is_Hide(int id)
{//죽은플레이어는 좌표가 (HIDE,HIDE)로 옮겨지기때문에 
	//게임의 상태가 진행중임에도 불구하고 HIDE,HIDE이면 초기화를 해주어함.
	//(게임이 재시작 할경우를 위함.)
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

	// 업데이트된 g_obstacleManager를 하나씩 가져와 o_info에 넣고,
	// 위의 p_info와 함께 Collision_Check함수의 인자에 넣어 충돌 체크한다.
	collision = false;
	for (int i = 0; i < MAXOBSTACLE; ++i)
	{
		o_info = g_obstacleManager[i];

		if (Collision_Check(p_info.position.x, p_info.position.y,
			o_info.position.x, o_info.position.y,
			p_info.size, o_info.size))
		{
			// 충돌 체크되었다면 hp를 업데이트한다.
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
		cout << id << " 번째 클라가 중지되어 다른 클라를 기다립니다." << endl;
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