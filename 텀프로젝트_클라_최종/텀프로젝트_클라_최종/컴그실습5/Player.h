#pragma once
using namespace std;
#include "Globals.h"

class Player
{
public:
	POSITION pos;
	COLOR color;
	int hp;
	unsigned int speed;
	unsigned int size;

	Player()
	{
		pos.x = 0;
		pos.y = 0;
	}
};