#include "lab_m1/skifree3d/skifree3d.h"

#include <iostream>
#include <cmath>
#include <time.h>

#define NUM_TYPES_OBSTACLES 4

using namespace m1;
using namespace std;

enum obstacle_type {
	OT_TREE,
	OT_ROCK,
	OT_POLE,
	OT_GIFT
};


struct SkiFree3D::LightSource {
	glm::vec3 position;
	glm::vec3 color;
	float constAtt;
	float linAtt;
	float sqAtt;

	bool spotlight;
	glm::vec3 direction;
	float cutOff;
};

struct SkiFree3D::Obstacle {
	glm::mat4 modelMatrix;
	int kind;

	glm::vec3 collisionPos;
	float radius;

	bool hasLight1;
	LightSource light1;
	bool hasLight2;
	LightSource light2;

	// Verifica daca cele doua obstacole nu se suprapun, optional putand verifica daca se
	// afla prea aproape, tinand cont de o distanta specificata
	bool collides(const Obstacle& other, const float od = 0) {
		return glm::distance(collisionPos, other.collisionPos) < (radius + other.radius + od);
	}
};

SkiFree3D::SkiFree3D() {
	// Atenuarea luminii globale NU depinde de distanta
	// factorul face lumina globala sa nu fie prea puternica
	globalLightAttenuation = 0.7f;
	materialKd = 0.7;
	materialKs = 0.4;
	materialShininess = 40;

	globalPrecision = pow(10, 3);

	snowStep = 30 * M_PI / 180;
	// Se salveaza axele rotite pentru a fi folosite mai tarziu, axa ox este aceeasi
	rotatedOY = glm::vec3(glm::rotate(glm::mat4(1.f), snowStep, glm::vec3(1, 0, 0)) * glm::vec4(0.f, 1.f, 0.f, 1.f));
	rotatedOZ = glm::vec3(glm::rotate(glm::mat4(1.f), snowStep, glm::vec3(1, 0, 0)) * glm::vec4(0.f, 0.f, 1.f, 1.f));
	rotatedOY = glm::normalize(rotatedOY);
	rotatedOZ = glm::normalize(rotatedOZ);
	playerPosition = glm::vec3(0.f);
	playerStep = 0.f;

	snowScaleVector = glm::vec3(1.f);
	snowLength = 50.f * max(snowScaleVector.x, max(snowScaleVector.y, snowScaleVector.z));

	playerLost = false;
	score = 0;
	playerCollisionHeight = 0.575f;
	playerCollisionRadius = 0.5f;

	minPlayerStep = -45 * M_PI / 180;
	maxPlayerStep = 45 * M_PI / 180;
	minMouseProc = 0.3f;
	maxMouseProc = 0.7f;

	// Culorile pe care le pot avea luminile de la copaci si de la cadouri
	colors = {
		glm::vec3(0.7f, 0.f, 0.f),
		glm::vec3(0.7f, 0.7f, 0.f),
		glm::vec3(0.f, 0.f, 0.7f)
	};

	initialObstacles = 25;
	minObstacleDistance = 2.f;
	initialDistanceFromPlayer = 2.5f;	
	attempts = 20;
	currentSpawnTime = 0;
	// Cat de mult timp trebuie sa treaca ca sa generez un nou obstacol, in secunde
	spawnTime = 0.18f;
	minObstacleSpawnStep = -45 * M_PI / 180;
	maxObstacleSpawnStep = 45 * M_PI / 180;
	// Cat de aproape fata de jumatate din lungimea planului pot sa apara obstacole
	procSpawnDistance = 0.7f;
	// Cat de departe trebuie sa fie obstacolele ca sa fie sterse, exprimat ca
	// procent din jumatate din lungimea laturii planului
	procDespawnDistance = 0.75f;

	cameraAngle = 20 * M_PI / 180;
	cameraUpDistance = 15.f;
	cameraPlayerDistance = 1.5f;
	cameraPosition = glm::vec3(0.f);
	mousePrecision = 0.005f;

	initialPlayerSpeed = playerSpeed = 10.f;
}

