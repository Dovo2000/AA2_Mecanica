#include <imgui\imgui.h>
#include <imgui\imgui_impl_sdl_gl3.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <cstdio>
#include <vector>

//Exemple
extern void Exemple_GUI();
extern void Exemple_PhysicsInit();
extern void Exemple_PhysicsUpdate(float dt);
extern void Exemple_PhysicsCleanup();

namespace LilSpheres {
	extern const int maxParticles;
	extern int firstParticleIdx;
	extern int particleCount;
	extern void updateParticles(int startIdx, int count, float* array_data);
}
namespace Sphere {
	float _radius = 1.f;
	glm::vec3 centerPos(0.f, 1.f, 0.f);
	extern void updateSphere(glm::vec3 pos, float radius = 1.f);
}
namespace Capsule {
	float _radius = 1.f;
	glm::vec3 posA(-3.f, 2.f, -2.f);
	glm::vec3 posB(-4.f, 2.f, 2.f);
	extern void updateCapsule(glm::vec3 posA, glm::vec3 posB, float radius = 1.f);
}
namespace Box {
	glm::vec3 normalUp(0, -1, 0);
	glm::vec3 normalDown(0, 1, 0);
	glm::vec3 normalRight(-1, 0, 0);
	glm::vec3 normalLeft(1, 0, 0);
	glm::vec3 normalBack(0, 0, 1);
	glm::vec3 normalFront(0, 0, -1);

	float dUp = -glm::dot(normalUp, glm::vec3(0, 10, 0));
	float dDown = -glm::dot(normalDown, glm::vec3(0, 0, 0));
	float dLeft = -glm::dot(normalLeft, glm::vec3(-5, 0, 0));
	float dRight = -glm::dot(normalRight, glm::vec3(5, 0, 0));
	float dBack = -glm::dot(normalBack, glm::vec3(0, 0, -5));
	float dFront = -glm::dot(normalFront, glm::vec3(0, 0, 5));
}

namespace {
	static struct PhysParams {
		float min = 0.f;
		float max = 10.f;
		float buttonTam = 3.f;
		float coneRad = 0.f;
		glm::vec3 dir, pos;
	} p_pars;

	static struct ParticleSystem {
		std::vector<glm::vec3> position;
		std::vector<glm::vec3> velocity;
		glm::vec3 acceleration;
		float lifeTime;
		std::vector<float> startTime;
		bool cascade = true, fountain = false;
		int emissionType = 0;
		int emisionRate;
		float accTime;
		int numParticles;
	} s_PS;
}

bool show_test_window = false;
extern bool renderSphere;
extern bool renderCapsule;
void GUI() {
	bool show = true;
	ImGui::Begin("Physics Parameters", &show, 0);

	{	
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);//FrameRate
		ImGui::SliderFloat("Min", &p_pars.min, 0.f, 5.f);
		ImGui::SliderFloat("Max", &p_pars.max, 6.f, 10.f);
		ImGui::SliderFloat("DirX", &p_pars.dir.x, -4.f, 4.f);
		ImGui::SliderFloat("DirY", &p_pars.dir.y, 1.f, 9.f);
		ImGui::SliderFloat("DirZ", &p_pars.dir.z, -4.f, 4.f);
		ImGui::SliderFloat("PosX", &p_pars.pos.x, -4.f, 4.f);
		ImGui::SliderFloat("PosY", &p_pars.pos.y, 1.f, 9.f);
		ImGui::SliderFloat("PosZ", &p_pars.pos.z, -4.f, 4.f);
		ImGui::SliderFloat("Cone", &p_pars.coneRad, 0.f, 2.f);
		ImGui::SliderInt("Emission Rate", &s_PS.emisionRate, 1, 30);
		ImGui::SliderInt("Emission Type", &s_PS.emissionType, 0, 1);
		ImGui::Spacing();
		ImGui::Checkbox("Render Sphere", &renderSphere);
		ImGui::SliderFloat("Sphere Pos X", &Sphere::centerPos.x, -5 , 5);
		ImGui::SliderFloat("Sphere Pos Y", &Sphere::centerPos.y, 0, 10);
		ImGui::SliderFloat("Sphere Pos Z", &Sphere::centerPos.z, -5, 5);
		ImGui::SliderFloat("Sphere Radius", &Sphere::_radius, 0.1f, 2.f);
		ImGui::Spacing();
		ImGui::Checkbox("Render Capsule", &renderCapsule);
		ImGui::SliderFloat("Capsule PosA X", &Capsule::posA.x, -5, 5);
		ImGui::SliderFloat("Capsule PosA Y", &Capsule::posA.y, -5, 5);
		ImGui::SliderFloat("Capsule PosA Z", &Capsule::posA.z, -5, 5);
		ImGui::SliderFloat("Capsule PosB X", &Capsule::posB.x, -5, 5);
		ImGui::SliderFloat("Capsule PosB Y", &Capsule::posB.y, -5, 5);
		ImGui::SliderFloat("Capsule PosB Z", &Capsule::posB.z, -5, 5);
		ImGui::SliderFloat("Radius", &Capsule::_radius, 0.1f, 3.f);
	}
	
	ImGui::End();
}

