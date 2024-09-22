#pragma once

#include <Vroom/Asset/AssetData/MeshData.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>

#include <glm/glm.hpp>

#include "Hash.h"

template<>
struct std::hash<std::pair<size_t, size_t>>
{
    std::size_t operator()(const std::pair<size_t, size_t>& p) const noexcept
    {
        auto hasher = std::hash<size_t>();
        auto seed = hasher(p.first);
        hash_combine(seed, p.second);
        return seed;
    }
};

class TriangularMesh
{
public:

    struct Vertex
    {
        glm::vec3 position;
        size_t faceIndex;
    };

    struct Face
    {
        union
        {
            struct { size_t i0, i1, i2; };
            glm::vec<3, size_t> indices;
            
        };
        union
        {
            struct { size_t n0, n1, n2; };
            glm::vec<3, size_t> neighbours;
        };
    };
public:
    // Adds a vertex and returns its index.
    size_t addVertex(const Vertex& v);
    
    // Adds a face with existing vertex indices and returns the index of the created face.
    size_t addFace(size_t v0, size_t v1, size_t v2);

    inline size_t localVertexIndex(size_t globalVertexIndex, size_t faceIndex) const;

    inline size_t globalVertexIndex(size_t localVertexIndex, size_t faceIndex) const;

    inline size_t firstFaceIndex(size_t vertexIndex) const;

    inline size_t CCWFaceIndex(size_t vertexIndex, size_t faceIndex) const;

    inline size_t CWFaceIndex(size_t vertexIndex, size_t faceIndex) const;

    inline size_t oppositeFaceIndex(size_t vertexIndex, size_t faceIndex) const;

    glm::vec3 laplacianPosition(size_t vertexIndex) const;

    void printVertexPosition(size_t vertexIndex) const;

    void printFace(size_t faceIndex) const;

    void printFaces() const;

    void printFacesAroundVertexCCW(size_t vertexIndex) const;

    void printFacesAroundVertexCW(size_t vertexIndex) const;

    void integrityTest() const;

    void loadOFF(const std::string& path);
    void saveOFF(const std::string& path) const;

    vrm::MeshData toMeshData() const;

    vrm::MeshData toSmoothMeshData() const;

private:
    std::vector<Vertex> m_Vertices;
    std::vector<Face> m_Faces;

    std::unordered_map<std::pair<size_t, size_t>, std::pair<size_t, size_t>> m_MetEdges;
};