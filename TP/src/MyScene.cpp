#include "MyScene.h"

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

MyScene::MyScene()
    : vrm::Scene(), m_Camera(0.1f, 100.f, glm::radians(90.f), 600.f / 400.f, { 0.5f, 10.f, 20.f }, { glm::radians(45.f), 0.f, 0.f })
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
    
    gameLayer.getCustomEvent("MouseMoved")
        .bindCallback([this](const vrm::Event& event) {
            turnRightValue += static_cast<float>(event.mouseDeltaX);
            lookUpValue -= static_cast<float>(event.mouseDeltaY);
        });
}

void MyScene::onInit()
{
    /* ImGui */

    ImGuiIO& io = ImGui::GetIO();
    m_Font = io.Fonts->AddFontFromFileTTF("Resources/Fonts/Roboto/Roboto-Regular.ttf", 24.0f);
    VRM_ASSERT_MSG(m_Font, "Failed to load font.");

    setCamera(&m_Camera);

    /* Processing */

    m_TriangularMesh.loadOFF("Resources/OffFiles/queen.off");

    /* Visualization */

    auto lightEntity = createEntity("Light");
    auto& c =  lightEntity.addComponent<vrm::PointLightComponent>();
    c.color = { 1.f, 1.f, 1.f };
    c.intensity = 1000000.f;
    c.radius = 2000.f;
    lightEntity.getComponent<vrm::TransformComponent>().setPosition({ -5.f, 1000.f , -5.f });
    
    showFlat();
}

void MyScene::onEnd()
{
}

void MyScene::onUpdate(float dt)
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

    /* Heat diffusion simulation */

    if (m_ViewMode == "Heat diffusion" && m_SimulationStarted)
        updateHeatDiffusion(dt);

}

void MyScene::onRender()
{
    ImGui::PushFont(m_Font);

    onImGui();

    ImGui::PopFont();
}

void MyScene::onImGui()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::Begin("Controls");
        ImGui::TextWrapped("While left clicking:");
        ImGui::TextWrapped("WASD to move the camera");
        ImGui::TextWrapped("Space to move up, Left Shift to move down");
        ImGui::Checkbox("Enable controls", &m_ControlsEnabled);
        ImGui::TextWrapped("Camera speed");
        ImGui::SliderFloat("##Camera speed", &myCameraSpeed, 0.f, 100.f);
        ImGui::TextWrapped("Camera angular speed");
        ImGui::SliderFloat("##Camera angular speed", &myCameraAngularSpeed, 0.f, 0.1f);
    ImGui::End();

    ImGui::Begin("Tweaks");
        if (ImGui::BeginCombo("View mode", m_ViewMode.c_str()))
        {
            if (ImGui::Selectable("Flat"))
            {
                m_ViewMode = "Flat";
                showFlat();
            }

            if (ImGui::Selectable("Laplacian smooth"))
            {
                m_ViewMode = "Laplacian smooth";
                showLaplacianSmooth();
            }
            
            if (ImGui::Selectable("Mean curvature"))
            {
                m_ViewMode = "Mean curvature";
                showCurvature();
            }
            
            if (ImGui::Selectable("Heat diffusion"))
            {
                m_ViewMode = "Heat diffusion";
            }

            ImGui::EndCombo();
        }
        if (m_ViewMode == "Heat diffusion")
        {
            ImGui::TextWrapped("Heat source triangle");
            ImGui::SliderInt("##Heat source triangle", &m_TriangleHeatSource, 0, static_cast<int>(m_TriangularMesh.getVertexCount() - 1));
            ImGui::TextWrapped("Heat source value");
            ImGui::SliderFloat("##Heat source value", &m_HeatSourceValue, 0.f, 100.f, "%.0f", ImGuiSliderFlags_Logarithmic);
            ImGui::TextWrapped("Iterations per frame");
            ImGui::SliderInt("##Iterations per frame", &m_IterationsPerFrame, 1, 50);
            
            if (m_SimulationStarted)
            {
                if (ImGui::Button("Stop simulation"))
                    m_SimulationStarted = false;
            }
            else
            {
                if (ImGui::Button("Start simulation"))
                    showHeatDiffusion();
            }
        }
    ImGui::End();

    ImGui::Begin("Stats");
        ImGui::TextWrapped("FPS: %.2f", ImGui::GetIO().Framerate);
        ImGui::TextWrapped("Last compute time: %.6f s", m_LastComputeTimeSeconds);
        if (!m_MeshAsset.getSubMeshes().empty())
        {
            ImGui::TextWrapped("Rendered vertices: %lu", m_MeshAsset.getSubMeshes().front().meshData.getVertexCount());
            ImGui::TextWrapped("Rendered triangles: %lu", m_MeshAsset.getSubMeshes().front().meshData.getTriangleCount());
        }
    ImGui::End();
}