SkiFree3D::~SkiFree3D() {

}

void SkiFree3D::Init() {
	Mesh *mesh;
	Texture2D *texture;
	Shader *shader;

	mesh = new Mesh("plane");
	mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "plane50.obj");
	meshes[mesh->GetMeshID()] = mesh;

	mesh = new Mesh("box");
	mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "box.obj");
	meshes[mesh->GetMeshID()] = mesh;

	mesh = new Mesh("sphere");
	mesh->LoadMesh(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::MODELS, "primitives"), "sphere.obj");
	meshes[mesh->GetMeshID()] = mesh;

	mesh = new Mesh("cone");
	mesh->LoadMesh(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "skifree3d", "meshes"), "cone.obj");
	meshes[mesh->GetMeshID()] = mesh;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "skifree3d", "textures", "snow.jpg").c_str(), GL_REPEAT);
	mapTextures["snow"] = texture;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "crate.jpg").c_str(), GL_REPEAT);
	mapTextures["crate"] = texture;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "skifree3d", "textures", "rock.png").c_str(), GL_REPEAT);
	mapTextures["rock"] = texture;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "skifree3d", "textures", "wood.jpg").c_str(), GL_REPEAT);
	mapTextures["wood"] = texture;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "skifree3d", "textures", "tree.jpg").c_str(), GL_REPEAT);
	mapTextures["tree"] = texture;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "skifree3d", "textures", "concrete.jpg").c_str(), GL_REPEAT);
	mapTextures["concrete"] = texture;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1, "skifree3d", "textures", "gift.jpg").c_str(), GL_REPEAT);
	mapTextures["gift"] = texture;

	texture = new Texture2D();
	texture->Load2D(PATH_JOIN(window->props.selfDir, RESOURCE_PATH::TEXTURES, "default.png").c_str(), GL_REPEAT);	
	mapTextures["default"] = texture;

	shader = new Shader("CustomShader");
	shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1,
				"skifree3d", "shaders", "VertexShader.glsl"), GL_VERTEX_SHADER);
	shader->AddShader(PATH_JOIN(window->props.selfDir, SOURCE_PATH::M1,
				"skifree3d", "shaders", "FragmentShader.glsl"), GL_FRAGMENT_SHADER);
	shader->CreateAndLink();
	shaders[shader->GetName()] = shader;

	srand(time(NULL));
	GenerateInitialObstacles(initialObstacles);
}

