#include "Mob.h"

#include <memory>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include "Building.h"
#include "Waypoint.h"
#include "GameState.h"
#include "Point.h"
#include <cmath>  

int Mob::previousUUID;

Mob::Mob() 
	: pos(-10000.f,-10000.f)
	, nextWaypoint(NULL)
	, targetPosition(new Point)
	, state(MobState::Moving)
	, uuid(Mob::previousUUID + 1)
	, attackingNorth(true)
	, health(-1)
	, targetLocked(false)
	, target(NULL)
	, lastAttackTime(0)
{
	Mob::previousUUID += 1;
}

void Mob::Init(const Point& pos, bool attackingNorth)
{
	health = GetMaxHealth();
	this->pos = pos;
	this->attackingNorth = attackingNorth;
	findClosestWaypoint();
}

std::shared_ptr<Point> Mob::getPosition() {
	return std::make_shared<Point>(this->pos);
}

bool Mob::findClosestWaypoint() {
	std::shared_ptr<Waypoint> closestWP = GameState::waypoints[0];
	float smallestDist = std::numeric_limits<float>::max();

	for (std::shared_ptr<Waypoint> wp : GameState::waypoints) {
		//std::shared_ptr<Waypoint> wp = GameState::waypoints[i];
		// Filter out any waypoints that are "behind" us (behind is relative to attack dir
		// Remember y=0 is in the top left
		if (attackingNorth && wp->pos.y > this->pos.y) {
			continue;
		}
		else if ((!attackingNorth) && wp->pos.y < this->pos.y) {
			continue;
		}

		float dist = this->pos.dist(wp->pos);
		if (dist < smallestDist) {
			smallestDist = dist;
			closestWP = wp;
		}
	}
	std::shared_ptr<Point> newTarget = std::shared_ptr<Point>(new Point);
	this->targetPosition->x = closestWP->pos.x;
	this->targetPosition->y = closestWP->pos.y;
	this->nextWaypoint = closestWP;
	
	return true;
}

void Mob::moveTowards(std::shared_ptr<Point> moveTarget, double elapsedTime) {
	Point movementVector;
	movementVector.x = moveTarget->x - this->pos.x;
	movementVector.y = moveTarget->y - this->pos.y;
	movementVector.normalize();
	movementVector *= (float)this->GetSpeed();
	movementVector *= (float)elapsedTime;
	pos += movementVector;
}


void Mob::findNewTarget() {
	// Find a new valid target to move towards and update this mob
	// to start pathing towards it

	if (!findAndSetAttackableMob()) { findClosestWaypoint(); }
}

// Have this mob start aiming towards the provided target
// TODO: impliment true pathfinding here
void Mob::updateMoveTarget(std::shared_ptr<Point> target) {
	this->targetPosition->x = target->x;
	this->targetPosition->y = target->y;
}

void Mob::updateMoveTarget(Point target) {
	this->targetPosition->x = target.x;
	this->targetPosition->y = target.y;
}


// Movement related
//////////////////////////////////
// Combat related

int Mob::attack(int dmg) {
	this->health -= dmg;
	return health;
}

bool Mob::findAndSetAttackableMob() {
	// Find an attackable target that's in the same quardrant as this Mob
	// If a target is found, this function returns true
	// If a target is found then this Mob is updated to start attacking it
	bool findAttackable = false;
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		if (otherMob->attackingNorth == this->attackingNorth) { continue; }

		bool imLeft    = this->pos.x     < (SCREEN_WIDTH / 2);
		bool otherLeft = otherMob->pos.x < (SCREEN_WIDTH / 2);

		bool imTop    = this->pos.y     < (SCREEN_HEIGHT / 2);
		bool otherTop = otherMob->pos.y < (SCREEN_HEIGHT / 2);
		if ((imLeft == otherLeft) && (imTop == otherTop)) {
			// If we're in the same quardrant as the otherMob
			// Mark it as the new target
			this->setAttackTarget(otherMob);
			findAttackable = true;
		}
	}
	return findAttackable;
}

