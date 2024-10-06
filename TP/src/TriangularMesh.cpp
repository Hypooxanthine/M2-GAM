#include "TriangularMesh.h"

#include <fstream>
#include <sstream>

#include <Vroom/Core/Assert.h>

#include <glm/gtx/string_cast.hpp>

size_t TriangularMesh::addVertex(const Vertex& v)
{
    m_Vertices.push_back(v);
    return m_Vertices.size() - 1;
}

size_t TriangularMesh::addFace(size_t v0, size_t v1, size_t v2)
{
    VRM_ASSERT(v0 < m_Vertices.size());
    VRM_ASSERT(v1 < m_Vertices.size());
    VRM_ASSERT(v2 < m_Vertices.size());
    
    Face f;

    f.i0 = v0;
    f.i1 = v1;
    f.i2 = v2;

    m_Vertices.at(v0).faceIndex = m_Faces.size();
    m_Vertices.at(v1).faceIndex = m_Faces.size();
    m_Vertices.at(v2).faceIndex = m_Faces.size();

    for (glm::length_t i = 0; i < 3; i++)
    {
        size_t edgeVertex0 = (f.indices[(i + 1) % 3]);
        size_t edgeVertex1 = (f.indices[(i + 2) % 3]);
        // We want the smallest index first so that we never store the same edge twice
        std::pair<size_t, size_t> ei = { std::min(edgeVertex0, edgeVertex1), std::max(edgeVertex0, edgeVertex1) };
        
        if (m_MetEdges.contains(ei))
        {
            size_t& globalFaceIndex = m_MetEdges.at(ei).first;
            glm::length_t& localEdgeIndex = m_MetEdges.at(ei).second;

            f.neighbours[i] = globalFaceIndex;

            m_Faces.at(globalFaceIndex).neighbours[localEdgeIndex] = m_Faces.size();

            m_MetEdges.erase(ei);
        }
        else
        {
            m_MetEdges[ei] = std::pair<size_t, glm::length_t>{ m_Faces.size(), i };
        }
    }

    m_Faces.push_back(f);

    return m_Faces.size() - 1;
}

void TriangularMesh::splitFace(size_t faceIndex, const glm::vec3& vertexPosition)
{
    Face& f = m_Faces.at(faceIndex);

    // Adding the vertex to the mesh
    const auto vertexIndex = m_Vertices.size();
    // We set its face to the face we are splitting, because this face will stay in the structure and then its index will stil be relevant
    m_Vertices.push_back(Vertex{ vertexPosition, faceIndex });
    
    // Indices of the face before the split
    const auto indices = f.indices;
    // Neighbours of the face before the split
    const auto neighbors = f.neighbours;

    const auto f0 = faceIndex;
    const auto f1 = m_Faces.size();
    const auto f2 = f1 + 1;

    m_Faces.reserve(m_Faces.size() + 2);

    // Set the vertex face to the first one
    m_Vertices.at(vertexIndex).faceIndex = f0;

    // First face : just change the existing face
    f.i2 = vertexIndex;
    f.n0 = f1;
    f.n1 = f2;
    f.n2 = neighbors[2]; // Should not change

    // Second face : create a new face
    Face newFace;
    newFace.i0 = vertexIndex;
    newFace.i1 = indices[1];
    newFace.i2 = indices[2];
    newFace.n0 = neighbors[0];
    newFace.n1 = f2;
    newFace.n2 = f0;
    m_Faces.push_back(newFace);

    // Third face : create a new face
    newFace.i0 = indices[0];
    newFace.i1 = vertexIndex;
    newFace.i2 = indices[2];
    newFace.n0 = f1;
    newFace.n1 = neighbors[1];
    newFace.n2 = f0;
    m_Faces.push_back(newFace);
}

size_t TriangularMesh::getVertexCount() const
{
    return m_Vertices.size();
}

size_t TriangularMesh::getFaceCount() const
{
    return m_Faces.size();
}