void SkiFree3D::RenderMesh(Mesh *mesh, const glm::mat4& modelMatrix, Texture2D *texture, bool keepStill)
{
	const auto pass_light = [](Shader *shader, int index, const LightSource& light) -> void {
		string base = "lights[" + to_string(index) + "].";
		string current;
		int loc;
		
		current = base + "position";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform3f(loc, light.position.x, light.position.y, light.position.z);

		current = base + "color";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform3f(loc, light.color.x, light.color.y, light.color.z);

		current = base + "const_att";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform1f(loc, light.constAtt);

		current = base + "lin_att";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform1f(loc, light.linAtt);

		current = base + "sq_att";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform1f(loc, light.sqAtt);

		current = base + "spotlight";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform1i(loc, light.spotlight);

		if (!light.spotlight)
			return;

		current = base + "direction";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform3f(loc, light.direction.x, light.direction.y, light.direction.z);

		current = base + "cut_off";
		loc = glGetUniformLocation(shader->program, current.c_str());
		glUniform1f(loc, light.cutOff);
	};

	if (!mesh)
		return;

	Shader *shader = shaders["CustomShader"];
	glUseProgram(shader->program);

	int loc = glGetUniformLocation(shader->program, "Model");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
	
	loc = glGetUniformLocation(shader->program, "View");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

	// Se foloseste matricea de proiectie de la camera deja implementata
	glm::mat4 projectionMatrix = GetSceneCamera()->GetProjectionMatrix();
	loc = glGetUniformLocation(shader->program, "Projection");
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	if (!texture)
		texture = mapTextures["default"];

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->GetTextureID());
	loc = glGetUniformLocation(shader->program, "texture");
	glUniform1i(loc, 0);

	loc = glGetUniformLocation(shader->program, "global_light_att");
	glUniform1f(loc, globalLightAttenuation);

	loc = glGetUniformLocation(shader->program, "material_kd");
	glUniform1f(loc, materialKd);

	loc = glGetUniformLocation(shader->program, "material_ks");
	glUniform1f(loc, materialKs);
	
	loc = glGetUniformLocation(shader->program, "material_shininess");
	glUniform1i(loc, materialShininess);

	loc = glGetUniformLocation(shader->program, "eye_position");
	glUniform3fv(loc, 1, glm::value_ptr(cameraPosition));

	int num_lights = 0;
	int num_obstacles = static_cast<int>(obstacles.size());
	for (int i = 0; i < num_obstacles; i++) {
		if (obstacles[i].hasLight1)
			pass_light(shader, num_lights++, obstacles[i].light1);

		if (obstacles[i].hasLight2)
			pass_light(shader, num_lights++, obstacles[i].light2);
	}

	loc = glGetUniformLocation(shader->program, "num_lights");
	glUniform1i(loc, num_lights);

	loc = glGetUniformLocation(shader->program, "keep_still");
	glUniform1i(loc, keepStill);
	
	if (keepStill) {
		loc = glGetUniformLocation(shader->program, "texture_offset");
		// O unitate in coordonate de textura trebuie sa corespunda la 50 de unitati ale planului cu zapada,
		// motiv pentru care apare termenul 1 / snowLength
		//
		// Din moment ce planul zapezii e inclinat fata de axa ox, modificarea coordonatei de textura pe y
		// corespunde cu pozitia jucatorului pe z in care trebuie luata in calcul si inclinarea planului
		glUniform2f(loc, (1 / snowLength) * playerPosition.x,
				(1 / (snowLength * cos(snowStep))) * playerPosition.z);
	}

	glBindVertexArray(mesh->GetBuffers()->m_VAO);
	glDrawElements(mesh->GetDrawMode(), static_cast<int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
}

void SkiFree3D::RenderPlayer(const glm::mat4& modelMatrix)
{
	// Schiorul
	glm::mat4 localModel = modelMatrix;
	localModel = glm::translate(localModel, glm::vec3(0.f, 0.575f, 0.f));
	RenderMesh(meshes["box"], localModel, mapTextures["crate"]);

	// Schiul din stanga
	localModel = modelMatrix;
	localModel = glm::translate(localModel, glm::vec3(-0.425f, 0.0375f, 0.f));
	localModel = glm::scale(localModel, glm::vec3(0.15f, 0.075f, 3.f));
	RenderMesh(meshes["box"], localModel);

	// Schiul din dreapta
	localModel = modelMatrix;
	localModel = glm::translate(localModel, glm::vec3(0.425f, 0.0375f, 0.f));
	localModel = glm::scale(localModel, glm::vec3(0.15f, 0.075f, 3.f));
	RenderMesh(meshes["box"], localModel);
}