// TODO Move this somewhere better like a utility class
int randomNumber(int minValue, int maxValue) {
	// Returns a random number between [min, max). Min is inclusive, max is not.
	return (rand() % maxValue) + minValue;
}

void Mob::setAttackTarget(std::shared_ptr<Attackable> newTarget) {
	this->state = MobState::Attacking;
	if (target == NULL) {
		target = newTarget;
		return;
	}
	int curDistance = std::abs(target->getPosition()->x - this->getPosition()->x) + std::abs(target->getPosition()->y - this->getPosition()->y);
	if (curDistance > std::abs(newTarget->getPosition()->x - this->getPosition()->x) + std::abs(newTarget->getPosition()->y - this->getPosition()->y)) {
		target = newTarget;
	}
}

bool Mob::targetInRange() {
	float range = this->GetSize(); // TODO: change this for ranged units
	float totalSize = range + target->GetSize();
	return this->pos.insideOf(*(target->getPosition()), totalSize);
}
// Combat related
////////////////////////////////////////////////////////////
// Collisions

// PROJECT 3: 
//  1) return a vector of mobs that we're colliding with
//  2) handle collision with towers & river 
std::vector<std::shared_ptr<Mob>> Mob::checkCollision() {
	std::vector<std::shared_ptr<Mob>> collisionMobs;
	for (std::shared_ptr<Mob> otherMob : GameState::mobs) {
		// don't collide with yourself
		if (this->sameMob(otherMob)) { continue; }

		// PROJECT 3: YOUR CODE CHECKING FOR A COLLISION GOES HERE
		int x = this->getPosition()->x;
		int y = this->getPosition()->y;
		if (std::abs(x - otherMob->getPosition()->x + 0.5) <= this->GetSize() + otherMob->GetSize()
			&& std::abs(y - otherMob->getPosition()->y + 0.5) <= this->GetSize() + otherMob->GetSize()) {
			collisionMobs.push_back(otherMob);
		}
	}
	return collisionMobs;
}

// process collision for mobs
void Mob::processCollision(std::shared_ptr<Mob> otherMob, double elapsedTime) {
	// PROJECT 3: YOUR COLLISION HANDLING CODE GOES HERE
	if (this->GetMass() <= otherMob->GetMass()) {
		Point spacing;
		spacing.x = this->pos.x - otherMob->getPosition()->x;
		spacing.y = this->pos.y - otherMob->getPosition()->y;
		spacing.normalize();
		if (std::abs(spacing.x) <= std::abs(spacing.y)) {
			if (spacing.x <= 0.0f && spacing.y >= 0.0f) {
				spacing.x -= 0.5f;
				spacing.y -= 0.5f;
			}
			else if (spacing.x > 0.0f && spacing.y >= 0.0f) {
				spacing.x += 0.5f;
					spacing.y -= 0.5f;
			}
			else if (spacing.x <= 0.0f && spacing.y <= 0.0f) {
				spacing.x -= 0.5f;
				spacing.y += 0.5f;
			}
			else if (spacing.x > 0.0f && spacing.y <= 0.0f) {
				spacing.x += 0.5f;
				spacing.y += 0.5f;
			}
		}
		else {
			if (spacing.x <= 0.0f && spacing.y >= 0.0f) {
				spacing.x += 0.5f;
				spacing.y += 0.5f;
			}
			else if (spacing.x > 0.0f && spacing.y >= 0.0f) {
				spacing.x -= 0.5f;
				spacing.y += 0.5f;
			}
			else if (spacing.x <= 0.0f && spacing.y <= 0.0f) {
				spacing.x += 0.5f;
				spacing.y -= 0.5f;
			}
			else if (spacing.x > 0.0f && spacing.y <= 0.0f) {
				spacing.x -= 0.5f;
				spacing.y -= 0.5f;
			}
		}
			
		spacing *= (float)this->GetSpeed();
		spacing *= (float)elapsedTime;

		pos += spacing;
	}
	
}

