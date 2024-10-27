#include "TriangulationScene.h"

#include <Vroom/Core/Application.h>
#include <Vroom/Core/GameLayer.h>
#include <Vroom/Core/Window.h>

#include <Vroom/Scene/Components/MeshComponent.h>
#include <Vroom/Scene/Components/TransformComponent.h>
#include <Vroom/Scene/Components/PointLightComponent.h>

#include <Vroom/Asset/AssetManager.h>
#include <Vroom/Asset/StaticAsset/MaterialAsset.h>

#include <glm/gtx/string_cast.hpp>

#include "imgui.h"

#include "RayCasting.h"

#include "ScopeProfiler.h"

TriangulationScene::TriangulationScene()
    : vrm::Scene(), m_Camera(0.1f, 100'000.f, glm::radians(90.f), 600.f / 400.f, { 0.f, 20.f, 0.f }, { glm::radians(90.f), 0.f, 0.f })
{
    auto& gameLayer = vrm::Application::Get().getGameLayer();

    // Bind triggers to the camera
    gameLayer.getTrigger("MoveForward")
        .bindCallback([this](bool triggered) { forwardValue += triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveBackward")
        .bindCallback([this](bool triggered) { forwardValue -= triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveRight")
        .bindCallback([this](bool triggered) { rightValue += triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveLeft")
        .bindCallback([this](bool triggered) { rightValue -= triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveUp")
        .bindCallback([this](bool triggered) { upValue += triggered ? 1.f : -1.f; });
    gameLayer.getTrigger("MoveDown")
        .bindCallback([this](bool triggered) { upValue -= triggered ? 1.f : -1.f; });
    
    gameLayer.getTrigger("MouseLeft")
        .bindCallback([this](bool triggered) {
            if (!m_ControlsEnabled)
                return;
            m_MouseLock = triggered;
            vrm::Application::Get().getWindow().setCursorVisible(!triggered);
        });
    gameLayer.getCustomEvent("MouseRight")
        .bindCallback([&, this](const vrm::Event& e) {
            onRightClick(static_cast<int>(e.mouseX), static_cast<int>(e.mouseY));
        });
    
    gameLayer.getCustomEvent("MouseMoved")
        .bindCallback([this](const vrm::Event& event) {
            turnRightValue += static_cast<float>(event.mouseDeltaX);
            lookUpValue -= static_cast<float>(event.mouseDeltaY);
        });
}

void TriangulationScene::onInit()
{
    /* ImGui */

    ImGuiIO& io = ImGui::GetIO();
    m_Font = io.Fonts->AddFontFromFileTTF("Resources/Fonts/Roboto/Roboto-Regular.ttf", 24.0f);
    VRM_ASSERT_MSG(m_Font, "Failed to load font.");

    setCamera(&m_Camera);

    /* Visualization */

    auto triangularMeshEntity = createEntity("TriangularMesh");
    auto& triangularMeshMesh = triangularMeshEntity.addComponent<vrm::MeshComponent>(m_TriangularMeshAsset.createInstance());
    triangularMeshMesh.setWireframe(m_WireFrame);

    auto lightEntity = createEntity("Light");
    auto& c =  lightEntity.addComponent<vrm::PointLightComponent>();
    c.color = glm::vec3(1.f);
    c.radius = 100'000.f;
    c.intensity = std::powf(c.radius, 1.5f);
    lightEntity.getComponent<vrm::TransformComponent>().setPosition({ -5.f, 10'000.f , -5.f });

    resetTriangularMesh();
}

void TriangulationScene::onEnd()
{
}

void TriangulationScene::onUpdate(float dt)
{
    /* Camera */
    if (m_ControlsEnabled && m_MouseLock)
    {
        m_Camera.move(forwardValue * myCameraSpeed * dt * m_Camera.getForwardVector());
        m_Camera.move(rightValue * myCameraSpeed * dt * m_Camera.getRightVector());
        m_Camera.move(upValue * myCameraSpeed * dt * glm::vec3{0.f, 1.f, 0.f});
        m_Camera.addYaw(turnRightValue * myCameraAngularSpeed);
        m_Camera.addPitch(lookUpValue * myCameraAngularSpeed);
    }

    lookUpValue = 0.f;
    turnRightValue = 0.f;
}

void TriangulationScene::onRender()
{
    ImGui::PushFont(m_Font);

    onImGui();

    ImGui::PopFont();
}

void TriangulationScene::onImGui()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::Begin("Controls");
        ImGui::TextWrapped("While left clicking:");
        ImGui::TextWrapped("WASD to move the camera");
        ImGui::TextWrapped("Space to move up, Left Shift to move down");
        ImGui::TextWrapped("Right click to interact with the scene.");
        ImGui::Checkbox("Enable controls", &m_ControlsEnabled);
        ImGui::TextWrapped("Camera speed");
        ImGui::SliderFloat("##Camera speed", &myCameraSpeed, 0.f, 10000.f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::TextWrapped("Camera angular speed");
        ImGui::SliderFloat("##Camera angular speed", &myCameraAngularSpeed, 0.f, 0.1f);
    ImGui::End();

    ImGui::Begin("Tweaks");
        if (ImGui::Checkbox("Wireframe", &m_WireFrame))
            getEntity("TriangularMesh").getComponent<vrm::MeshComponent>().setWireframe(m_WireFrame);

        ImGui::Checkbox("Check mesh integrity when updating", &m_IntegrityTestWhenUpdating);

        if (ImGui::BeginCombo("Triangulation mode", m_TriangulationModeLabel.c_str()))
        {
            if (ImGui::Selectable("Naive") && m_TriangulationMode != TriangulationMode::NAIVE)
            {
                m_TriangulationModeLabel = "Naive";
                m_TriangulationMode = TriangulationMode::NAIVE;
                resetTriangularMesh();

                m_Camera.setNear(0.1f);
                m_Camera.setFar(100.f);
                m_Camera.setWorldPosition({ 0.f, 20.f, 0.f });
                m_Camera.setRotation({ glm::radians(90.f), 0.f, 0.f });
                m_ControlsEnabled = false;
                myCameraSpeed = 10.f;
            }
            if (ImGui::Selectable("Continuous Delaunay") && m_TriangulationMode != TriangulationMode::CONTINUOUS_DELAUNAY)
            {
                m_TriangulationModeLabel = "Continuous Delaunay";
                m_TriangulationMode = TriangulationMode::CONTINUOUS_DELAUNAY;
                resetTriangularMesh();

                m_Camera.setNear(0.1f);
                m_Camera.setFar(100.f);
                m_Camera.setWorldPosition({ 0.f, 20.f, 0.f });
                m_Camera.setRotation({ glm::radians(90.f), 0.f, 0.f });
                m_ControlsEnabled = false;
                myCameraSpeed = 10.f;
            }
            if (ImGui::Selectable("Terrain") && m_TriangulationMode != TriangulationMode::TERRAIN)
            {
                m_TriangulationModeLabel = "Terrain";
                m_TriangulationMode = TriangulationMode::TERRAIN;
                resetTriangularMesh();

                m_Camera.setNear(1'000.f);
                m_Camera.setFar(20'000.f);
                m_Camera.setWorldPosition({ 0.f, 10'000.f, 0.f });
                m_Camera.setRotation({ glm::radians(90.f), 0.f, 0.f });
                m_ControlsEnabled = true;
                myCameraSpeed = 5'000.f;
            }

            ImGui::EndCombo();
        }

        if (m_TriangulationMode == TriangulationMode::NAIVE)
        {
            if (ImGui::BeginCombo("Edit mode", m_EditModeLabel.c_str()))
            {
                if (ImGui::Selectable("Place vertices") && m_EditMode != EditMode::PLACE_VERTICES)
                {
                    m_EditModeLabel = "Place vertices";
                    m_EditMode = EditMode::PLACE_VERTICES;
                }
                if (ImGui::Selectable("Flip edge") && m_EditMode != EditMode::FLIP_EDGE)
                {
                    m_EditModeLabel = "Flip edge";
                    m_EditMode = EditMode::FLIP_EDGE;
                }

                ImGui::EndCombo();
            }
        }

        // In naive mode, we let the possibility of transforming the triangulation into a Delaunay
        // triangulation. We do not do it in continuous Delaunay because the mesh
        // is suppose to stay Delaunay while adding vertices.
        if (m_TriangulationMode == TriangulationMode::NAIVE && ImGui::Button("Delaunay algorithm"))
        {
            {
                PROFILE_SCOPE_VARIABLE(m_LastProcessTime);
                m_LastFlipsCount = m_TriangularMesh.delaunayAlgorithm();
            }
            updateTriangularMesh();
        }

        if (m_TriangulationMode == TriangulationMode::TERRAIN)
        {
            if (ImGui::BeginCombo("Terrain data", m_TerrainDataLabel.c_str()))
            {
                for (const auto& file : std::filesystem::directory_iterator(std::filesystem::current_path() / "Resources" / "TerrainData"))
                {
                    if (file.is_directory())
                        continue;

                    std::string fileName = file.path().filename().string();
                    if (ImGui::Selectable(fileName.c_str()))
                    {
                        m_TerrainDataLabel = fileName;
                        m_TerrainDataPath = file.path();
                    }
                }

                ImGui::EndCombo();
            }

            if (!m_TerrainDataLabel.empty())
            {
                if (ImGui::Button("Naive triangulation"))
                {
                    naiveTriangulation(m_TerrainDataPath);
                }

                if (ImGui::Button("Delaunay triangulation"))
                {
                    delaunayTriangulation(m_TerrainDataPath);
                }
            }
        }

        if (ImGui::Button("Integrity test"))
            m_TriangularMesh.integrityTest();

        if (ImGui::Button("Reset"))
            resetTriangularMesh();
    ImGui::End();

    ImGui::Begin("Stats");
        ImGui::TextWrapped("FPS: %.2f", ImGui::GetIO().Framerate);
        if (m_LastProcessTime >= 0.f)
            ImGui::TextWrapped("Last process time: %.6f s", m_LastProcessTime);
        if (m_LastFlipsCount >= 0)
            ImGui::TextWrapped("Last flips count: %d", m_LastFlipsCount);
        ImGui::TextWrapped("Camera position: %s", glm::to_string(m_Camera.getPosition()).c_str());
        ImGui::TextWrapped("Camera rotation: %s", glm::to_string(m_Camera.getRotation()).c_str());
    ImGui::End();
}

void TriangulationScene::onRightClick(int mouseX, int mouseY)
{
    const auto width = vrm::Application::Get().getGameLayer().getFrameBuffer().getSpecification().width;
    const auto height = vrm::Application::Get().getGameLayer().getFrameBuffer().getSpecification().height;

    const Ray ray = Math::RayUnderCursor(m_Camera.getPosition(), m_Camera.getView(), m_Camera.getProjection(), { mouseX, mouseY }, { width, height });
    const HitResult hit = Math::RayCastWithPlane(ray, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 0.f });

    if (!hit.hasHit) return;

    if (m_TriangulationMode == TriangulationMode::NAIVE)
    {
        if (m_EditMode == EditMode::PLACE_VERTICES)
        {
            VRM_LOG_INFO("Placing vertex at {}", glm::to_string(hit.position));
            {
                PROFILE_SCOPE_VARIABLE(m_LastProcessTime);
                m_TriangularMesh.addVertex_StreamingTriangulation(hit.position);
            }
            updateTriangularMesh();
        }
        else if (m_EditMode == EditMode::FLIP_EDGE)
        {
            {
                PROFILE_SCOPE_VARIABLE(m_LastProcessTime);
                m_TriangularMesh.edgeFlip(glm::vec3(hit.position.x, 0.f, hit.position.z));
            }
            updateTriangularMesh();
        }
    }
    else if (m_TriangulationMode == TriangulationMode::CONTINUOUS_DELAUNAY)
    {
        VRM_LOG_INFO("Placing vertex at {}", glm::to_string(hit.position));
        {
            PROFILE_SCOPE_VARIABLE(m_LastProcessTime);
            m_LastFlipsCount = m_TriangularMesh.addVertex_StreamingDelaunayTriangulation(hit.position);
        }
        updateTriangularMesh();
    }

}

void TriangulationScene::naiveTriangulation(const std::filesystem::path& data)
{
    std::ifstream ifs(data);
    VRM_ASSERT_MSG(ifs.is_open(), "Couldn't open file {}.", data.string());

    {
        PROFILE_SCOPE_VARIABLE(m_LastProcessTime);
        auto vertexCount = setupTriangulation(ifs);

        std::string line;
        while (std::getline(ifs, line))
        {
            std::stringstream sstream(line);
            glm::vec3 v;
            std::string token;

            sstream >> token;
            v.x = std::stof(token);
            sstream >> token;
            v.z = std::stof(token);
            v.y = 0.f;

            m_TriangularMesh.addVertex_StreamingTriangulation(v);
        }
    }

    fixHeights(ifs);

    updateTriangularMesh();
}

void TriangulationScene::delaunayTriangulation(const std::filesystem::path& data)
{
    std::ifstream ifs(data);
    VRM_ASSERT_MSG(ifs.is_open(), "Couldn't open file {}.", data.string());

    {
        PROFILE_SCOPE_VARIABLE(m_LastProcessTime);
        auto vertexCount = setupTriangulation(ifs);

        std::string line;
        while (std::getline(ifs, line))
        {
            std::stringstream sstream(line);
            glm::vec3 v;
            std::string token;

            sstream >> token;
            v.x = std::stof(token);
            sstream >> token;
            v.z = std::stof(token);
            v.y = 0.f;

            m_TriangularMesh.addVertex_StreamingDelaunayTriangulation(v);
        }
    }

    fixHeights(ifs);

    updateTriangularMesh();
}

unsigned long long TriangulationScene::setupTriangulation(std::ifstream& ifs)
{
    m_TriangularMesh.clear();

    std::string line;

    std::getline(ifs, line);
    auto vertexCount = std::stoull(line);

    std::array<size_t, 3> vertexIndices;
    std::array<glm::vec2, 3> vertices;
    TriangularMesh::Vertex v;

    for (uint8_t i = 0; i < 3; ++i)
    {
        std::getline(ifs, line);
        std::stringstream sstream(line);

        std::string token;

        sstream >> token;
        v.position.x = std::stof(token);
        sstream >> token;
        v.position.z = std::stof(token);
        v.position.y = 0.f;

        vertices[i] = glm::vec2(v.position.x, -v.position.z);
        vertexIndices[i] = m_TriangularMesh.addVertex(v);
    }

    if (glm::determinant(glm::mat2(vertices[1] - vertices[0], vertices[2] - vertices[0])) > 0.f)
        m_TriangularMesh.addFirstFaceForTriangulation(vertexIndices[0], vertexIndices[1], vertexIndices[2]);
    else
        m_TriangularMesh.addFirstFaceForTriangulation(vertexIndices[0], vertexIndices[2], vertexIndices[1]);

    return vertexCount;
}

void TriangulationScene::fixHeights(std::ifstream& ifs)
{
    ifs.clear();
    ifs.seekg(0);

    std::string line;
    std::getline(ifs, line);

    for (size_t i = 0; std::getline(ifs, line); ++i)
    {
        std::stringstream sstream(line);
        std::string token;

        sstream >> token;
        sstream >> token;
        sstream >> token;

        size_t index = i;
        if (i > 2) index = index + 1; // Because of infinite vertex

        m_TriangularMesh.getVertex(index).position.y = std::stof(token);
    }
}

void TriangulationScene::resetTriangularMesh()
{
    m_TriangularMesh.clear();
    size_t v0 = m_TriangularMesh.addVertex({ { -5.f, 0.f, 0.f } });
    size_t v1 = m_TriangularMesh.addVertex({ {  0.f, 0.f, 5.f } });
    size_t v2 = m_TriangularMesh.addVertex({ {  5.f, 0.f, 0.f } });
    m_TriangularMesh.addFirstFaceForTriangulation(v0, v1, v2);
    m_LastFlipsCount = -1;
    m_LastProcessTime = -1.f;
    updateTriangularMesh();
}

void TriangulationScene::updateTriangularMesh()
{
    if (m_IntegrityTestWhenUpdating)
        m_TriangularMesh.integrityTest();

    auto data = m_TriangularMesh.toMeshData();
    m_TriangularMeshAsset.clear();
    m_TriangularMeshAsset.addSubmesh(data);

    auto e = getEntity("TriangularMesh");
    e.getComponent<vrm::MeshComponent>().setMesh(m_TriangularMeshAsset.createInstance());
}