void SkiFree3D::GenerateInitialObstacles(int numObstacles)
{
	Obstacle obstacle;
	float ox, oz;
	bool ok;
	glm::mat4 modelMatrix;
	glm::mat4 baseMatrix = glm::rotate(glm::mat4(1.f), snowStep, glm::vec3(1.f, 0.f, 0.f));
	int currentAttempts = 0;

	for (int i = 0; i < numObstacles; i++) {
		do {
			ok = true;

			// Se genereaza coordonatele pentru noul obstacol, exceptand un patrat
			// in jurul jucatorului
			ox = static_cast<float>(rand() % static_cast<int>(globalPrecision
						* (snowLength * 0.5f - initialDistanceFromPlayer)))
					* (1 / globalPrecision) + initialDistanceFromPlayer;
			oz = static_cast<float>(rand() % static_cast<int>(globalPrecision
						* (snowLength * 0.5f - initialDistanceFromPlayer)))
					* (1 / globalPrecision) + initialDistanceFromPlayer;

			if (rand() % 2)
				ox = -ox;
			if (rand() % 2)
				oz = -oz;

			modelMatrix = baseMatrix;
			modelMatrix = glm::translate(modelMatrix, glm::vec3(ox, 0, oz));
			obstacle = CreateObstacle(OT_TREE, modelMatrix);

			// Se verifica ca obstacolul generat sa nu fie prea aproape de alte obstacole
			for (int j = 0, size = obstacles.size(); j < size; j++) {
				if (obstacle.collides(obstacles[j], minObstacleDistance)) {
					ok = false;
					break;
				}
			}

		// Se incearca generarea de mai multe ori, dar se renunta daca nu se reuseste dupa un
		// numar dat de incercari
		} while (!ok && currentAttempts++ < attempts);

		if (!ok)
			continue;

		obstacles.push_back(obstacle);
	}
}

// Verifica daca obstacolul este in interiorul planului
//
// Se calculeaza unghiul format dintre obstacol si una dintre axele perpendiculare
// care determina planul
// Se alege cel mai mic unghi format cu una din axe, iar pe baza acestui unghi
// se calculeaza distanta maxima la care se poate afla obiectul in asa fel
// incat acesta sa se afle in interiorul planului, daca distanta reala
// fata de jucator este mai mica decat aceasta distanta atunci obiectul se afla in
// interiorul planului
bool SkiFree3D::ObstacleInBounds(const Obstacle& obstacle)
{
	// Pozitia sferei de coliziune este un pic peste plan pentru aproape toate
	// obstacolele, asa ca se face proiectia acesteia pe plan
	glm::vec3 vec = obstacle.collisionPos
		- static_cast<float>(glm::dot(obstacle.collisionPos, rotatedOY)
				/ pow(glm::length(rotatedOY), 2)) * rotatedOY;
	// Se ia vectorul de la jucator pana la obstacol
	vec -= playerPosition;
	float angle = acos(glm::dot(glm::normalize(vec), rotatedOZ));
	if (angle >= M_PI)
		angle -= M_PI;
	if (angle >= M_PI / 2)
		angle -= M_PI / 2;
	angle = M_PI / 4 - abs(angle - M_PI / 4);
	return glm::length(vec) <= procDespawnDistance * (snowLength / (2 * cos(angle)));
}

void SkiFree3D::CleanObstacles()
{
	auto it = obstacles.begin();
	while (it != obstacles.end()) {
		if (!ObstacleInBounds(*it))
			it = obstacles.erase(it);
		else
			it++;
	}
}

// Genereaza un singur obstacol, se foloseste dupa ce s-au generat obstacolele initiale
bool SkiFree3D::GenerateObstacle()
{
	glm::vec3 vec(0.f), oldVec(0.f);
	glm::vec3 direction = GetPlayerDirection();

	// Se foloseste directia jucatorului proiectata pe xOz
	direction.y = 0;
	direction = glm::normalize(direction);

	// Obstacolul va fi generat in interiorul planului, cat mai aproape de jucator,
	// pentru ca acesta sa fie nevoit sa-l ocoleasca
	oldVec += procSpawnDistance * (snowLength / 2) * direction;

	int numAttempts = 0;
	bool ok;
	Obstacle obstacle;
	do {
		ok = true;

		// Proiectia directiei jucatorului mai este rotita cu un unghi generat la intamplare
		float offset = static_cast<float>(rand() %
				static_cast<int>((maxObstacleSpawnStep - minObstacleSpawnStep)
					* globalPrecision))
			* (1 / globalPrecision) + minObstacleSpawnStep;
		
		vec = oldVec;
		vec = glm::vec3(glm::rotate(glm::mat4(1.f), offset, glm::vec3(0.f, 1.f, 0.f))
				* glm::vec4(vec, 1.f));
	
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1.f), playerPosition);
		modelMatrix = glm::rotate(modelMatrix, snowStep, glm::vec3(1.f, 0.f, 0.f));
		modelMatrix = glm::translate(modelMatrix, vec);
		obstacle = CreateObstacle(rand() % 4, modelMatrix);

		for (int i = 0, size = obstacles.size(); i < size; i++) {
			if (obstacle.collides(obstacles[i], minObstacleDistance)) {
				ok = false;
				break;
			}
		}
	} while(!ok && numAttempts++ < attempts);

	if (ok)
		obstacles.push_back(obstacle);

	return ok;
}

