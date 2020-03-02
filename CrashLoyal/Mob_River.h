#pragma once

#include "Mob.h"
#include <cfloat>

class Mob_River : public Mob
{
public:
	virtual int GetMaxHealth() const { return 4; }
	virtual float GetSpeed() const { return 1.0f; }
	virtual float GetSize() const { return 4.0f; }
	virtual float GetMass() const { return FLT_MAX; }
	virtual int GetDamage() const { return 0; }
	virtual float GetAttackTime() const { return 0.0f; }
	const char* GetDisplayLetter() const { return ""; }
};