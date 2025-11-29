#ifndef PROP_MONEY_H
#define PROP_MONEY_H
#pragma once

#include <props.h>

#define MONEY_DROP_SPIRAL_STEP_ANGLE 42.0f
#define MONEY_DROP_SPIRAL_STEP_RAISE 3.0f
#define MONEY_DROP_SPIRAL_MAX_HEIGHT 100.0f // Max. height relative to the bottom before resetting spiral origin

#define MONEY_DROP_SPAWN_THROW_PITCH 45.0f // Elevation angle for spawn velocity
#define MONEY_DROP_SPAWN_VELOCITY    60.0f

class CMoneyProp : public CPhysicsProp
{
public:
	int mAmount = 0;
};

#endif // !PROP_MONEY_H
