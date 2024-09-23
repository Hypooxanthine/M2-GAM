#include "Vroom/Scene/Components/MeshComponent.h"

#include "Vroom/Asset/Asset.h"
#include "Vroom/Render/RenderObject/RenderMesh.h"

namespace vrm
{

MeshComponent::MeshComponent(const MeshInstance& meshInstance, const std::vector<MaterialInstance>& materialInstances)
{
    setMesh(meshInstance, materialInstances);
}

MeshComponent::MeshComponent(const MeshInstance& meshInstance)
{
    setMesh(meshInstance);
}

size_t MeshComponent::getSubMeshCount() const
{
    return m_MeshInstance.getStaticAsset()->getSubMeshes().size();
}

const MeshData& MeshComponent::getMeshData(size_t index) const
{
    return m_MeshInstance.getStaticAsset()->getSubMeshes().at(index).meshData;
}

const RenderMesh& MeshComponent::getRenderMesh(size_t index) const
{
    return m_MeshInstance.getStaticAsset()->getSubMeshes().at(index).renderMesh;
}

const MaterialInstance& MeshComponent::getMaterial(size_t index) const
{
    return m_MaterialInstances.at(index);
}

void MeshComponent::setMesh(const MeshInstance& meshInstance)
{
    m_MeshInstance = meshInstance;
    m_MaterialInstances.clear();
    for (const auto& subMesh : m_MeshInstance.getStaticAsset()->getSubMeshes())
    {
        m_MaterialInstances.push_back(subMesh.defaultMaterial);
    }
}

void MeshComponent::setMesh(const MeshInstance& meshInstance, const std::vector<MaterialInstance>& materialInstances)
{
    VRM_ASSERT_MSG(meshInstance.getStaticAsset()->getSubMeshes().size() == materialInstances.size(), "The number of materials must match the number of submeshes.");
    m_MeshInstance = meshInstance;
    m_MaterialInstances = materialInstances;
}

void MeshComponent::setMaterial(size_t slot, const MaterialInstance& material)
{
    VRM_ASSERT_MSG(slot < m_MaterialInstances.size(), "Invalid material slot.");
    m_MaterialInstances.at(slot) = material;
}

} // namespace vrm
