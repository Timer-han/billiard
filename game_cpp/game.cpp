#include <iostream>
#include <cassert>
#include <cmath>
#include <array>

#include "../framework/scene.hpp"
#include "../framework/game.hpp"
#include "../framework/engine.hpp"


//-------------------------------------------------------
//	Basic Vector2 class
//-------------------------------------------------------

class Vector2
{
public:
	float x = 0.f;
	float y = 0.f;

	constexpr Vector2() = default;
	constexpr Vector2( float vx, float vy );
	constexpr Vector2( Vector2 const &other ) = default;
};


constexpr Vector2::Vector2( float vx, float vy ) :
	x( vx ),
	y( vy )
{
}


//-------------------------------------------------------
//	game parameters
//-------------------------------------------------------

namespace Params
{
	namespace System
	{
		constexpr int targetFPS = 60;
	}

	namespace Table
	{
		constexpr float width = 15.f;
		constexpr float height = 8.f;
		constexpr float pocketRadius = 0.4f;

		static constexpr std::array< Vector2, 6 > pocketsPositions =
		{
			Vector2{ -0.5f * width, -0.5f * height },
			Vector2{ 0.f, -0.5f * height },
			Vector2{ 0.5f * width, -0.5f * height },
			Vector2{ -0.5f * width, 0.5f * height },
			Vector2{ 0.f, 0.5f * height },
			Vector2{ 0.5f * width, 0.5f * height }
		};

		static constexpr std::array< Vector2, 7 > ballsPositions =
		{
			// player ball
			Vector2( -0.3f * width, 0.f ),
			// other balls
			Vector2( 0.2f * width, 0.f ),
			Vector2( 0.25f * width, 0.05f * height ),
			Vector2( 0.25f * width, -0.05f * height ),
			Vector2( 0.3f * width, 0.1f * height ),
			Vector2( 0.3f * width, 0.f ),
			Vector2( 0.3f * width, -0.1f * height )
		};
	}

	namespace Ball
	{
		constexpr float radius = 0.3f;
	}

	namespace Shot
	{
		constexpr float chargeTime = 1.f;
	}
}


//-------------------------------------------------------
//	Table logic
//-------------------------------------------------------

class Table
{
public:
	Table() = default;
	Table(Table const&) = delete;

	void init();
	void deinit();


public:
	float ballData[7][6]; // 0 - position X, 1 - position Y, 2 - tan of angle, 3 - velocity of X, 4 - velocity of Y, 5 - existing on table
	Vector2 pocketData[6];
	std::array< Scene::Mesh*, 6 > pockets = {};
	std::array< Scene::Mesh*, 7 > balls = {};
};





void Table::init()
{
	for (int i = 0; i < 6; i++)
	{
		assert(!pockets[i]);
		pockets[i] = Scene::createPocketMesh(Params::Table::pocketRadius);
		Scene::placeMesh(pockets[i], Params::Table::pocketsPositions[i].x, Params::Table::pocketsPositions[i].y, 0.f);
		pocketData[i].x = Params::Table::pocketsPositions[i].x;
		pocketData[i].y = Params::Table::pocketsPositions[i].y;
	}

	for (int i = 0; i < 7; i++)
	{
		assert(!balls[i]);
		balls[i] = Scene::createBallMesh(Params::Ball::radius);
		Scene::placeMesh(balls[i], Params::Table::ballsPositions[i].x, Params::Table::ballsPositions[i].y, 0.f);
		ballData[i][0] = Params::Table::ballsPositions[i].x;
		ballData[i][1] = Params::Table::ballsPositions[i].y;
		ballData[i][2] = 0.f;
		ballData[i][3] = 0.f;
		ballData[i][4] = 0.f;
		ballData[i][5] = 1.f;

	}
}


void Table::deinit()
{
	for (Scene::Mesh* mesh : pockets)
		Scene::destroyMesh(mesh);
	for (Scene::Mesh* mesh : balls)
		Scene::destroyMesh(mesh);
	pockets = {};
	balls = {};
}


//-------------------------------------------------------
//	game public interface
//-------------------------------------------------------

namespace Game
{
	Table table;

	bool isChargingShot = false;
	float shotChargeProgress = 0.f;

	void init()
	{
		Engine::setTargetFPS(Params::System::targetFPS);
		Scene::setupBackground(Params::Table::width, Params::Table::height);
		table.init();
	}


	void deinit()
	{
		table.deinit();
	}


