#pragma once

#include <Vroom/Scene/Scene.h>
#include <Vroom/Render/Camera/FirstPersonCamera.h>
#include <Vroom/Asset/StaticAsset/MeshAsset.h>

#include <glm/gtc/constants.hpp>

#include <vector>

#include "imgui.h"

#include "TriangularMesh.h"

class MyScene : public vrm::Scene
{
public:
	MyScene();
	~MyScene() = default;
 
protected:
	void onInit() override;

	void onEnd() override;

	void onUpdate(float dt) override;

	void onRender() override;

private:
	void onImGui();

	void showFlat();
	void showLaplacianSmooth();
	void showCurvature();
	void showHeatDiffusion();

	void updateHeatDiffusion(float dt);

private:
    vrm::FirstPersonCamera m_Camera;
    float forwardValue = 0.f, rightValue = 0.f, upValue = 0.f;
	float turnRightValue = 0.f, lookUpValue = 0.f;
	float myCameraSpeed = 10.f, myCameraAngularSpeed = .08f * glm::two_pi<float>() / 360.f;
    bool m_MouseLock = false;

	ImFont* m_Font = nullptr;

	bool m_ControlsEnabled = true;

	vrm::MeshAsset m_MeshAsset;
	TriangularMesh m_TriangularMesh;
	vrm::MeshData m_SmoothMeshData;

	std::string m_ViewMode = "Flat";
	float m_LastComputeTimeSeconds = 0.f;

	bool m_SimulationStarted = false;
	int m_TriangleHeatSource = 10;
	float m_HeatSourceValue = 10.f;
	int m_IterationsPerFrame = 1;
};