float spawnTime = 0.5f, currentTime = 0.0f;

void PhysicsInit() {
	//Exemple_PhysicsInit();
	p_pars.dir.x = p_pars.dir.y = p_pars.dir.z = 0.f;
	p_pars.pos.x = -4.5f;
	p_pars.pos.z = 0.f;
	p_pars.pos.y = 5.f;
	s_PS.numParticles = 0;
	s_PS.lifeTime = 5.f;
	s_PS.accTime = 0.0f;
	s_PS.emisionRate = 1;
	s_PS.acceleration = glm::vec3(0.0f, -9.81f, 0.0f);
	extern bool renderParticles; renderParticles = true;
	renderSphere = false;
	renderCapsule = true;
	LilSpheres::firstParticleIdx = 0;
	LilSpheres::particleCount = s_PS.numParticles;
}

void pointPlaneCollision(glm::vec3 norm, glm::vec3 &pos, glm::vec3 &vel)
{
	float D = -glm::dot(norm, pos);

	pos = pos - 2 * (dot(norm, pos) + D) * norm;
	vel = vel - 2.f * (dot(norm, vel)) * norm;
}
void pointPlaneCollision(glm::vec3 norm, glm::vec3 &pos, glm::vec3 &vel, float D)	//Por si ya tenemos el valor independiente del plano, como en el caso del cubo que se calcula antes
{
	pos = pos - 2 * (dot(norm, pos) + D) * norm;
	vel = vel - 2.f * (dot(norm, vel)) * norm;
}

void collisionBox(glm::vec3 &pos, glm::vec3 &vel, float dt)				//FALTA ELASTICIDAD
{
	float distanceUp = (dot(Box::normalUp, pos) + Box::dUp) / sqrt(pow(Box::normalUp.x, 2) + pow(Box::normalUp.y, 2) + pow(Box::normalUp.z, 2));
	//distanceUp = distanceUp < 0 ? distanceUp * -1 : distanceUp;
	float distanceDown = (dot(Box::normalDown, pos) + Box::dDown) / sqrt(pow(Box::normalDown.x, 2) + pow(Box::normalDown.y, 2) + pow(Box::normalDown.z, 2));
	//distanceDown = distanceDown < 0 ? distanceDown * -1 : distanceDown;
	float distanceLeft = (dot(Box::normalLeft, pos) + Box::dLeft) / sqrt(pow(Box::normalLeft.x, 2) + pow(Box::normalLeft.y, 2) + pow(Box::normalLeft.z, 2));
	//distanceLeft = distanceLeft < 0 ? distanceLeft * -1 : distanceLeft;
	float distanceRight = (dot(Box::normalRight, pos) + Box::dRight) / sqrt(pow(Box::normalRight.x, 2) + pow(Box::normalRight.y, 2) + pow(Box::normalRight.z, 2));
	//distanceRight = distanceRight < 0 ? distanceRight * -1 : distanceRight;
	float distanceBack = (dot(Box::normalBack, pos) + Box::dBack) / sqrt(pow(Box::normalBack.x, 2) + pow(Box::normalBack.y, 2) + pow(Box::normalBack.z, 2));
	//distanceBack = distanceBack < 0 ? distanceBack * -1 : distanceBack;
	float distanceFront = (dot(Box::normalFront, pos) + Box::dFront) / sqrt(pow(Box::normalFront.x, 2) + pow(Box::normalFront.y, 2) + pow(Box::normalFront.z, 2));
	//distanceFront = distanceFront < 0 ? distanceFront * -1 : distanceFront;

	if (distanceDown <= 0)
	{
		pointPlaneCollision(Box::normalDown, pos, vel, Box::dDown);
	}
	if (distanceUp <= 0)
	{
		pointPlaneCollision(Box::normalUp, pos, vel, Box::dUp);
	}
	if (distanceLeft <= 0)
	{
		pointPlaneCollision(Box::normalLeft, pos, vel, Box::dLeft);
	}
	if (distanceRight <= 0)
	{
		pointPlaneCollision(Box::normalRight, pos, vel, Box::dRight);
	}
	if (distanceFront <= 0)
	{
		pointPlaneCollision(Box::normalFront, pos, vel, Box::dFront);
	}
	if (distanceBack <= 0)
	{
		pointPlaneCollision(Box::normalBack, pos, vel, Box::dBack);
	}
}

