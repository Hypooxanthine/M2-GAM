#pragma once

#include <Vroom/Scene/Scene.h>
#include <Vroom/Render/Camera/FirstPersonCamera.h>
#include <Vroom/Asset/StaticAsset/MeshAsset.h>

#include <glm/gtc/constants.hpp>

#include <vector>

#include "imgui.h"
#include "TriangularMesh.h"

class TriangulationScene : public vrm::Scene
{
public:
	TriangulationScene();
	~TriangulationScene() = default;
 
protected:
	void onInit() override;

	void onEnd() override;

	void onUpdate(float dt) override;

	void onRender() override;

private:
	void onImGui();

	void onRightClick(int mouseX, int mouseY);
	void updateTriangularMesh();

private:
    vrm::FirstPersonCamera m_Camera;
    float forwardValue = 0.f, rightValue = 0.f, upValue = 0.f;
	float turnRightValue = 0.f, lookUpValue = 0.f;
	float myCameraSpeed = 10.f, myCameraAngularSpeed = .08f * glm::two_pi<float>() / 360.f;
    bool m_MouseLock = false;

	ImFont* m_Font = nullptr;

	bool m_ControlsEnabled = true;

	vrm::MeshAsset m_TriangularMeshAsset;
	TriangularMesh m_TriangularMesh;
};