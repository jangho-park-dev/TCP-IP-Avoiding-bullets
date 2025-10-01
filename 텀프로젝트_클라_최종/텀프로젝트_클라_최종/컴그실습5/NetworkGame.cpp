#pragma once
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
#include <winsock2.h>
#include <GL/freeglut.h>
#include <iostream>
#include <string>
#include "Player.h"
#include "Enemy.h"
#include "Globals.h"
#include "Client.h"
using namespace std;

GLvoid render(GLvoid);
GLvoid Reshape(int w, int h);
void TimerFunction(int);
void keyboard(unsigned char, int, int);
void Motion(int x, int y);

extern Packet g_packet;

extern Player *player[MAXPLAYER];
extern Enemy *enemies[MAXOBSTACLE];


extern Player_Info players_info[MAXPLAYER];
extern int p_num;

int main(int argc, char * argv[])
{
	g_packet.gameState = Wait; // 클라가시작했으니 게임상태를 wait으로초기화.
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 0;

	// 소켓을 생성해줄거야 
	g_packet.client_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (g_packet.client_sock == INVALID_SOCKET)
		err_quit("socket()");

	while (1)
	{
		string server_IP;
		cout << "서버 ip를 입력해주세요 : ";
		cin >> server_IP;

		SOCKADDR_IN serveraddr;
		ZeroMemory(&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = inet_addr(server_IP.c_str());
		serveraddr.sin_port = htons(SERVERPORT);

		int retval = connect(g_packet.client_sock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR)
			err_display("connect()");
		else
			break;
	}


	int retval = recvn(g_packet.client_sock, (char*)&p_num, sizeof(p_num), 0);
	cout << p_num << endl;

	// 플레이어 메모리 잡아줌
	for (int i = 0; i < MAXPLAYER; ++i)
		player[i] = new Player;

	// obstacle들을 enemy로 간주했음. enemy 메모리잡아줌.
	for (int i = 0; i < MAXOBSTACLE; ++i)
		enemies[i] = new Enemy;

	//초기화 함수들
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(300, 0);
	glutInitWindowSize(600, 800);
	glutCreateWindow("test");
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(Reshape);
	glutTimerFunc(16, TimerFunction, 1);
	glutDisplayFunc(render);
	glutMotionFunc(Motion);
	glutMainLoop();

	return 0;
}

// 마우스
void Motion(int x, int y)
{
	// 오픈지엘은 좌표계가 다르기 때문에 그걸 위해 설정해준 것임.
	if (g_packet.gameState == Progressing)
	{
		if (x - 300 >= -300 && x - 300 <= 300)
			player[p_num]->pos.x = (float)(x - 300);
	}
}
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'r':
	{
		// Wait이면 Ready로 바꿔 준다.
		if (g_packet.gameState == Wait )
		{
			g_packet.gameState = Ready;
			send_Ready();
		}

		if (g_packet.gameState == End || g_packet.gameState == Win)
			g_packet.gameState = Wait;

		break;
	}
	default:
		break;
	}
	glutPostRedisplay();
}

// 이 함수는 render 전에 무조건 불리는 함수다.
void TimerFunction(int value)
{
	// 호출순서는 render, timerfunction, redner, timerfunction 무한반복

	// 플레이어가 r을 눌렀을 때의 상황임.
	if (g_packet.gameState == Ready)
	{
		// 서버로부터 게임의 초기 상태를 받아온다.
		recv_initInfo();

		for (int i = 0; i < MAXPLAYER; ++i)
		{
			if (players_info[i].position.x != 0)
				players_info[i].position.x = 0;
			if (players_info[i].position.y != 0)
				players_info[i].position.y = 0;
		}
	}

	// 게임의 상태가 진행 중일 때임.
	if (g_packet.gameState == Progressing)
	{
		// 플레이어의 좌표를 프레임 시간 별로 서버에 전송한다.
		send_PosInfo();
		
		// 서버가 연산한 정보를 받는다.
		recv_Allinfo();
		
		update();
	}

	glutPostRedisplay();
	glutTimerFunc(16, TimerFunction, 1);
}

GLvoid render(GLvoid)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// player 이동 후 렌더

	glPopMatrix();
	if (g_packet.gameState == Wait)
	{
		glPushMatrix();
		{
			glColor3f(1.0, 1.0, 1.0);
			string str;
			str = "PRESS 'r' Ready Button ";
			Textprint(-130, 500, 0, str);

		}
		glPopMatrix();
	}
	else if (g_packet.gameState == Progressing)
	{
		
		for (int i = 0; i < MAXPLAYER; ++i)
		{
			glPushMatrix();
			{
				glTranslatef(players_info[i].position.x, players_info[i].position.y, 0);
				glColor3f(players_info[i].color.r, players_info[i].color.g, players_info[i].color.b);
				glutSolidCube(players_info[i].size);

				if (i == p_num)
				{
					glPushMatrix();
					{
						string str = "me";
						Textprint(-14, 11, 0, str);
					}
					glPopMatrix();

				}
			}
			glPopMatrix();
		}
		// 적 이동 후 렌더
		for (int i = 0; i < MAXOBSTACLE; ++i)
		{
			glPushMatrix();
			{
	
				glColor3f(enemies[i]->color.r, enemies[i]->color.g, enemies[i]->color.b);
				glTranslatef(enemies[i]->pos.x, enemies[i]->pos.y, 0);
				glutSolidSphere(enemies[i]->size, 10, 10);
			}
			glPopMatrix();
		}
		//if (g_packet.p_info.color.r == 1.f && g_packet.p_info.color.g == 1.f && g_packet.p_info.color.b == 1.f) // 이거 한방에 바꾸고싶은데 어케하누 ??
		if (players_info[p_num].color.r == 1.f &&
			players_info[p_num].color.g == 1.f &&
			players_info[p_num].color.b == 1.f)
		{
			glPushMatrix();
			{
				glColor3f(1.0, 1.0, 1.0);
				string str;
				str = "HIT !!!";
				Textprint(0, 300, 0, str);

			}
			glPopMatrix();

			glPushMatrix();
			{
				glColor3f(1.0f, 0, 0);
				string str;
				str = "Hp: " + to_string(player[p_num]->hp);
				Textprint(-270, -40, 0, str);
			}
			glPopMatrix();
		}
		else
		{
			// player hp 실시간 출력
			glPushMatrix();
			{
				glColor3f(1.0f, 1.0f, 1.0f);
				string str;
				str = "Hp: " + to_string(player[p_num]->hp);
				Textprint(-270, -40, 0, str);
			}
			glPopMatrix();
		}
	}

	else if (g_packet.gameState == End)
	{
		glPushMatrix();
		{
			glColor3f(1.0, 0.0, 0.0);
			string str;
			str = "Game Over, PRESS 'r' Restart !";
			Textprint(-130, 400, 0, str);

		}
		glPopMatrix();
	}
	else if (g_packet.gameState == Win)
	{
		glPushMatrix();
		{
			glColor3f(1.0, 1.0, 0.0);
			string str;
			str = "Win, PRESS 'r' Restart !";
			Textprint(-130, 400, 0, str);

		}
		glPopMatrix();
	}

	glutSwapBuffers();
}
GLvoid Reshape(int w, int h)
{
	glMatrixMode(GL_PROJECTION);
	//glOrtho(-400.0, 400.0, -100, 800.0, -1000, 1000);
	glOrtho(-300.0, 300.0, -100, 800.0, -1000, 1000);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);	
}