	void update(float dt)
	{
		float velocity;
		if (isChargingShot)
			shotChargeProgress = std::min(shotChargeProgress + dt / Params::Shot::chargeTime, 1.f);
		Scene::updateProgressBar(shotChargeProgress);
		for (int i = 0; i < 7; i++) {
			if (table.ballData[i][5] < 0) {
				continue;
			}
			
			for (int j = 0; j < 6; j++) {
				// Removing a ball from the table if it hits a hole
				if (pow(table.ballData[i][0] - table.pocketData[j].x, 2) + pow(table.ballData[i][1] - table.pocketData[j].y, 2) <= pow(0.55f, 2)) {
					Scene::placeMesh(table.balls[i], -10, -10, 0);
					table.ballData[i][0] = -10;
					table.ballData[i][1] = -10;
					table.ballData[i][2] = 0;
					table.ballData[i][3] = 0;
					table.ballData[i][5] = -1.f;
					break;
				}
			}
			if (table.ballData[i][5] < 0) {
				continue;
			}

			// Сhanging the position of the ball
			table.ballData[i][0] += table.ballData[i][3];
			table.ballData[i][1] += table.ballData[i][4];

			// Ball braking
			velocity = pow(pow(table.ballData[i][3], 2) + pow(table.ballData[i][4], 2), 0.002);
			table.ballData[i][3] = table.ballData[i][3] * velocity * 0.997;
			table.ballData[i][4] = table.ballData[i][4] * velocity * 0.997;

			// touching the wall
			if (fabs(table.ballData[i][0] - 8.2) < 1 && table.ballData[i][3] > 0 || fabs(table.ballData[i][0] + 8.2) < 1 && table.ballData[i][3] < 0){
				table.ballData[i][3] *= -0.5;
				table.ballData[i][2] *= -1;
			}
			if (fabs(table.ballData[i][1] - 4.7) < 1 && table.ballData[i][4] > 0 || fabs(table.ballData[i][1] + 4.7) < 1 && table.ballData[i][4] < 0) {
				table.ballData[i][2] *= -1;
				table.ballData[i][4] *= -0.5;
			}

			// touching the ball
			for (int j = 0; j < 7; j++) {
				if (i == j || table.ballData[j][5] < 0) {
					continue;
				}
				if (pow(table.ballData[i][0] - table.ballData[j][0], 2) + pow(table.ballData[i][1] - table.ballData[j][1], 2) <= pow(0.599f, 2)) {
					
					table.ballData[j][2] = (table.ballData[i][1] - table.ballData[j][1]) / (table.ballData[i][0] - table.ballData[j][0]);
					table.ballData[i][2] = table.ballData[j][2];

					velocity = sqrt(pow(table.ballData[i][3], 2) + pow(table.ballData[i][4], 2));
					table.ballData[j][3] -= velocity / sqrt(1 + pow(table.ballData[j][2], 2)) * (table.ballData[i][0] - table.ballData[j][0] > 0 ? 1 : -1);
					table.ballData[j][4] -= velocity / sqrt(1 + pow(table.ballData[j][2], 2)) * (table.ballData[j][2]) * (table.ballData[i][0] - table.ballData[j][0] > 0 ? 1 : -1);

					velocity = sqrt(pow(table.ballData[i][3], 2) + pow(table.ballData[i][4], 2));
					table.ballData[i][3] -= velocity / sqrt(1 + pow(table.ballData[i][2], 2)) * (table.ballData[j][0] - table.ballData[i][0] < 0 ? -1 : 1);
					table.ballData[i][4] -= velocity / sqrt(1 + pow(table.ballData[i][2], 2)) * (table.ballData[j][2]) * (table.ballData[j][0] - table.ballData[i][0] < 0 ? -1 : 1);

					table.ballData[i][3] *= 0.8;
					table.ballData[j][3] *= 0.8;
					table.ballData[i][4] *= 0.8;
					table.ballData[j][4] *= 0.8;

				}
				
			}
			// saving changes
			Scene::placeMesh(table.balls[i], table.ballData[i][0], table.ballData[i][1], table.ballData[i][2]);
		}
	}


	void mouseButtonPressed( float x, float y )
	{
		isChargingShot = true;
	}


	void mouseButtonReleased( float x, float y )
	{
		// TODO: implement billiard logic here

		// saving angle and velocity
		table.ballData[0][2] = (y - table.ballData[0][1]) / (x - table.ballData[0][0]);
		table.ballData[0][3] = shotChargeProgress / sqrt((1 + pow(table.ballData[0][2], 2))) * (x - table.ballData[0][0] > 0 ? 1 : -1) / 2;
		table.ballData[0][4] = shotChargeProgress / sqrt((1 + pow(table.ballData[0][2], 2))) * table.ballData[0][2] * (x - table.ballData[0][0] > 0 ? 1 : -1) / 2;


		Scene::placeMesh(table.balls[0], table.ballData[0][0], table.ballData[0][1], table.ballData[0][2]);


		isChargingShot = false;
		shotChargeProgress = 0.f;
	}
}