glm::length_t TriangularMesh::localVertexIndex(size_t globalVertexIndex, size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);
    if (f.i0 == globalVertexIndex)
        return 0;
    if (f.i1 == globalVertexIndex)
        return 1;
    if (f.i2 == globalVertexIndex)
        return 2;

    return -1;
}

size_t TriangularMesh::globalVertexIndex(glm::length_t localVertexIndex, size_t faceIndex) const
{
    return m_Faces.at(faceIndex).indices[localVertexIndex];
}

size_t TriangularMesh::firstFaceIndex(size_t vertexIndex) const
{
    return m_Vertices.at(vertexIndex).faceIndex;
}

size_t TriangularMesh::CCWFaceIndex(size_t vertexIndex, size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);
    size_t v_local = localVertexIndex(vertexIndex, faceIndex);

    return f.neighbours[(v_local + 1) % 3];
}

size_t TriangularMesh::CWFaceIndex(size_t vertexIndex, size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);
    size_t v_local = localVertexIndex(vertexIndex, faceIndex);

    return f.neighbours[(v_local + 2) % 3];
}

size_t TriangularMesh::oppositeFaceIndex(size_t vertexIndex, size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);
    glm::length_t v_local = localVertexIndex(vertexIndex, faceIndex);

    return f.neighbours[v_local];
}

void TriangularMesh::printVertexPosition(size_t vertexIndex) const
{
    const auto& v = m_Vertices.at(vertexIndex);

    VRM_LOG_INFO("Vertex {} : {}", vertexIndex, glm::to_string(v.position));
}

void TriangularMesh::printFace(size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);

    VRM_LOG_INFO("Face {} : {} {} {} | {} {} {}", faceIndex, f.i0, f.i1, f.i2, f.n0, f.n1, f.n2);
}

void TriangularMesh::printFaces() const
{
    VRM_LOG_INFO("Faces :");
    for (size_t i = 0; i < m_Faces.size(); i++)
    {
        const auto& f = m_Faces.at(i);

        VRM_LOG_INFO("\tFace {} : {} {} {} | {} {} {}", i, f.i0, f.i1, f.i2, f.n0, f.n1, f.n2);
    }
}

void TriangularMesh::printFacesAroundVertexCCW(size_t vertexIndex) const
{
    size_t f = firstFaceIndex(vertexIndex);
    size_t f0 = f;

    for (;;)
    {
        printFace(f);

        size_t nextFace = CCWFaceIndex(vertexIndex, f);
        if (nextFace == f0)
            break;
        f = nextFace;
    }
}

void TriangularMesh::printFacesAroundVertexCW(size_t vertexIndex) const
{
    size_t f = firstFaceIndex(vertexIndex);
    size_t f0 = f;

    for (;;)
    {
        printFace(f);

        size_t nextFace = CWFaceIndex(vertexIndex, f);
        if (nextFace == f0)
            break;
        f = nextFace;
    }
}

void TriangularMesh::integrityTest() const
{
    // Test in faces

    
    for (size_t i = 0; i < m_Faces.size(); i++)
    {
        const auto& f = m_Faces.at(i);
        
        // Check if aff vertices exist
        VRM_ASSERT(f.i0 < m_Vertices.size());
        VRM_ASSERT(f.i1 < m_Vertices.size());
        VRM_ASSERT(f.i2 < m_Vertices.size());

        // Check if all neighbours have f as neighbour

        auto& nf0 = m_Faces.at(f.n0);
        VRM_ASSERT(nf0.n0 == i || nf0.n1 == i || nf0.n2 == i);
        auto& nf1 = m_Faces.at(f.n1);
        VRM_ASSERT(nf1.n0 == i || nf1.n1 == i || nf1.n2 == i);
        auto& nf2 = m_Faces.at(f.n0);
        VRM_ASSERT(nf2.n0 == i || nf2.n1 == i || nf2.n2 == i);

        i++;
    }
}

