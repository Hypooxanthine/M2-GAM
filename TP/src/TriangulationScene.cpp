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

TriangulationScene::TriangulationScene()
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
    gameLayer.getCustomEvent("MouseRight")
        .bindCallback([&, this](const vrm::Event& e) {
            onRightClick(e.mouseX, e.mouseY);
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

    /* Processing */
    //m_TriangularMesh.loadOFF("Resources/OffFiles/queen.off");
    //m_TriangularMeshAsset.addSubmesh(m_TriangularMesh.toMeshData());

    /* Visualization */
    auto supportEntity = createEntity("Support");
    auto& supportMesh = supportEntity.addComponent<vrm::MeshComponent>(vrm::AssetManager::Get().getAsset<vrm::MeshAsset>("Resources/Meshes/Support.obj"));
    auto& supportTransform = supportEntity.getComponent<vrm::TransformComponent>();
    supportTransform.setScale({ 10.f, 10.f, 10.f });
    supportMesh.setWireframe(true);

    auto triangularMeshEntity = createEntity("TriangularMesh");
    auto& triangularMeshMesh = triangularMeshEntity.addComponent<vrm::MeshComponent>(m_TriangularMeshAsset.createInstance());
    triangularMeshMesh.setWireframe(false);

    auto lightEntity = createEntity("Light");
    auto& c =  lightEntity.addComponent<vrm::PointLightComponent>();
    c.color = { 1.f, 1.f, 1.f };
    c.intensity = 1000000.f;
    c.radius = 2000.f;
    lightEntity.getComponent<vrm::TransformComponent>().setPosition({ -5.f, 1000.f , -5.f });
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
        ImGui::Checkbox("Enable controls", &m_ControlsEnabled);
        ImGui::TextWrapped("Camera speed");
        ImGui::SliderFloat("##Camera speed", &myCameraSpeed, 0.f, 100.f);
        ImGui::TextWrapped("Camera angular speed");
        ImGui::SliderFloat("##Camera angular speed", &myCameraAngularSpeed, 0.f, 0.1f);
    ImGui::End();

    ImGui::Begin("Tweaks");
    ImGui::End();

    ImGui::Begin("Stats");
        ImGui::TextWrapped("FPS: %.2f", ImGui::GetIO().Framerate);
    ImGui::End();
}

void TriangulationScene::onRightClick(int mouseX, int mouseY)
{
    const auto width = vrm::Application::Get().getGameLayer().getFrameBuffer().getSpecification().width;
    const auto height = vrm::Application::Get().getGameLayer().getFrameBuffer().getSpecification().height;

    Ray ray = Math::RayUnderCursor(m_Camera.getPosition(), m_Camera.getViewProjection(), { mouseX, mouseY }, { width, height });
    auto e = getEntity("Support");
    HitResult hit = Math::RayCastWithMesh(ray, e.getComponent<vrm::MeshComponent>(), e.getComponent<vrm::TransformComponent>().getTransform());

    if (hit.hasHit)
    {
        VRM_LOG_TRACE("Mouse position: ({}, {}) / ({}, {})", mouseX, mouseY, width, height);
        VRM_LOG_TRACE("Hit at position: {}", glm::to_string(hit.position));
        TriangularMesh::Vertex v;
        v.position = hit.position;
        m_TriangularMesh.addVertex(v);
        auto vertexCount = m_TriangularMesh.getVertexCount();
        if (vertexCount > 2)
        {
            m_TriangularMesh.addFace(vertexCount - 3, vertexCount - 2, vertexCount - 1);
            updateTriangularMesh();
        }
    }
}

void TriangulationScene::updateTriangularMesh()
{
    auto data = m_TriangularMesh.toMeshData();
    m_TriangularMeshAsset.clear();
    m_TriangularMeshAsset.addSubmesh(data);

    auto e = getEntity("TriangularMesh");
    e.getComponent<vrm::MeshComponent>().setMesh(m_TriangularMeshAsset.createInstance());
}