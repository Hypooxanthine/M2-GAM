#pragma once

#include <Vroom/Asset/AssetData/MeshData.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <type_traits>

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

    inline glm::length_t localVertexIndex(size_t globalVertexIndex, size_t faceIndex) const;

    inline size_t globalVertexIndex(glm::length_t localVertexIndex, size_t faceIndex) const;

    inline size_t firstFaceIndex(size_t vertexIndex) const;

    inline size_t CCWFaceIndex(size_t vertexIndex, size_t faceIndex) const;

    inline size_t CWFaceIndex(size_t vertexIndex, size_t faceIndex) const;

    inline size_t oppositeFaceIndex(size_t vertexIndex, size_t faceIndex) const;

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

    template<typename Fn>
    auto laplacian(const size_t vertexIndex, Fn u) const -> typename std::invoke_result<Fn, size_t>::type;

public: // Iterators
    using VertexIterator = std::vector<Vertex>::iterator;
    using FaceIterator = std::vector<Face>::iterator;

    class Circulator_on_faces
    {
    public:
        Circulator_on_faces(TriangularMesh& mesh, size_t vertexIndex, size_t faceIndex, size_t count = 0)
            : m_Mesh(mesh), m_VertexIndex(vertexIndex), m_FaceIndex(faceIndex), m_Count(count)
        {
        }

        Face& operator*()
        {
            return m_Mesh.m_Faces.at(m_FaceIndex);
        }

        Circulator_on_faces& operator++()
        {
            m_FaceIndex = m_Mesh.CCWFaceIndex(m_VertexIndex, m_FaceIndex);
            return *this;
        }

        Circulator_on_faces& operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        Circulator_on_faces& operator--()
        {
            m_FaceIndex = m_Mesh.CWFaceIndex(m_VertexIndex, m_FaceIndex);
            if (m_FaceIndex == m_Mesh.firstFaceIndex(m_VertexIndex))
                m_Count++;
            return *this;
        }

        Circulator_on_faces& operator--(int)
        {
            auto temp = *this;
            --(*this);
            if (m_FaceIndex == m_Mesh.firstFaceIndex(m_VertexIndex))
                m_Count--;
            return temp;
        }

        bool operator==(const Circulator_on_faces& other) const
        {
            return m_VertexIndex == other.m_VertexIndex && m_FaceIndex == other.m_FaceIndex && m_Count == other.m_Count;
        }

        bool operator!=(const Circulator_on_faces& other) const
        {
            return !(*this == other);
        }

    private:
        TriangularMesh& m_Mesh;
        size_t m_VertexIndex;
        size_t m_FaceIndex;
        size_t m_Count = 0;
    };

public: // Iterator access

    VertexIterator verticesBegin() { return m_Vertices.begin(); }
    VertexIterator verticesEnd() { return m_Vertices.end(); }

    FaceIterator facesBegin() { return m_Faces.begin(); }
    FaceIterator facesEnd() { return m_Faces.end(); }

    Circulator_on_faces turnAroundVertexBegin(size_t vertexIndex)
    {
        return Circulator_on_faces(*this, vertexIndex, firstFaceIndex(vertexIndex), 0);
    }
    
    Circulator_on_faces turnAroundVertexEnd(size_t vertexIndex)
    {
        return Circulator_on_faces(*this, vertexIndex, firstFaceIndex(vertexIndex), 1);
    }

private:
    std::vector<Vertex> m_Vertices;
    std::vector<Face> m_Faces;

    std::unordered_map<std::pair<size_t, size_t>, std::pair<size_t, glm::length_t>> m_MetEdges;
};

/* Templates implementation */

template<typename Fn>
auto TriangularMesh::laplacian(const size_t vertexIndex, Fn u) const -> typename std::invoke_result<Fn, size_t>::type
    {
        using ReturnType = typename std::invoke_result<Fn, size_t>::type;
        const size_t i = vertexIndex;

        size_t f = firstFaceIndex(i);
        size_t f0 = f;

        ReturnType laplacian = ReturnType(0);
        float sumAreas = 0.f;
        size_t faceCount = 0;

        // Laplacian with cotangent formula
        for (;;)
        {
            faceCount++;
        
            // Finding vertex j
            glm::length_t i_local_CCW = localVertexIndex(i, f);
            glm::length_t j_local_CCW = (i_local_CCW + 1) % 3;
            size_t j = globalVertexIndex(j_local_CCW, f);

            auto ij = u(j) - u(i);
            
            size_t leftFaceIndex = CWFaceIndex(i, f);
            size_t& rightFaceIndex = f;

            glm::length_t cwVertexIndex_local = (localVertexIndex(i, leftFaceIndex) + 1) % 3;
            size_t cwVertexIndex = globalVertexIndex(cwVertexIndex_local, leftFaceIndex);

            glm::length_t ccwVertexIndex_local = (j_local_CCW + 1) % 3;
            size_t ccwVertexIndex = globalVertexIndex(ccwVertexIndex_local, rightFaceIndex);

            glm::vec3 cw_to_i = m_Vertices.at(i).position - m_Vertices.at(cwVertexIndex).position;
            glm::vec3 cw_to_j = m_Vertices.at(j).position - m_Vertices.at(cwVertexIndex).position;
            float cot_alpha = dot(cw_to_i, cw_to_j) / length(cross(cw_to_i, cw_to_j));

            glm::vec3 ccw_to_i = m_Vertices.at(i).position - m_Vertices.at(ccwVertexIndex).position;
            glm::vec3 ccw_to_j = m_Vertices.at(j).position - m_Vertices.at(ccwVertexIndex).position;
            float cot_beta = dot(ccw_to_i, ccw_to_j) / length(cross(ccw_to_i, ccw_to_j));

            laplacian = laplacian + (cot_alpha + cot_beta) * ij;
            sumAreas += length(cross(ccw_to_i, ccw_to_j)) / 2.f;

            f = CCWFaceIndex(i, f);
            if (f == f0)
                break;
        }

        laplacian = laplacian / (2.f * sumAreas / 3.f);

        return laplacian;
    }