// Creeaza un obstacol, setand luminile si pozitiile sferelor de coliziune
SkiFree3D::Obstacle SkiFree3D::CreateObstacle(int kind, const glm::mat4& modelMatrix)
{
	Obstacle obstacle;	

	obstacle.kind = kind;
	obstacle.modelMatrix = modelMatrix;
	obstacle.hasLight1 = false;
	obstacle.hasLight2 = false;

	switch (kind) {
	case OT_TREE:
		obstacle.hasLight1 = true;
		obstacle.light1.position = glm::vec3(modelMatrix * glm::vec4(0.f, 0.25f, 0.f, 1.f));
		obstacle.light1.color = colors[rand() % colors.size()];
		obstacle.light1.constAtt = 0.2f;
		obstacle.light1.linAtt = 1.f;
		obstacle.light1.sqAtt = 0.2f;
		obstacle.light1.spotlight = false;

		obstacle.collisionPos = glm::vec3(modelMatrix * glm::vec4(0.f, 1.f, 0.f, 1.f));
		obstacle.radius = 1.2f;

		break;
	case OT_ROCK:
		obstacle.collisionPos = glm::vec3(modelMatrix * glm::vec4(0.f, 0.f, 0.f, 1.f));
		obstacle.radius = 0.5f;

		break;
	case OT_POLE:
		obstacle.hasLight1 = true;
		obstacle.light1.position = glm::vec3(modelMatrix * glm::vec4(-1.4f, 4.f, 0.f, 1.f));
		obstacle.light1.color = glm::vec3(1.f, 1.f, 1.f);
		obstacle.light1.constAtt = 0.3f;
		obstacle.light1.linAtt = 0.2f;
		obstacle.light1.sqAtt = 0.1f;
		obstacle.light1.spotlight = true;
		obstacle.light1.direction = glm::vec3(0.f, -1.f, 0.f);
		obstacle.light1.cutOff = 25 * M_PI / 180;

		obstacle.hasLight2 = true;
		obstacle.light2 = obstacle.light1;
		obstacle.light2.position = glm::vec3(modelMatrix * glm::vec4(1.4f, 4.f, 0.f, 1.f));

		obstacle.collisionPos = glm::vec3(modelMatrix * glm::vec4(0.f, 0.5f, 0.f, 1.f));
		obstacle.radius = 0.1f;

		break;
	case OT_GIFT:
		obstacle.hasLight1 = true;
		obstacle.light1.position = glm::vec3(modelMatrix * glm::vec4(0.f, 0.4f, 0.f, 1.f));	
		obstacle.light1.color = colors[rand() % colors.size()];
		obstacle.light1.constAtt = 0.3f;
		obstacle.light1.linAtt = 2.f;
		obstacle.light1.sqAtt = 1.f;
		obstacle.light1.spotlight = false;

		obstacle.collisionPos = glm::vec3(modelMatrix * glm::vec4(0.f, 0.5f, 0.f, 1.f));
		obstacle.radius = 0.5f;

		break;
	}
	
	return obstacle;
}