void MyScene::showFlat()
{
    if (entityExists("Mesh"))
        destroyEntity(getEntity("Mesh"));

    m_MeshAsset.clear();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    m_MeshAsset.addSubmesh(m_TriangularMesh.toMeshData());
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    m_LastComputeTimeSeconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.f;

    auto entity = createEntity("Mesh");
    auto& meshComponent = entity.addComponent<vrm::MeshComponent>(m_MeshAsset.createInstance());
    meshComponent.setMaterial(0, vrm::AssetManager::Get().getAsset<vrm::MaterialAsset>("Resources/Engine/Material/Mat_Default.asset"));
    
    entity.getComponent<vrm::TransformComponent>().setPosition({ 0.f, 0.f, 0.f });
    entity.getComponent<vrm::TransformComponent>().setScale({ 10.f, 10.f, 10.f });
}

void MyScene::showLaplacianSmooth()
{
    if (entityExists("Mesh"))
        destroyEntity(getEntity("Mesh"));

    m_MeshAsset.clear();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    m_MeshAsset.addSubmesh(m_TriangularMesh.toSmoothMeshData());
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    m_LastComputeTimeSeconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.f;

    auto entity = createEntity("Mesh");
    auto& meshComponent = entity.addComponent<vrm::MeshComponent>(m_MeshAsset.createInstance());
    meshComponent.setMaterial(0, vrm::AssetManager::Get().getAsset<vrm::MaterialAsset>("Resources/Engine/Material/Mat_Default.asset"));
    
    entity.getComponent<vrm::TransformComponent>().setPosition({ 0.f, 0.f, 0.f });
    entity.getComponent<vrm::TransformComponent>().setScale({ 10.f, 10.f, 10.f });
}

void MyScene::showCurvature()
{
    if (entityExists("Mesh"))
        destroyEntity(getEntity("Mesh"));

    m_MeshAsset.clear();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    m_MeshAsset.addSubmesh(m_TriangularMesh.toSmoothMeshData());
    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    m_LastComputeTimeSeconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1'000'000.f;

    auto entity = createEntity("Mesh");
    auto& meshComponent = entity.addComponent<vrm::MeshComponent>(m_MeshAsset.createInstance());
    meshComponent.setMaterial(0, vrm::AssetManager::Get().getAsset<vrm::MaterialAsset>("Resources/Material/Mat_Curvature.asset"));
    
    entity.getComponent<vrm::TransformComponent>().setPosition({ 0.f, 0.f, 0.f });
    entity.getComponent<vrm::TransformComponent>().setScale({ 10.f, 10.f, 10.f });
}

void MyScene::showHeatDiffusion()
{
    m_SmoothMeshData = m_TriangularMesh.toSmoothMeshData();

    for (glm::length_t i = 0; i < 3; i++)
    {
        auto vertexIndex = m_TriangularMesh.getFace(static_cast<size_t>(m_TriangleHeatSource)).indices[i];
        m_SmoothMeshData.getVertices().at(vertexIndex).scalar = m_HeatSourceValue;
    }

    m_SimulationStarted = true;
}

void MyScene::updateHeatDiffusion(float dt)
{
    // Updating heat values

    auto heatFunction = [this](size_t vertexIndex) -> float {
        return m_SmoothMeshData.getVertices().at(vertexIndex).scalar;
    };

    for (size_t i = 0; i < static_cast<size_t>(m_IterationsPerFrame); i++)
    {
        m_SmoothMeshData = std::move(m_TriangularMesh.toHeatMeshData(heatFunction, 0.0000001f));

        for (glm::length_t i = 0; i < 3; i++)
        {
            auto vertexIndex = m_TriangularMesh.getFace(static_cast<size_t>(m_TriangleHeatSource)).indices[i];
            m_SmoothMeshData.getVertices().at(vertexIndex).scalar = m_HeatSourceValue;
        }
    }
    
    // Updating the mesh
    if (entityExists("Mesh"))
        destroyEntity(getEntity("Mesh"));

    m_MeshAsset.clear();
    m_MeshAsset.addSubmesh(m_SmoothMeshData, vrm::AssetManager::Get().getAsset<vrm::MaterialAsset>("Resources/Material/Mat_Heat.asset"));

    auto entity = createEntity("Mesh");
    auto& meshComponent = entity.addComponent<vrm::MeshComponent>(m_MeshAsset.createInstance());

    entity.getComponent<vrm::TransformComponent>().setPosition({ 0.f, 0.f, 0.f });
    entity.getComponent<vrm::TransformComponent>().setScale({ 10.f, 10.f, 10.f });
}