#pragma once

#include <Vroom/Asset/AssetData/MeshData.h>
#include <Vroom/Core/Log.h>

#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <type_traits>
#include <thread>
#include <optional>
#include <deque>

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

    /**
     * Layout:
     *        e0
     *      / |  \
     *     /  |   \
     *    /   |    \
     *   v  t0 | t1 v
     *    \   |    /
     *     \  |   /
     *      \ |  /
     *        e1
     * 
     */
    struct Edge
    {
        // Edge indices
        size_t e0, e1;
        // Triangle indices
        size_t t0, t1;
    };
public:
    TriangularMesh();

    TriangularMesh(const TriangularMesh& other) = default;

    void clear();

    // Adds a vertex and returns its index.
    size_t addVertex(const Vertex& v);
    
    // Adds a face with existing vertex indices and returns the index of the created face.
    size_t addFace(size_t v0, size_t v1, size_t v2);

    size_t addFirstFaceForTriangulation(size_t v0, size_t v1, size_t v2);

    void faceSplit(size_t faceIndex, const glm::vec3& vertexPosition);

    Edge edgeFlip(size_t vertexIndex0, size_t vertexIndex1);

    // Finds the nearest edge and performs an edge flip.
    void edgeFlip(const glm::vec3& coords);

    bool isFaceInfinite(size_t faceIndex) const;

    std::optional<size_t> getFaceContainingPoint(const glm::vec3& vertexPosition) const;

    bool canPointSeeEdge(const glm::vec3& point, size_t vertexIndex0, size_t vertexIndex1) const;

    size_t addVertex_StreamingTriangulation(const glm::vec3& vertexPosition);

    bool isEdgeDelaunay(const Edge& edge) const;

    int addVertex_StreamingDelaunayTriangulation(const glm::vec3& vertexPosition);

    int delaunayAlgorithm(std::deque<Edge>& checkList);

    int delaunayAlgorithm();

    size_t getVertexCount() const;

    size_t getFaceCount() const;

    inline const Vertex& getVertex(size_t index) const { return m_Vertices.at(index); }

    inline Vertex& getVertex(size_t index) { return m_Vertices.at(index); }

    inline const Face& getFace(size_t index) const { return m_Faces.at(index); }

    glm::length_t localVertexIndex(size_t globalVertexIndex, size_t faceIndex) const;

    size_t globalVertexIndex(glm::length_t localVertexIndex, size_t faceIndex) const;

    size_t firstFaceIndex(size_t vertexIndex) const;

    size_t CCWFaceIndex(size_t vertexIndex, size_t faceIndex) const;

    size_t CWFaceIndex(size_t vertexIndex, size_t faceIndex) const;

    size_t oppositeFaceIndex(size_t vertexIndex, size_t faceIndex) const;

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

    template <typename Fn>
    vrm::MeshData toHeatMeshData(Fn heatFunction, float factor) const;

    template<typename Fn>
    auto laplacian(const size_t vertexIndex, Fn u) const -> typename std::invoke_result<Fn, size_t>::type;

public: // Iterators
    using Iterator_on_vertices = std::vector<Vertex>::iterator;
    using Iterator_on_faces = std::vector<Face>::iterator;

    class Circulator_on_faces
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = size_t;
        using difference_type = int;
        using pointer = size_t*;
        using reference = size_t&;
        
    public:
        Circulator_on_faces(const TriangularMesh* mesh, size_t vertexIndex, size_t faceIndex, int count = 0)
            : m_Mesh(mesh), m_VertexIndex(vertexIndex), m_FaceIndex(faceIndex), m_Count(count)
        {
        }

        Circulator_on_faces(const Circulator_on_faces& other) = default;

        Circulator_on_faces& operator=(const Circulator_on_faces& other) = default;

        size_t operator*() const
        {
            return m_FaceIndex;
        }

        size_t operator->() const
        {
            return m_FaceIndex;
        }

        Circulator_on_faces& operator++()
        {
            m_FaceIndex = m_Mesh->CCWFaceIndex(m_VertexIndex, m_FaceIndex);
            if (m_FaceIndex == m_Mesh->firstFaceIndex(m_VertexIndex))
                m_Count++;
            return *this;
        }

        Circulator_on_faces operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        Circulator_on_faces& operator--()
        {
            m_FaceIndex = m_Mesh->CWFaceIndex(m_VertexIndex, m_FaceIndex);
            if (m_FaceIndex == m_Mesh->firstFaceIndex(m_VertexIndex))
                m_Count--;
            return *this;
        }

        Circulator_on_faces operator--(int)
        {
            auto temp = *this;
            --(*this);
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
        const TriangularMesh* m_Mesh;
        size_t m_VertexIndex;
        size_t m_FaceIndex;
        int m_Count = 0;
    };

    class Circulator_on_vertices
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        
    public:
        Circulator_on_vertices(const TriangularMesh* mesh, size_t vertexIndex, int count = 0)
            : m_Mesh(mesh), m_VertexIndex(vertexIndex), m_Face(mesh, vertexIndex, mesh->firstFaceIndex(vertexIndex), count), m_Count(count)
        {
            m_TurningVertexIndex_local = (m_Mesh->localVertexIndex(m_VertexIndex, *m_Face) + 1) % 3;
        }

        size_t operator*() const
        {
            return m_Mesh->getFace(*m_Face).indices[m_TurningVertexIndex_local];
        }

        Circulator_on_vertices& operator++()
        {
            ++m_Face;
            m_TurningVertexIndex_local = (m_Mesh->localVertexIndex(m_VertexIndex, *m_Face) + 1) % 3;
            if (*m_Face == m_Mesh->firstFaceIndex(m_VertexIndex))
                m_Count++;
            return *this;
        }

        Circulator_on_vertices operator++(int)
        {
            auto temp = *this;
            ++(*this);
            return temp;
        }

        Circulator_on_vertices& operator--()
        {
            --m_Face;
            m_TurningVertexIndex_local = (m_Mesh->localVertexIndex(m_VertexIndex, *m_Face) + 1) % 3;
            if (*m_Face == m_Mesh->firstFaceIndex(m_VertexIndex))
                m_Count--;
            return *this;
        }

        Circulator_on_vertices operator--(int)
        {
            auto temp = *this;
            --(*this);
            return temp;
        }

        bool operator==(const Circulator_on_vertices& other) const
        {
            return m_VertexIndex == other.m_VertexIndex && *m_Face == *other.m_Face && m_Count == other.m_Count;
        }

        bool operator!=(const Circulator_on_vertices& other) const
        {
            return !(*this == other);
        }

    private:
        const TriangularMesh* m_Mesh;
        size_t m_VertexIndex;
        Circulator_on_faces m_Face;
        glm::length_t m_TurningVertexIndex_local;
        int m_Count = 0;
    };

