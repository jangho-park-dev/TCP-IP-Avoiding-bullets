//#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
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

	bool collision = false; //������ Ŭ���̾�Ʈ�� �浹�ߴ��� ���θ� ���� ��������

	while (true)
	{
		// �� �����忡 ����� Ŭ���� ���� ���°� Win�̸�,
		// ������ Ŭ�� �����带 ������Ѵ�.
		// �̴� ���� ���� �� ����� �� �ʿ��� �Լ���.
		Resume_Check(current_id);


		// ���� Ŭ�󿡰� �غ� �޴´�.
		recv_Ready(g_packet[current_id].client_sock, current_id);
		
		// ���� �ϳ��� Progressing ���¶�� ������ �ƴϹǷ� ����Ѵ�.
		while (true)
		{
			if (AllReady())
			{
				break;
			}// ��� �غ� �Ǿ��°�?
		}

		// resume_flag�� true��(������ ����� �Ǿ�����)
		// ��� Ŭ���� ���¸� Progressing���� �ٲٰ�,
		// ��� Ŭ�󿡰� �ش� Ŭ���� ���� ���¸� ������ flag�� false�� �Ѵ�.
		// flag�� ���� �����̰� 3���� ������ ��
		// �� ���� �����忡���� ����Ǿ�� �ϹǷ� �Ӱ� ������ �����Ѵ�.
		EnterCriticalSection(&cs); // �Ѹ��� Ŭ���̾�Ʈ�� �����ϵ��� lock()
		if (resume_flag) // 1,2,3 Ŭ���߿� �Ѹ� �����ϸ� ��
		{
			Switching_Once();
		}
		LeaveCriticalSection(&cs);//unlock()
		cout << current_id << " ��° ���� ���� : " << g_packet[current_id].gameState << endl;
		
		// �� Ŭ���� ���� ���°� Progressing�̸� ��� ����.

		//���� Ŭ���̾�Ʈ�� ���°� progressing �϶��� ����������.
		//���� �ΰ��� ���� ȯ����
		while(g_packet[current_id].gameState == Progressing)
		{
			// �÷��̾��� ��ġ ������ �޾ƿ´�.
			recv_PlayerPosInfo(current_id);

			// ���� �簳 ��, �÷��̾��� �������� ������ �־��ٸ�?
			
			IF_Pos_is_Hide(current_id);

			// ������ ��ֹ� ������ ������Ʈ�Ѵ�.
			//�����ڿ��̹Ƿ� �Ӱ迵������ ó����.
			EnterCriticalSection(&cs);
			obstacleUpdate();
			LeaveCriticalSection(&cs);

			// �浹 üũ�� Hp ������Ʈ�� �����Ѵ�.
			CollisionCheck_And_HpUpdate(current_id, collision);

			// Hp ������Ʈ�� �κ��� �������� �𸣴� ���� ���¸� ������Ʈ�Ѵ�.
			gameStateUpdate(current_id);

			// �̱� ����� �ִ��� Ȯ���Ͽ� Win���·� ���� �� ������ �׷��� �Ѵ�.
			checkWin();

			// ��� ������ ������ ����, ��Ŷ�� ��ֹ� ������ Set���ش�.
			SetBeforeSendObstacleInfo();

			// ��� ������ ������.
			send_Allinfo(current_id, collision);
		}

		// �� Ŭ�� Win�� �ƴϸ� ��ٷ��� �ϴ� �����带 �Ͻ� ������Ų��.
		WaitForRemainder(current_id);

		// Win�� Ŭ��� WaitForRemainder�Լ��� ��ġ�� �ʰ�
		// ��� ������ reset�� �� ���� ���� �ö󰣴�.
		resetInfo(current_id);
	}
	return 0;
}
int main(int argc, char* argv[])
{
	//�Ӱ迵�� �ʱ�ȭ
	InitializeCriticalSection(&cs);
	// �ʱ� ��ֹ��� ������ �ʱ�ȭ����.
	InitObstacle();
	
	int retval;

	// ���� �ʱ�ȭ
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

	// ������ ��ſ� ����� ����
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
		cout << "Ŭ���̾�Ʈ id " << num << " accept" << endl;

		// ������ Ŭ�� ������� id�� �Ű��ش�.
		g_packet[num].id = num;

		// ������� �Ű��� id�� Ŭ�� �Ѱ� �ڽ��� id�� �˰� �Ѵ�.
		send(g_packet[num].client_sock, (char*)& num, sizeof(num), 0);

		// ������ Ŭ���̾�Ʈ ���� ���
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		hThread[num] = CreateThread(NULL, 0, ProcessClient, (LPVOID)g_packet[num].client_sock, 0, 0);
		num++;
	}

	for (int i = 0; i < MAXPLAYER; ++i)
		CloseHandle(hThread[i]);
	// closesocket()
	closesocket(listen_sock);
	DeleteCriticalSection(&cs);
	// ���� ����
	WSACleanup();
	return 0;
}
