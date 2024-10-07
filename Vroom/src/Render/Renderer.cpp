#include "Vroom/Render/Renderer.h"

#include <array>
#include <glm/gtc/matrix_transform.hpp>

#include "Vroom/Core/Application.h"

#include "Vroom/Render/Abstraction/GLCall.h"
#include "Vroom/Render/Abstraction/VertexArray.h"
#include "Vroom/Render/Abstraction/VertexBuffer.h"
#include "Vroom/Render/Abstraction/VertexBufferLayout.h"
#include "Vroom/Render/Abstraction/IndexBuffer.h"
#include "Vroom/Render/Abstraction/Shader.h"
#include "Vroom/Render/Abstraction/FrameBuffer.h"

#include "Vroom/Render/RawShaderData/SSBOPointLightData.h"

#include "Vroom/Asset/AssetManager.h"
#include "Vroom/Asset/StaticAsset/ShaderAsset.h"
#include "Vroom/Asset/StaticAsset/MaterialAsset.h"
#include "Vroom/Asset/StaticAsset/TextureAsset.h"
#include "Vroom/Asset/StaticAsset/MeshAsset.h"

#include "Vroom/Scene/Components/PointLightComponent.h"
#include "Vroom/Scene/Components/MeshComponent.h"

#include "Vroom/Scene/Scene.h"

static float SCREEN_QUAD_VERTICES[] = {
    -1.f, -1.f, 0.f, 0.f,
    1.f, -1.f, 1.f, 0.f,
    1.f, 1.f, 1.f, 1.f,
    -1.f, 1.f, 0.f, 1.f
};

static unsigned int SCREEN_QUAD_INDICES[] = {
    0, 1, 2,
    2, 3, 0
};

namespace vrm
{

std::unique_ptr<Renderer> Renderer::s_Instance = nullptr;

Renderer::Renderer()
    : m_ScreenQuadVBO(SCREEN_QUAD_VERTICES, 16 * sizeof(float)), m_ScreenQuadIBO(SCREEN_QUAD_INDICES, 6)
{
    // Initializing frame buffering data.
    m_ScreenShader = AssetManager::Get().getAsset<ShaderAsset>("Resources/Engine/Shader/ScreenShader/RenderShader_Screen.asset");
    m_ScreenQuadLayout.pushFloat(2);
    m_ScreenQuadLayout.pushFloat(2);
    m_ScreenQuadVAO.addBuffer(m_ScreenQuadVBO, m_ScreenQuadLayout);

    m_LightRegistry.setBindingPoint(0);
    m_ClusteredLights.setBindingPoint(1);

    //GLCall(glEnable(GL_CULL_FACE));
    //GLCall(glCullFace(GL_BACK));
    //GLCall(glFrontFace(GL_CCW));
}

Renderer::~Renderer()
{
}

void Renderer::Init()
{
    VRM_ASSERT_MSG(s_Instance == nullptr, "Renderer already initialized.");
    auto* privateRenderer = new Renderer();
    s_Instance = std::unique_ptr<Renderer>(privateRenderer);
}

void Renderer::Shutdown()
{
    s_Instance.reset();
}

Renderer& Renderer::Get()
{
    VRM_ASSERT_MSG(s_Instance != nullptr, "Renderer not initialized.");
    return *s_Instance;
}

void Renderer::beginScene(const CameraBasic& camera)
{
    m_Camera = &camera;

    m_LightRegistry.beginFrame();
}

void Renderer::endScene(const FrameBuffer& target)
{
    // Setting up lights
    m_LightRegistry.endFrame();
    
    // Clustered shading
    m_ClusteredLights.setupClusters({ 12, 12, 24 }, *m_Camera);
    m_ClusteredLights.processLights(*m_Camera);

    // Rendering to the requested target
    target.bind();
    target.clearColorBuffer();
    GLCall(glViewport(m_ViewportOrigin.x, m_ViewportOrigin.y, m_ViewportSize.x, m_ViewportSize.y));

    // Drawing meshes
    for (const auto& mesh : m_Meshes)
    {
        drawMesh(mesh);
    }

    // Clearing data for next frame
    m_Camera = nullptr;
    m_Meshes.clear();
}

void Renderer::submitMesh(const MeshComponent& mesh, const glm::mat4& model)
{
    m_Meshes.push_back({ mesh, model });
}

void Renderer::submitPointLight(const glm::vec3& position, const PointLightComponent& pointLight, const std::string& identifier)
{
    m_LightRegistry.submitPointLight(pointLight, position, identifier);
}

void Renderer::drawMesh(const QueuedMesh& mesh) const
{
    VRM_DEBUG_ASSERT_MSG(m_Camera, "No camera set for rendering. Did you call beginScene?");

    const auto cameraPos = m_Camera->getPosition();

    if (mesh.meshComponent.getWireframe())
        GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
    else
        GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));

    for (size_t i = 0; i < mesh.meshComponent.getSubMeshCount(); i++)
    {
        const auto& subMeshRenderMesh = mesh.meshComponent.getRenderMesh(i);
        const auto& subMeshMaterial = mesh.meshComponent.getMaterial(i);

        // Binding data
        subMeshRenderMesh.getVertexArray().bind();
        subMeshRenderMesh.getIndexBuffer().bind();

        const Shader& shader = subMeshMaterial.getStaticAsset()->getShader();
        shader.bind();

        // Setting uniforms
        shader.setUniformMat4f("u_Model", mesh.model);
        shader.setUniformMat4f("u_View", m_Camera->getView());
        shader.setUniformMat4f("u_Projection", m_Camera->getProjection());
        shader.setUniformMat4f("u_ViewProjection", m_Camera->getViewProjection());
        shader.setUniform3f("u_ViewPosition", cameraPos);
        shader.setUniform1f("u_Near", m_Camera->getNear());
        shader.setUniform1f("u_Far", m_Camera->getFar());
        shader.setUniform2ui("u_ViewportSize", m_ViewportSize.x, m_ViewportSize.y);

        // Setting material textures uniforms
        size_t textureCount = subMeshMaterial.getStaticAsset()->getTextureCount();
        if (textureCount > 0)
        {
            std::vector<int> textureSlots(textureCount);
            for (size_t i = 0; i < textureCount; ++i)
            {
                const auto& texture = subMeshMaterial.getStaticAsset()->getTexture(i);
                texture.getStaticAsset()->getGPUTexture().bind((unsigned int)i);
                textureSlots[i] = (int)i;
            }

            shader.setUniform1iv("u_Texture", (int)textureCount, textureSlots.data());
        }

        // Drawing data
        GLCall(glDrawElements(GL_TRIANGLES, (GLsizei)subMeshRenderMesh.getIndexBuffer().getCount(), GL_UNSIGNED_INT, nullptr));
    }

}

const glm::vec<2, unsigned int>& Renderer::getViewportOrigin() const
{
    return m_ViewportOrigin;
}

const glm::vec<2, unsigned int>& Renderer::getViewportSize() const
{
    return m_ViewportSize;
}

void Renderer::setViewport(const glm::vec<2, unsigned int>& o, const glm::vec<2, unsigned int>& s)
{
    setViewportOrigin(o);
    setViewportSize(s);
}

void Renderer::setViewportOrigin(const glm::vec<2, unsigned int>& o)
{
    m_ViewportOrigin = o;
}

void Renderer::setViewportSize(const glm::vec<2, unsigned int>& s)
{
    m_ViewportSize = s;
}

} // namespace vrm