public: // Iterator access

    Iterator_on_vertices begin_vertices() { return m_Vertices.begin(); }
    Iterator_on_vertices end_vertices() { return m_Vertices.end(); }

    Iterator_on_faces begin_faces() { return m_Faces.begin(); }
    Iterator_on_faces end_faces() { return m_Faces.end(); }

    Circulator_on_faces begin_turning_faces(size_t vertexIndex) const
    {
        return Circulator_on_faces(this, vertexIndex, firstFaceIndex(vertexIndex), 0);
    }
    
    Circulator_on_faces end_turning_faces(size_t vertexIndex) const
    {
        return Circulator_on_faces(this, vertexIndex, firstFaceIndex(vertexIndex), 1);
    }

    Circulator_on_vertices begin_turning_vertices(size_t vertexIndex) const
    {
        return Circulator_on_vertices(this, vertexIndex, 0);
    }

    Circulator_on_vertices end_turning_vertices(size_t vertexIndex) const
    {
        return Circulator_on_vertices(this, vertexIndex, 1);
    }

private:
    std::vector<Vertex> m_Vertices;
    std::vector<Face> m_Faces;

    std::unordered_map<std::pair<size_t, size_t>, std::pair<size_t, glm::length_t>> m_MetEdges;

    bool m_IsForTriangulation = false;
    size_t m_InfiniteVertexIndex = 0;
};

/* Templates implementation */

template<typename Fn>
auto TriangularMesh::laplacian(const size_t vertexIndex, Fn u) const -> typename std::invoke_result<Fn, size_t>::type
{
    using ReturnType = typename std::invoke_result<Fn, size_t>::type;
    const size_t i = vertexIndex;

    ReturnType laplacian = ReturnType(0);
    float sumAreas = 0.f;

    // Laplacian with cotangent formula
    for (auto it = begin_turning_faces(vertexIndex); it != end_turning_faces(vertexIndex); ++it)
    {
        auto f = *it;
        // Finding vertex j
        glm::length_t i_local_CCW = localVertexIndex(i, f);
        glm::length_t j_local_CCW = (i_local_CCW + 1) % 3;
        size_t j = globalVertexIndex(j_local_CCW, f);

        auto ij = u(j) - u(i);

        auto leftFaceIt = it;
        leftFaceIt--;
        
        size_t leftFaceIndex = *leftFaceIt;
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
    }

    laplacian = laplacian / (2.f * sumAreas / 3.f);

    return laplacian;
}

template<typename Fn>
vrm::MeshData TriangularMesh::toHeatMeshData(Fn heatFunction, float factor) const
{
    std::vector<vrm::Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.resize(getVertexCount());
    indices.reserve(getFaceCount() * 3);

    auto cores = std::thread::hardware_concurrency();

    auto work = [this, &vertices, &indices, heatFunction, factor](size_t start, size_t end)
    {
        for (size_t i = start; i < end; i++)
        {
            if (isFaceInfinite(i))
                continue;
            
            Face f = m_Faces.at(m_Vertices.at(i).faceIndex);
            vrm::Vertex v;
            v.position = m_Vertices.at(i).position;
            v.texCoords = { 0.f, 0.f };

            auto positionFunction = [this](size_t i) -> glm::vec3 { return m_Vertices.at(i).position; };
            v.normal = laplacian(i, positionFunction);

            const glm::vec3 AB = m_Vertices.at(f.i1).position - m_Vertices.at(f.i0).position;
            const glm::vec3 AC = m_Vertices.at(f.i2).position - m_Vertices.at(f.i0).position;
            glm::vec3 flatNormal = glm::cross(AB, AC);
            if (glm::dot(v.normal, flatNormal) < 0.f)
                v.normal = -v.normal;

            v.scalar = heatFunction(i) + laplacian(i, heatFunction) * factor;

            vertices.at(i) = v;
        }
    };
    
    std::vector<std::thread> threads;
    size_t step = getVertexCount() / cores;

    for (size_t i = 0; i < cores; i++)
        threads.emplace_back(work, i * step, (i == (cores - 1)) ? getVertexCount() : (i + 1) * step);

    for (auto& t : threads)
        t.join();

    for (const auto& f : m_Faces)
    {
        indices.push_back(static_cast<uint32_t>(f.i0));
        indices.push_back(static_cast<uint32_t>(f.i1));
        indices.push_back(static_cast<uint32_t>(f.i2));
    }

    return vrm::MeshData{ std::move(vertices), std::move(indices) };
}