// handle collision between mobs and building.
std::shared_ptr<Building> Mob::checkBuildingCollision() {
	for (std::shared_ptr<Building> b : GameState::buildings) {
		int x = this->getPosition()->x;
		int y = this->getPosition()->y;
		if (std::abs(x - b->getPosition()->x + 0.5) <= (this->GetSize() + b->GetSize()) / 2
			&& std::abs(y - b->getPosition()->y + 0.5) <= (this->GetSize() + b->GetSize()) / 2) {
			return b;
		}
	}
	return std::shared_ptr<Building>(nullptr);
}

// process collision between mobs and building.
void Mob::processBuildingCollision(std::shared_ptr<Building> building, double elapsedTime) {
	Point spacing;
	spacing.x = this->pos.x - building->getPosition()->x;
	spacing.y = this->pos.y - building->getPosition()->y;
	spacing.normalize();
	if (spacing.x <= 0.0f && spacing.y >= 0.0f) {
		spacing.x -= 0.2f;
		spacing.y -= 0.2f;
	}
	else if (spacing.x > 0.0f && spacing.y >= 0.0f) {
		spacing.x += 0.2f;
		spacing.y -= 0.2f;
	}
	else if (spacing.x <= 0.0f && spacing.y <= 0.0f) {
		spacing.x -= 0.2f;
		spacing.y += 0.2f;
	}
	else if (spacing.x > 0.0f && spacing.y <= 0.0f) {
		spacing.x += 0.2f;
		spacing.y += 0.2f;
	}
	spacing *= (float)this->GetSpeed();
	spacing *= (float)elapsedTime;

	pos += spacing;
}

// handle collision between mobs and river.
std::shared_ptr<Point> Mob::checkRiverCollision() {
	int x = this->getPosition()->x;
	int y = this->getPosition()->y;

	// check left river 
	int leftRiverX = (int) RIVER_LEFT_X;
	int leftRiverY = (int) RIVER_TOP_Y;
	int leftRiverW = (int)(LEFT_BRIDGE_CENTER_X - BRIDGE_WIDTH / 2.0);
	int leftRiverH = (int)(RIVER_BOT_Y - RIVER_TOP_Y);
	if (x >= leftRiverX && x <= leftRiverX + leftRiverW && y >= leftRiverY && y <= leftRiverY + leftRiverH) {
		Point leftRiverPos(leftRiverX, leftRiverY);
		return std::make_shared<Point>(leftRiverPos);
	}

	// check middle river 
	int midRiverX = (int)(LEFT_BRIDGE_CENTER_X + BRIDGE_WIDTH / 2.0 - 0.5);
	int midRiverY = (int)RIVER_TOP_Y;
	int midRiverW = (int)(RIGHT_BRIDGE_CENTER_X - LEFT_BRIDGE_CENTER_X - BRIDGE_WIDTH);
	int midRiverH = (int)(RIVER_BOT_Y - RIVER_TOP_Y);
	if (x >= midRiverX && x <= midRiverX + midRiverW && y >= midRiverY && y <= midRiverY + midRiverH) {
		Point midRiverPos(midRiverX, midRiverY);
		return std::make_shared<Point>(midRiverPos);
	}

	// check right river 
	int rightRiverX = (int)(RIGHT_BRIDGE_CENTER_X + BRIDGE_WIDTH / 2.0 - 0.5);
	int rightRiverY = (int)RIVER_TOP_Y;
	int rightRiverW = (int)(SCREEN_WIDTH - RIGHT_BRIDGE_CENTER_X - BRIDGE_WIDTH / 2.0);
	int rightRiverH = (int)(RIVER_BOT_Y - RIVER_TOP_Y);
	if (x >= rightRiverX && x <= rightRiverX + rightRiverW && y >= rightRiverY && y <= rightRiverY + rightRiverH) {
		Point rightRiverPos(rightRiverX, rightRiverY);
		return std::make_shared<Point>(rightRiverPos);
	}

	return std::shared_ptr<Point>(nullptr);
}