void TriangularMesh::loadOFF(const std::string& path)
{
    VRM_LOG_INFO("Loading .off TriangularMesh {}", path);
    m_Vertices.clear();
    m_Faces.clear();
    m_MetEdges.clear();

    std::ifstream ifs;

    ifs.open(path);
    VRM_ASSERT(ifs.is_open());

    std::string line;
    std::getline(ifs, line); // OFF

    size_t verticesNb = 0;
    size_t facesNb = 0;

    std::getline(ifs, line);
    std::stringstream ss(line);
    
    ss >> verticesNb;
    ss >> facesNb;

    while(m_Vertices.size() < verticesNb && std::getline(ifs, line))
    {
        ss = std::stringstream(line);
        Vertex v;
        ss >> v.position.x;
        ss >> v.position.y;
        ss >> v.position.z;
        addVertex(v);
    }

    while(m_Faces.size() < facesNb && std::getline(ifs, line))
    {
        ss = std::stringstream(line);
        size_t i0, i1, i2;
        size_t faces;
        ss >> faces;
        VRM_ASSERT(faces == 3);

        ss >> i0;
        ss >> i1;
        ss >> i2;
        addFace(i0, i1, i2);
    }

    VRM_LOG_INFO("{} vertices and {} faces loaded.", m_Vertices.size(), m_Faces.size());
}

void TriangularMesh::saveOFF(const std::string& path) const
{
    std::ofstream ofs;
    ofs.open(path, std::ios_base::trunc);

    VRM_ASSERT(ofs.is_open());

    ofs << "OFF\n";
    ofs << m_Vertices.size() << " " << m_Faces.size() << " 0\n\n";

    for (const auto& v : m_Vertices)
    {
        ofs << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
    }

    ofs << "\n";

    for (const auto& f : m_Faces)
    {
        ofs << "3 " << f.i0 << " " << f.i1 << " " << f.i2 << "\n";
    }

    ofs.close();

    VRM_LOG_INFO("File {} written.", path);
}

vrm::MeshData TriangularMesh::toMeshData() const
{
    std::vector<vrm::Vertex> vertices;
    std::vector<uint32_t> indices;

    const size_t triangleCount = m_Faces.size();

    vertices.reserve(triangleCount * 3);
    indices.reserve(triangleCount * 3);

    for (const auto& f : m_Faces)
    {
        vrm::Vertex A, B, C;
        A.position = m_Vertices.at(f.i0).position;
        B.position = m_Vertices.at(f.i1).position;
        C.position = m_Vertices.at(f.i2).position;

        A.texCoords = { 0.f, 0.f };
        B.texCoords = { 0.f, 0.f };
        C.texCoords = { 0.f, 0.f };

        const glm::vec3 AB = B.position - A.position;
        const glm::vec3 AC = C.position - A.position;
        const glm::vec3 normal = glm::normalize(glm::cross(AB, AC));

        A.normal = normal;
        B.normal = normal;
        C.normal = normal;

        const uint32_t indexOffset = static_cast<uint32_t>(vertices.size());

        vertices.push_back(A);
        vertices.push_back(B);
        vertices.push_back(C);

        indices.push_back(indexOffset + 0);
        indices.push_back(indexOffset + 1);
        indices.push_back(indexOffset + 2);
    }

    return vrm::MeshData{ std::move(vertices), std::move(indices) };
}

vrm::MeshData TriangularMesh::toSmoothMeshData() const
{
    std::vector<vrm::Vertex> vertices;
    std::vector<uint32_t> indices;

    vertices.reserve(getVertexCount());
    indices.reserve(getFaceCount() * 3);

    for (size_t i = 0; i < m_Vertices.size(); i++)
    {
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

        v.scalar = 0.f;

        vertices.push_back(v);
    }

    for (const auto& f : m_Faces)
    {
        indices.push_back(static_cast<uint32_t>(f.i0));
        indices.push_back(static_cast<uint32_t>(f.i1));
        indices.push_back(static_cast<uint32_t>(f.i2));
    }

    return vrm::MeshData{ std::move(vertices), std::move(indices) };
}