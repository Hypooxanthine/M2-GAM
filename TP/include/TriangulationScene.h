#pragma once

#include <Vroom/Scene/Scene.h>
#include <Vroom/Render/Camera/FirstPersonCamera.h>
#include <Vroom/Asset/StaticAsset/MeshAsset.h>

#include <glm/gtc/constants.hpp>

#include <vector>
#include <filesystem>

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

	void naiveTriangulation(const std::filesystem::path& data);
	void delaunayTriangulation(const std::filesystem::path& data);

	unsigned long long setupTriangulation(std::ifstream& ifs);
	void fixHeights(std::ifstream& ifs);

	void resetTriangularMesh();
	void updateTriangularMesh();

private:
    vrm::FirstPersonCamera m_Camera;
    float forwardValue = 0.f, rightValue = 0.f, upValue = 0.f;
	float turnRightValue = 0.f, lookUpValue = 0.f;
	float myCameraSpeed = 10.f, myCameraAngularSpeed = .08f * glm::two_pi<float>() / 360.f;
    bool m_MouseLock = false;

	ImFont* m_Font = nullptr;

	bool m_ControlsEnabled = false;

	vrm::MeshAsset m_TriangularMeshAsset;
	TriangularMesh m_TriangularMesh;
	bool m_WireFrame = true;
	bool m_IntegrityTestWhenUpdating = true;

	std::string m_TriangulationModeLabel = "Continuous Delaunay";
	enum class TriangulationMode
	{
		NAIVE = 0,
		CONTINUOUS_DELAUNAY,
		TERRAIN
	};

	TriangulationMode m_TriangulationMode = TriangulationMode::CONTINUOUS_DELAUNAY;

	std::string m_EditModeLabel = "Place vertices";
	enum class EditMode
	{
		PLACE_VERTICES = 0,
		FLIP_EDGE,
	};
	
	EditMode m_EditMode = EditMode::PLACE_VERTICES;

	std::string m_TerrainDataLabel = "";
	std::filesystem::path m_TerrainDataPath;

	float m_LastProcessTime = -1.f;
	int m_LastFlipsCount = -1;
};