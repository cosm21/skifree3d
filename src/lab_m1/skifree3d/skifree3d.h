#pragma once

#include "components/simple_scene.h"
#include "components/transform.h"

#include <unordered_map>
#include <string>

namespace m1
{
	class SkiFree3D : public gfxc::SimpleScene
	{
	protected:
		struct Obstacle;
		struct LightSource;

	public:
		SkiFree3D();
		~SkiFree3D();

		void Init() override;

	private:
		void FrameStart() override;
		void Update(float deltaTimeSeconds) override;
		void FrameEnd() override;

		void OnInputUpdate(float deltaTime, int mods) override;
		void OnKeyPress(int key, int mods) override;
		void OnKeyRelease(int key, int mods) override;
		void OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) override;
		void OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) override;
		void OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) override;
		void OnWindowResize(int width, int height) override;

		void RegisterSnowMesh();

		glm::vec3 GetPlayerDirection();

		void RenderMesh(Mesh *mesh, const glm::mat4 &modelMatrix,
				Texture2D *texture = NULL, bool keepStill = false);

		void RenderPlayer(const glm::mat4& modelMatrix);
		void RenderObstacle(const Obstacle &obstacle);
		void RenderSnow(const glm::mat4& modelMatrix);

		Obstacle CreateObstacle(int kind, const glm::mat4& modelMatrix);
		void GenerateInitialObstacles(int numObstacles);
		bool ObstacleInBounds(const Obstacle& obstacle);
		void CleanObstacles();
		bool GenerateObstacle();
		int CheckPlayerCollision();
		void Reset();
		void UpdateCamera();

	protected:
		glm::mat4 viewMatrix;
		float globalPrecision;

		std::unordered_map<std::string, Texture2D*> mapTextures;

		float globalLightAttenuation;
		float materialKd;
		float materialKs;
		int materialShininess;

		int initialObstacles;
		std::vector<Obstacle> obstacles;
		std::vector<glm::vec3> colors;
		
		float snowStep, playerStep;
		glm::vec3 rotatedOY, rotatedOZ;
		glm::vec3 playerPosition;

		glm::vec3 snowScaleVector;
		float snowLength;

		float playerSpeed, initialPlayerSpeed;
		float playerCollisionHeight;
		float playerCollisionRadius;

		float minPlayerStep, maxPlayerStep;
		float minMouseProc, maxMouseProc;

		float spawnTime, currentSpawnTime;
		float minObstacleDistance;
		float initialDistanceFromPlayer;
		int maxObstacles, attempts;
		float minObstacleSpawnStep, maxObstacleSpawnStep;
		float procSpawnDistance, procDespawnDistance;

		int score;
		bool playerLost;

		float cameraAngle;
		float cameraUpDistance, cameraPlayerDistance;
		float mousePrecision;
		glm::vec3 cameraPosition;
	};
}

