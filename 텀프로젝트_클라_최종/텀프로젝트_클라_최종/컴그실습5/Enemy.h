#pragma once
#include <random>
#include "Globals.h"
using namespace std;

class Enemy
{
public:
	unsigned int speed;
	POSITION pos;
	unsigned int size;
	unsigned int damage;
	COLOR color;
};