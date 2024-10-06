/**
 * @file RayCasting.h
 * @author Alexandre Beaujon
 * @brief Adapted from my project https://github.com/Hypooxanthine/Mesh-Clouder .
 * 
 */

#pragma once

#include <glm/glm.hpp>
#include <Vroom/Scene/Components/MeshComponent.h>

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
};

struct HitResult
{
    bool hasHit;
    float depth;
    glm::vec3 position;
    glm::vec3 normal;
};

class Math
{
public:
    static Ray RayUnderCursor(const glm::vec3& cameraPos, const glm::mat4& viewProjection, const glm::vec2& cursorPos, const glm::vec2& viewportSize);
    static HitResult RayCastWithMesh(const Ray& ray, const vrm::MeshComponent& mesh, const glm::mat4& modelMatrix);

    /**
     * @brief This assumes that vectors are collinear.
     * 
     * @param u First vector.
     * @param v Second vector.
     * @return true Same directions.
     * @return false Inverse directions.
     */
    static bool AreVectorsOfSameDirection(const glm::vec3& u, const glm::vec3& v);

    /**
     * @brief Returns a rotation matrix M so that M * "from" is aligned with "to".
     * 
     * @param from The vector to align
     * @param to The target vector
     * @return glm::mat4 Rotation to apply to "from" so that its aligned with "to"
     */
    static glm::mat4 AlignVectors(const glm::vec3& from, const glm::vec3& to);
};