void SkiFree3D::RenderObstacle(const Obstacle& obstacle)
{
	glm::mat4 baseMatrix = obstacle.modelMatrix, modelMatrix;

	switch(obstacle.kind) {
	case OT_TREE:
		// Randeaza frunzele
		modelMatrix = baseMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.f, 2.5f, 0.f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.2f, 1.5f, 1.2f));
		RenderMesh(meshes["cone"], modelMatrix, mapTextures["tree"]);

		// Randeaza trunchiul
		modelMatrix = baseMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.f, 0.5f, 0.f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 1.f, 0.3f));
		RenderMesh(meshes["box"], modelMatrix, mapTextures["wood"]);
		break;
	case OT_ROCK:
		// Randeaza stanca din stanga
		modelMatrix = baseMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, 0.f, 0.f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(1.2f));
		RenderMesh(meshes["sphere"], modelMatrix, mapTextures["rock"]);

		// Randeaza stanca din dreapta
		modelMatrix = baseMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.3f, 0.f, 0.f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.9f));
		RenderMesh(meshes["sphere"], modelMatrix, mapTextures["rock"]);
		break;
	case OT_POLE:
		// Randeaza partea de sus a stalpului
		modelMatrix = baseMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.f, 4.1f, 0.f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(3.f, 0.2f, 0.2f));
		RenderMesh(meshes["box"], modelMatrix, mapTextures["concrete"]);

		// Randeaza partea de jos a stalpului
		modelMatrix = baseMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.f, 2.f, 0.f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f, 4.f, 0.2f));
		RenderMesh(meshes["box"], modelMatrix, mapTextures["concrete"]);
		break;
	case OT_GIFT:
		modelMatrix = baseMatrix;
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0.f, 0.4f, 0.f));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(0.8f));
		RenderMesh(meshes["box"], modelMatrix, mapTextures["gift"]);
		break;
	}
}

// Verifica daca jucatorul s-a lovit de un obstacol, intoarce indexul din lista
// al obstacolului cu care s-a lovit sau -1, altfel
int SkiFree3D::CheckPlayerCollision()
{
	glm::vec3 vec(playerPosition + playerCollisionHeight * rotatedOY);

	for (int i = 0, size = obstacles.size(); i < size; i++)
		if (glm::distance(obstacles[i].collisionPos, vec)
				< playerCollisionRadius + obstacles[i].radius)
			return i;
	return -1;
}

// Reinitializeaza anumiti parametrii atunci cand se reseteaza jocul
void SkiFree3D::Reset()
{
	playerLost = false;
	playerSpeed = initialPlayerSpeed;
	score = 0;
	playerPosition = glm::vec3(0.f);
	playerStep = 0.f;
	currentSpawnTime = 0.f;
	obstacles.clear();
	GenerateInitialObstacles(initialObstacles);
}

void SkiFree3D::FrameStart() {
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::ivec2 resolution = window->GetResolution();
	glViewport(0, 0, resolution.x, resolution.y);
}

void SkiFree3D::UpdateCamera()
{
	glm::ivec2 res = window->GetResolution();
	glm::vec3 cameraUpDir = glm::vec3(
			glm::rotate(glm::mat4(1.f),
				cameraAngle,
				glm::vec3(1.f, 0.f, 0.f))
			* glm::vec4(rotatedOY, 1.f));

	cameraPosition = playerPosition + cameraUpDistance * glm::normalize(cameraUpDir);
	glm::vec3 center = playerPosition + cameraPlayerDistance * rotatedOZ;

	viewMatrix = glm::lookAt(cameraPosition, center, glm::vec3(0.f, 0.f, -1.f));
}

