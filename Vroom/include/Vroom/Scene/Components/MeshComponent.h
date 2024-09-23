#pragma once

#include <vector>

#include "Vroom/Asset/AssetInstance/MeshInstance.h"
#include "Vroom/Asset/AssetInstance/MaterialInstance.h"

namespace vrm
{

class MeshData;
class RenderMesh;

/**
 * @brief Mesh component.
 * 
 * A mesh component is a component that stores the mesh of an entity.
 */
class MeshComponent
{
public:
    MeshComponent() = delete;

    /**
     * @brief Construct a new Mesh Component object with custom materials.
     * 
     * @param mesh The mesh instance.
     * @param materials The materials of the submeshes. Must be complete.
     */
    MeshComponent(const MeshInstance& mesh, const std::vector<MaterialInstance>& materials);

    /**
     * @brief Construct a new Mesh Component object with default materials.
     * 
     * @param mesh The mesh instance.
     */
    MeshComponent(const MeshInstance& mesh);

    /**
     * @brief Get the number of submeshes.
     * 
     * @return size_t The number of submeshes.
     */
    size_t getSubMeshCount() const;

    /**
     * @brief Get the mesh data of a submesh.
     * 
     * @param slot The slot of the submesh.
     * @return const MeshData& The mesh data.
     */
    const MeshData& getMeshData(size_t slot) const;

    /**
     * @brief Get the render mesh of a submesh.
     * 
     * @param slot The slot of the submesh.
     * @return const RenderMesh& The render mesh.
     */
    const RenderMesh& getRenderMesh(size_t slot) const;

    /**
     * @brief Get the material of a submesh.
     * 
     * @param slot The slot of the submesh.
     * @return const MaterialInstance& The material.
     */
    const MaterialInstance& getMaterial(size_t slot) const;

    /**
     * @brief Set the mesh with default materials.
     * 
     * @param mesh The mesh instance.
     */
    void setMesh(const MeshInstance& mesh);

    /**
     * @brief Set the mesh with custom materials.
     * 
     * @param mesh The mesh instance.
     * @param materials The materials of the submeshes. Must be complete.
     */
    void setMesh(const MeshInstance& mesh, const std::vector<MaterialInstance>& materials);

    /**
     * @brief Set the material of a submesh.
     * 
     * @param slot The slot of the submesh.
     * @param material The material.
     */
    void setMaterial(size_t slot, const MaterialInstance& material);

private:
    MeshInstance m_MeshInstance;
    std::vector<MaterialInstance> m_MaterialInstances;
};

} // namespace vrm