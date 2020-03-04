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
		ImGui::SliderInt("Emission Rate", &s_PS.emisionRate, 1, 100);
		ImGui::SliderInt("Emission Type", &s_PS.emissionType, 0, 1);
		//ImGui::Button("Fountain");
	}
	
	ImGui::End();
}

float spawnTime = 0.5f, currentTime = 0.0f;

void PhysicsInit() {
	//Exemple_PhysicsInit();
	p_pars.dir.x = p_pars.dir.y = p_pars.dir.z = 1.f;
	p_pars.pos.x = p_pars.pos.z = 0.f;
	p_pars.pos.y = 5.f;
	s_PS.numParticles = 0;
	s_PS.lifeTime = 5.f;
	s_PS.accTime = 0.0f;
	s_PS.emisionRate = 1;
	s_PS.acceleration = glm::vec3(0.0f, -9.81f, 0.0f);
	extern bool renderParticles; renderParticles = true;
	LilSpheres::firstParticleIdx = 0;
	LilSpheres::particleCount = s_PS.numParticles;
}


void collisionVox(glm::vec3 &pos, glm::vec3 &vel, float dt)				//FALTA ELASTICIDAD
{
	if (pos.x <= -5.f || pos.x >= 5.f)
	{
		vel.x = -vel.x;
		pos.x += vel.x * dt;
	}
	else if (pos.y <= 0.f || pos.y >= 10.f)
	{
		vel.y = -vel.y;
		pos.y += vel.y * dt;
	}
	else if (pos.z <= -5.f || pos.z >= 5.f)
	{
		vel.z = -vel.z;
		pos.z += vel.z * dt;
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
		collisionVox(s_PS.position[i], s_PS.velocity[i], dt);
		
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
	//glm::vec4 plano(Ax,By,Cz,D);
	//Planos: x + D = 0,-x + D = 0,y + D = 0,-y + D = 0,z + D = 0,-z + D = 0
}
/*				s_PS.position[i] = glm::vec3(0.f, 7.f, 3.f);
				s_PS.velocity[i] = glm::vec3((float)rand() / (RAND_MAX / (p_pars.max - p_pars.min)), (float)rand() / (RAND_MAX / (p_pars.max - p_pars.min)), (float)rand() / (RAND_MAX / (p_pars.max - p_pars.min)));
				s_PS.startTime[i] = ImGui::GetTime();
				*/

void PhysicsCleanup() {
	Exemple_PhysicsCleanup();
}