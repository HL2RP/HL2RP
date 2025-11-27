#ifndef PROP_MONEY_H
#define PROP_MONEY_H
#pragma once

#include <props.h>

#define MONEY_DROP_CIRCLE_RADIUS     20.0f
//#define MONEY_DROP_CIRCLE_STEP_ANGLE 42.0f
#define MONEY_DROP_CIRCLE_STEP_ANGLE 40.0f // Plugin

#define MONEY_DROP_SPIRAL_STEP_RAISE 3.0f

#define MONEY_DROP_SPAWN_VELOCITY_UP 45.0f
#define MONEY_DROP_SPAWN_VELOCITY_2D 45.0f

class CMoneyProp : public CPhysicsProp
{
public:
	int mAmount = 0;
};

#endif // !PROP_MONEY_H
