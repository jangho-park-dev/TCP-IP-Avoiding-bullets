//#define _WINSOCK_DEPRECATED_NO_WARNINGS // 최신 VC++ 컴파일 시 경고 방지
#pragma warning(disable:4996)
#pragma comment(lib, "ws2_32")
#include <iostream>
#include <winsock2.h>
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <windows.h>
#include <random>
#include "Globals.h"
#include "Server.h"
using namespace std;

extern CRITICAL_SECTION cs;
extern Packet g_packet[MAXPLAYER];
extern Obstacle_Info g_obstacleManager[MAXOBSTACLE];
extern int g_saveReady[MAXPLAYER];
extern bool resume_flag;
extern HANDLE hThread[MAXPLAYER];

DWORD WINAPI ProcessClient(LPVOID arg)
{
	SOCKET client_sock = (SOCKET)arg;
	int retval;
	int current_id = getId(client_sock);

	bool collision = false; //각각의 클라이언트가 충돌했는지 여부를 위한 지역변수

	while (true)
	{
		// 현 스레드에 연결된 클라의 게임 상태가 Win이면,
		// 나머지 클라 스레드를 재실행한다.
		// 이는 게임 종료 후 재시작 시 필요한 함수다.
		Resume_Check(current_id);


		// 현재 클라에게 준비를 받는다.
		recv_Ready(g_packet[current_id].client_sock, current_id);
		
		// 만약 하나라도 Progressing 상태라면 끝난게 아니므로 대기한다.
		while (true)
		{
			if (AllReady())
			{
				break;
			}// 모두 준비 되었는가?
		}

		// resume_flag가 true면(게임이 재시작 되었으면)
		// 모든 클라의 상태를 Progressing으로 바꾸고,
		// 모든 클라에게 해당 클라의 게임 상태를 보내고 flag를 false로 한다.
		// flag는 공유 변수이고 3개의 스레드 중
		// 한 개의 스레드에서만 실행되어야 하므로 임계 영역을 설정한다.
		EnterCriticalSection(&cs); // 한명의 클라이언트만 실행하도록 lock()
		if (resume_flag) // 1,2,3 클라중에 한명만 실행하면 됨
		{
			Switching_Once();
		}
		LeaveCriticalSection(&cs);//unlock()
		cout << current_id << " 번째 게임 상태 : " << g_packet[current_id].gameState << endl;
		
		// 현 클라의 게임 상태가 Progressing이면 계속 돈다.

		//현재 클라이언트의 상태가 progressing 일때만 루프돌도록.
		//실제 인게임 내의 환경임
		while(g_packet[current_id].gameState == Progressing)
		{
			// 플레이어의 위치 정보를 받아온다.
			recv_PlayerPosInfo(current_id);

			// 게임 재개 시, 플레이어의 포지션이 숨겨져 있었다면?
			
			IF_Pos_is_Hide(current_id);

			// 서버의 장애물 정보를 업데이트한다.
			//공유자원이므로 임계영역으로 처리함.
			EnterCriticalSection(&cs);
			obstacleUpdate();
			LeaveCriticalSection(&cs);

			// 충돌 체크와 Hp 업데이트를 진행한다.
			CollisionCheck_And_HpUpdate(current_id, collision);

			// Hp 업데이트된 부분이 있을지도 모르니 게임 상태를 업데이트한다.
			gameStateUpdate(current_id);

			// 이긴 사람이 있는지 확인하여 Win상태로 만들 수 있으면 그렇게 한다.
			checkWin();

			// 모든 정보를 보내기 전에, 패킷에 장애물 정보를 Set해준다.
			SetBeforeSendObstacleInfo();

			// 모든 정보를 보낸다.
			send_Allinfo(current_id, collision);
		}

		// 현 클라가 Win이 아니면 기다려야 하니 스레드를 일시 정지시킨다.
		WaitForRemainder(current_id);

		// Win인 클라는 WaitForRemainder함수를 거치지 않고
		// 모든 정보를 reset한 후 루프 위로 올라간다.
		resetInfo(current_id);
	}
	return 0;
}
int main(int argc, char* argv[])
{
	//임계영역 초기화
	InitializeCriticalSection(&cs);
	// 초기 장애물의 정보를 초기화해줌.
	InitObstacle();
	
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKADDR_IN clientaddr;
	int addrlen;
	int num = 0;


	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		g_packet[num].client_sock = accept(listen_sock, (SOCKADDR*)& clientaddr, &addrlen);
		if (g_packet[num].client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		cout << "클라이언트 id " << num << " accept" << endl;

		// 접속한 클라 순서대로 id를 매겨준다.
		g_packet[num].id = num;

		// 순서대로 매겨준 id를 클라에 넘겨 자신의 id를 알게 한다.
		send(g_packet[num].client_sock, (char*)& num, sizeof(num), 0);

		// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		hThread[num] = CreateThread(NULL, 0, ProcessClient, (LPVOID)g_packet[num].client_sock, 0, 0);
		num++;
	}

	for (int i = 0; i < MAXPLAYER; ++i)
		CloseHandle(hThread[i]);
	// closesocket()
	closesocket(listen_sock);
	DeleteCriticalSection(&cs);
	// 윈속 종료
	WSACleanup();
	return 0;
}
