#include "RayCasting.h"

#include <glm/gtx/rotate_vector.hpp>

#include <Vroom/Asset/AssetData/MeshData.h>

Ray Math::RayUnderCursor(const glm::vec3& cameraPos, const glm::mat4& view, const glm::mat4& projection, const glm::vec2& cursorPos, const glm::vec2& viewportSize)
{
    Ray out;
    out.origin = cameraPos;

    const float mouseX = 2.f * cursorPos.x / viewportSize.x - 1.f;
    const float mouseY = 1.f - 2.f * cursorPos.y / viewportSize.y;

    const glm::vec4 rayClip = glm::vec4(mouseX, mouseY, -1.f, 1.f);
    glm::vec4 rayView = glm::inverse(projection) * rayClip;
    rayView.z = -1.f;
    rayView.w = 0.f;
    const glm::vec3 rayWorld = glm::inverse(view) * rayView;

    out.direction = glm::normalize(rayWorld);

    return out;
}

HitResult Math::RayCastWithMesh(const Ray& ray, const vrm::MeshComponent& mesh, const glm::mat4& modelMatrix)
{
    HitResult out;
    out.hasHit = false;

    const auto& vertices = mesh.getMeshData(0).getVertices();
    const auto& indices = mesh.getMeshData(0).getIndices();

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        unsigned int iA = indices[i], iB = indices[i+1], iC = indices[i+2];
        // Positions of points of triangle
        glm::vec3 A = modelMatrix * glm::vec4{ vertices[iA].position.x, vertices[iA].position.y, vertices[iA].position.z, 1.f };
        glm::vec3 B = modelMatrix * glm::vec4{ vertices[iB].position.x, vertices[iB].position.y, vertices[iB].position.z, 1.f };
        glm::vec3 C = modelMatrix * glm::vec4{ vertices[iC].position.x, vertices[iC].position.y, vertices[iC].position.z, 1.f };

        glm::vec3 n = glm::cross(B - A, C - A);

        // Test with plane of triangle ABC
        float depth = glm::dot(n, A - ray.origin) / glm::dot(n, ray.direction);

        // If depth is negative : triangle is behind the ray
        // Even before checking if ray intersected the triangle, we can already check depths
        if(depth < 0 || (out.hasHit == true && out.depth < depth)) continue;

        // Test if point belongs to the triangle

        glm::vec3 P = ray.origin + depth * ray.direction; // P is the intersection point

        glm::vec3 cp1 = glm::cross(B-A, P-A);
        glm::vec3 cp2 = glm::cross(C-B, P-B);

        if(!AreVectorsOfSameDirection(cp1, cp2)) continue;

        glm::vec3 cp3 = glm::cross(A-C, P-C);

        if(!AreVectorsOfSameDirection(cp2, cp3)) continue;

        // Ray intersected triangle.

        out.hasHit = true;
        out.depth = depth;
        out.normal = glm::normalize(n);
        out.position = P;
    }

    return out;
}

HitResult Math::RayCastWithPlane(const Ray& ray, const glm::vec3& planeNormal, const glm::vec3& planePoint)
{
    HitResult out;
    out.hasHit = false;

    float depth = glm::dot(planeNormal, planePoint - ray.origin) / glm::dot(planeNormal, ray.direction);

    if(depth < 0) return out;

    out.hasHit = true;
    out.depth = depth;
    out.position = ray.origin + depth * ray.direction;
    out.normal = planeNormal;

    return out;
}

bool Math::AreVectorsOfSameDirection(const glm::vec3& u, const glm::vec3& v)
{
    return glm::dot(u, v) >= 0.f;
}

glm::mat4 Math::AlignVectors(const glm::vec3& from, const glm::vec3& to)
{
    glm::vec3 n = glm::cross(from, to);
    // If "from" and "to" are colinear, n will be the null vector. We need to check that. No rotation is needed in this case because vectors are already aligned, so we return identity matrix.
    if(glm::dot(n, n) < 0.0001f) return glm::mat4(1.f);

    float angle = glm::acos(glm::dot(from, to) / (glm::length(from) * glm::length(to)));

    n = glm::normalize(n);

    glm::mat4 mat = glm::rotate(glm::mat4(1.f), angle, n);

    return mat;
}