// process collision between mobs and river.
void Mob::processRiverCollision(std::shared_ptr<Point> river, double elapsedTime) {
	Point spacing;
	spacing.x = this->pos.x - targetPosition->x;
	spacing.y = this->pos.y - targetPosition->y;
	spacing.normalize();
	if (river->x == RIVER_LEFT_X) {
		spacing.x += 0.5f;
	}
	else if (river->x == (int)(LEFT_BRIDGE_CENTER_X + BRIDGE_WIDTH / 2.0 - 0.5)) {
		int mid = river->x + (int)(RIGHT_BRIDGE_CENTER_X - LEFT_BRIDGE_CENTER_X - BRIDGE_WIDTH) / 2;
		if (this->pos.x <= mid) {
			spacing.x -= 0.5f;
		}
		else {
			spacing.x += 0.5f;
		}
	}
	else {
		spacing.x -= 0.5f;
	}
	spacing *= (float)this->GetSpeed();
	spacing *= (float)elapsedTime;

	pos += spacing;
}


// Collisions
///////////////////////////////////////////////
// Procedures

void Mob::attackProcedure(double elapsedTime) {
	if (this->target == nullptr || this->target->isDead()) {
		this->targetLocked = false;
		this->target = nullptr;
		this->state = MobState::Moving;
		return;
	}

	if (targetInRange()) {
		if (this->lastAttackTime >= this->GetAttackTime()) {
			// If our last attack was longer ago than our cooldown
			this->target->attack(this->GetDamage());
			this->lastAttackTime = 0; // lastAttackTime is incremented in the main update function
			return;
		}
	}
	else {
		// If the target is not in range
		moveTowards(target->getPosition(), elapsedTime);
		std::vector<std::shared_ptr<Mob>> otherMobs = this->checkCollision();
		for (std::shared_ptr<Mob> otherMob : otherMobs) {
			this->processCollision(otherMob, elapsedTime);
		}
		std::shared_ptr<Building> building = this->checkBuildingCollision();
		if (building) {
			this->processBuildingCollision(building, elapsedTime);
		}
		std::shared_ptr<Point> river = this->checkRiverCollision();
		if (river) {
			findClosestWaypoint();
			this->processRiverCollision(river, elapsedTime);
		}
	}
}

void Mob::moveProcedure(double elapsedTime) {
	if (targetPosition) {
		moveTowards(targetPosition, elapsedTime);

		// Check for collisions
		if (this->nextWaypoint->pos.insideOf(this->pos, (this->GetSize() + WAYPOINT_SIZE))) {
			std::shared_ptr<Waypoint> trueNextWP = this->attackingNorth ?
												   this->nextWaypoint->upNeighbor :
												   this->nextWaypoint->downNeighbor;
			setNewWaypoint(trueNextWP);
		}

		// PROJECT 3: You should not change this code very much, but this is where your 
		// collision code will be called from
		std::vector<std::shared_ptr<Mob>> otherMobs = this->checkCollision();
		for (std::shared_ptr<Mob> otherMob : otherMobs) {
			if (otherMob) {
				this->processCollision(otherMob, elapsedTime);
			}
		}

		std::shared_ptr<Building> building = this->checkBuildingCollision();
		if (building) {
			this->processBuildingCollision(building, elapsedTime);
		}

		std::shared_ptr<Point> river = this->checkRiverCollision();
		if (river) {
			this->processRiverCollision(river, elapsedTime);
		}

		// Fighting otherMob takes priority always
		findAndSetAttackableMob();
		findNewTarget();


	} else {
		// if targetPosition is nullptr
		findNewTarget();
	}
}

void Mob::update(double elapsedTime) {

	switch (this->state) {
	case MobState::Attacking:
		this->attackProcedure(elapsedTime);
		break;
	case MobState::Moving:
	default:
		this->moveProcedure(elapsedTime);
		break;
	}

	this->lastAttackTime += (float)elapsedTime;
}