void SkiFree3D::Update(float deltaTimeSeconds) {
	glm::mat4 modelMatrix = glm::mat4(1.f);

	CleanObstacles();
	UpdateCamera();

	for (int i = 0, size = obstacles.size(); i < size; i++)
		RenderObstacle(obstacles[i]);

	// Randeaza jucatorul
	modelMatrix = glm::mat4(1.f);
	modelMatrix = glm::translate(modelMatrix, playerPosition);
	modelMatrix = glm::rotate(modelMatrix, playerStep, rotatedOY);
	modelMatrix = glm::rotate(modelMatrix, snowStep, glm::vec3(1.f, 0.f, 0.f));
	RenderPlayer(modelMatrix);

	// Randeaza zapada
	modelMatrix = glm::mat4(1.f);
	modelMatrix = glm::translate(modelMatrix, playerPosition);
	modelMatrix = glm::rotate(modelMatrix, snowStep, glm::vec3(1.f, 0.f, 0.f));
	modelMatrix = glm::scale(modelMatrix, snowScaleVector);
	RenderMesh(meshes["plane"], modelMatrix, mapTextures["snow"], true);

	// Actualizeaza pozitia jucatorului
	playerPosition += playerSpeed * deltaTimeSeconds * GetPlayerDirection();

	// Se verifica daca jucatorul a lovit ceva
	int index = playerLost ? -1 : CheckPlayerCollision();
	if (index != -1) {
		if (obstacles[index].kind == OT_GIFT) {
			// Daca jucatorul a lovit un cadou atunci il scot din lista
			// de obstacole
			score++;
			obstacles.erase(obstacles.begin() + index);
		} else {
			// Daca jucatorul a lovit altceva atunci opresc jocul
			playerLost = true;
			playerSpeed = 0.f;
			cout << "You Lost!" << endl;
			cout << "Score: " << score << endl;
			cout << "Press R to reset." << endl;
		}	
	}

	// Se incrementeaza timpul de cand s-a creat ultimul obstacol
	currentSpawnTime += deltaTimeSeconds;
	if (currentSpawnTime >= spawnTime) {
		// Daca a trecut suficient timp de cand s-a creat ultimul
		// obstacol atunci generez altul
		GenerateObstacle();
		currentSpawnTime = 0;
	}
}

void SkiFree3D::FrameEnd() {

}

// Directia pe care este orientat jucatorul, vectorul apartine planului rotit
glm::vec3 SkiFree3D::GetPlayerDirection() 
{
	return glm::vec3(glm::rotate(glm::mat4(1.f), playerStep, rotatedOY) * glm::vec4(rotatedOZ, 1.f));
}

void SkiFree3D::OnInputUpdate(float deltaTime, int mods) {
	if (window->MouseHold(GLFW_MOUSE_BUTTON_RIGHT))
		return;
}

void SkiFree3D::OnKeyPress(int key, int mods) {
	if (playerLost && key == GLFW_KEY_R)
		Reset();
}

void SkiFree3D::OnKeyRelease(int key, int mods) {

}

void SkiFree3D::OnMouseMove(int mouseX, int mouseY, int deltaX, int deltaY) {
	glm::ivec2 res = window->GetResolution();

	// Se calculeaza la cat la suta din latimea maxima a ferestrei
	// se afla cursorul, iar apoi acest procentaj este translatat de la
	// de la procentul minim si cel maxim la 0% si 100%, astfel jucatorul
	// trebuie sa miste mouse-ul doar intr-o sectiune din latimea ferestrei
	float proc = (float)mouseX / (float)res.x;
	proc = (proc - minMouseProc) / (maxMouseProc - minMouseProc);
	playerStep = proc * (maxPlayerStep - minPlayerStep) + minPlayerStep;

	if (proc > 1.f)
		playerStep = maxPlayerStep;
	if (proc < 0.f)
		playerStep = minPlayerStep;
}

void SkiFree3D::OnMouseBtnPress(int mouseX, int mouseY, int button, int mods) {

}

void SkiFree3D::OnMouseBtnRelease(int mouseX, int mouseY, int button, int mods) {

}

void SkiFree3D::OnMouseScroll(int mouseX, int mouseY, int offsetX, int offsetY) {

}

void SkiFree3D::OnWindowResize(int width, int height) {

}