void collisionSphere(glm::vec3 &pos, glm::vec3 &vel)
{
	if ((sqrt(pow(pos.x - Sphere::centerPos.x, 2) + pow(pos.y - Sphere::centerPos.y, 2) + pow(pos.z - Sphere::centerPos.z, 2)) - Sphere::_radius) < 0.f)
	{
		float a = pow(vel.x, 2) + pow(vel.y, 2) + pow(vel.z, 2);
		float b = (2 * (pos.x - Sphere::centerPos.x) * vel.x) + (2 * (pos.y - Sphere::centerPos.y) * vel.y) + (2 * (pos.z - Sphere::centerPos.z) * vel.z);
		float c = pow(pos.x - Sphere::centerPos.x, 2) + pow(pos.y - Sphere::centerPos.y, 2) + pow(pos.z - Sphere::centerPos.z, 2) - pow(Sphere::_radius, 2);
		float var1 = (-b + sqrt(pow(b, 2) - 4 * a * c)) / (2 * a);
		float var2 = (-b - sqrt(pow(b, 2) - 4 * a * c)) / (2 * a);
		glm::vec3 inter;

		inter = var1 < var2 ? pos + vel * var1 : pos + vel * var2;

		glm::vec3 sphereNormal = (inter - Sphere::centerPos) / Sphere::_radius;

		pointPlaneCollision(sphereNormal, pos, vel);
	}
}

void collisionCapsule(glm::vec3 &pos, glm::vec3 &vel)
{
	glm::vec3 closestPoint = Capsule::posA + 
							(glm::clamp(
								glm::dot(pos - Capsule::posA, Capsule::posB - Capsule::posA) / 
								pow(glm::sqrt(
									pow(Capsule::posB.x - Capsule::posA.x, 2) + 
									pow(Capsule::posB.y - Capsule::posA.y, 2) + 
									pow(Capsule::posB.z - Capsule::posA.z, 2)), 2),0.f,1.f)) * 
								(Capsule::posB - Capsule::posA);
	float distanceCilindre = sqrt(pow(pos.x - closestPoint.x, 2) + pow(pos.y - closestPoint.y, 2) + pow(pos.z - closestPoint.z, 2)) - Capsule::_radius;

	if (distanceCilindre <= 0)
	{
		glm::vec3 normVec = glm::normalize(pos - closestPoint);
		glm::vec3 point = closestPoint + normVec * Capsule::_radius;

		pointPlaneCollision(normVec, pos, vel);
	}
}

void PhysicsUpdate(float dt) {
	s_PS.accTime += dt;
	if ((s_PS.accTime * s_PS.emisionRate) >= 1)
	{
		s_PS.numParticles++;
		if (s_PS.emissionType == 0)
		{
			s_PS.position.push_back(p_pars.pos);
			s_PS.velocity.push_back(p_pars.dir + (float)rand() / (RAND_MAX / (p_pars.coneRad*2) - p_pars.coneRad));
			s_PS.startTime.push_back(ImGui::GetTime());
			s_PS.accTime = 0;
		}
		else if (s_PS.emissionType == 1){
			s_PS.position.push_back(glm::vec3(p_pars.pos.x + (float)rand() / (RAND_MAX / (p_pars.max - p_pars.min)), p_pars.pos.y, p_pars.pos.z));
			s_PS.velocity.push_back(p_pars.dir);
			s_PS.startTime.push_back(ImGui::GetTime());
			s_PS.accTime = 0;
		}
	}

	for (int i = s_PS.numParticles - 1; i >= LilSpheres::firstParticleIdx; i--)
	{
		s_PS.position[i] += s_PS.velocity[i] * dt;
		s_PS.velocity[i] += s_PS.acceleration * dt;
		collisionBox(s_PS.position[i], s_PS.velocity[i], dt);
		if(renderSphere) collisionSphere(s_PS.position[i], s_PS.velocity[i]);
		if (renderCapsule) collisionCapsule(s_PS.position[i], s_PS.velocity[i]);
		if (ImGui::GetTime() - s_PS.startTime[i] >= s_PS.lifeTime)
		{
			s_PS.position.erase(s_PS.position.begin() + i);
			s_PS.velocity.erase(s_PS.velocity.begin() + i);
			s_PS.startTime.erase(s_PS.startTime.begin() + i);
			s_PS.numParticles--;
		}
			
	}
	if(s_PS.numParticles > 0)
		LilSpheres::updateParticles(LilSpheres::firstParticleIdx, s_PS.numParticles, &s_PS.position[0].x);
	LilSpheres::particleCount = s_PS.numParticles;
	if (renderSphere) Sphere::updateSphere(Sphere::centerPos, Sphere::_radius);
	if (renderCapsule) Capsule::updateCapsule(Capsule::posA, Capsule::posB, Capsule::_radius);
	//glm::vec4 plano(Ax,By,Cz,D);
	//Planos: x + D = 0,-x + D = 0,y + D = 0,-y + D = 0,z + D = 0,-z + D = 0
}

void PhysicsCleanup() {
	Exemple_PhysicsCleanup();
}