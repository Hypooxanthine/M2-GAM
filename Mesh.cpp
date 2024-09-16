#include "Mesh.h"

#include <fstream>
#include <sstream>

#include "Assert.h"

size_t Mesh::addVertex(const Vertex& v)
{
    m_Vertices.push_back(v);
    return m_Vertices.size() - 1;
}

size_t Mesh::addFace(size_t v0, size_t v1, size_t v2)
{
    ASSERT(v0 < m_Vertices.size());
    ASSERT(v1 < m_Vertices.size());
    ASSERT(v2 < m_Vertices.size());
    
    Face f;

    f.i0 = v0;
    f.i1 = v1;
    f.i2 = v2;

    m_Vertices.at(v0).faceIndex = m_Faces.size();
    m_Vertices.at(v1).faceIndex = m_Faces.size();
    m_Vertices.at(v2).faceIndex = m_Faces.size();

    for (uint8_t i = 0; i < 3; i++)
    {
        std::pair<size_t, size_t> ei;
        if (i == 0)
            ei = { std::min(v1, v2), std::max(v1, v2) };
        else if (i == 1)
            ei = { std::min(v0, v2), std::max(v0, v2) };
        else
            ei = { std::min(v0, v1), std::max(v0, v1) };
        
        if (m_MetEdges.contains(ei))
        {
            size_t& globalFaceIndex = m_MetEdges.at(ei).first;
            size_t& localEdgeIndex = m_MetEdges.at(ei).second;

            if (i == 0)
                f.n0 = globalFaceIndex;
            else if (i == 1)
                f.n1 = globalFaceIndex;
            else
                f.n2 = globalFaceIndex;

            if (localEdgeIndex == 0)
                m_Faces.at(globalFaceIndex).n0 = m_Faces.size();
            else if (localEdgeIndex == 1)
                m_Faces.at(globalFaceIndex).n1 = m_Faces.size();
            else
                m_Faces.at(globalFaceIndex).n2 = m_Faces.size();
        }
        else
        {
            m_MetEdges[ei] = std::pair<size_t, size_t>{ m_Faces.size(), i };
        }
    }

    m_Faces.push_back(f);

    return m_Faces.size() - 1;
}

size_t Mesh::localVertexIndex(size_t globalVertexIndex, size_t faceIndex) const
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

size_t Mesh::globalVertexIndex(size_t localVertexIndex, size_t faceIndex) const
{
    return m_Faces.at(faceIndex).indices[localVertexIndex];
}

size_t Mesh::firstFaceIndex(size_t vertexIndex) const
{
    return m_Vertices.at(vertexIndex).faceIndex;
}

size_t Mesh::CCWFaceIndex(size_t vertexIndex, size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);
    size_t v_local = localVertexIndex(vertexIndex, faceIndex);

    return f.neighbours[(v_local + 1) % 3];
}

size_t Mesh::CWFaceIndex(size_t vertexIndex, size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);
    size_t v_local = localVertexIndex(vertexIndex, faceIndex);

    return f.neighbours[(v_local + 2) % 3];
}

size_t Mesh::oppositeFaceIndex(size_t vertexIndex, size_t faceIndex) const
{
    const auto& f = m_Faces.at(faceIndex);
    size_t v_local = localVertexIndex(vertexIndex, faceIndex);

    return f.neighbours[v_local];
}

Vector3f Mesh::laplacian(size_t vertexIndex) const
{
    size_t i = vertexIndex;

    size_t f = firstFaceIndex(i);
    size_t f0 = f;

    Vector3f laplacian = { 0.f, 0.f, 0.f };
    float sumAreas = 0.f;
    size_t faceCount = 0;

    // Laplacian with cotangent formula
    for (;;)
    {
        faceCount++;
    
        // Finding vertex j
        size_t i_local = localVertexIndex(i, f);
        size_t j_local = (i_local + 1) % 3;
        size_t j = m_Faces.at(f).indices[j_local];

        Vector3f ij = m_Vertices.at(j).position - m_Vertices.at(i).position;
        
        size_t leftFaceIndex = CWFaceIndex(i, f);
        size_t rightFaceIndex = f;

        size_t cwVertexIndex_local = (localVertexIndex(leftFaceIndex, i) + 1) % 3;
        size_t cwVertexIndex = m_Faces.at(leftFaceIndex).indices[cwVertexIndex_local];

        size_t ccwVertexIndex_local = (localVertexIndex(rightFaceIndex, i) + 2) % 3;
        size_t ccwVertexIndex = m_Faces.at(rightFaceIndex).indices[ccwVertexIndex_local];

        Vector3f cw_to_i_normalized = normalized(m_Vertices.at(i).position - m_Vertices.at(cwVertexIndex).position);
        Vector3f cw_to_j_normalized = normalized(m_Vertices.at(j).position - m_Vertices.at(cwVertexIndex).position);
        float cos_alpha = dot(cw_to_i_normalized, cw_to_j_normalized);
        float sin_alpha = sqrt(1 - cos_alpha * cos_alpha);
        float cot_alpha = cos_alpha / sin_alpha;

        Vector3f ccw_to_i_normalized = normalized(m_Vertices.at(i).position - m_Vertices.at(ccwVertexIndex).position);
        Vector3f ccw_to_j_normalized = normalized(m_Vertices.at(j).position - m_Vertices.at(ccwVertexIndex).position);
        float cos_beta = dot(ccw_to_i_normalized, ccw_to_j_normalized);
        float sin_beta = lenght(normalized(cross(ij, m_Vertices.at(ccwVertexIndex).position - m_Vertices.at(j).position)));
        float cot_beta = cos_beta / sin_beta;

        laplacian = laplacian + (cot_alpha + cot_beta) * ij;
        sumAreas += (1.f / 3.f) * 0.5f * lenght(cross(ij, m_Vertices.at(ccwVertexIndex).position - m_Vertices.at(j).position));

        size_t nextFace = CCWFaceIndex(i, f);
        if (nextFace == f0)
            break;
        f = nextFace;
    }

    laplacian = laplacian / (2.f * sumAreas);

    std::cout << "Face count : " << faceCount << '\n';
    std::cout << "sumAreas : " << sumAreas << '\n';

    return laplacian;
}

void Mesh::printFaces() const
{
    std::cout << m_Faces.size() << " faces.\n";
    for (size_t i = 0; i < m_Faces.size(); i++)
    {
        const auto& f = m_Faces.at(i);

        std::cout << i << " " << f.i0 << " " << f.i1 << " " << f.i2 << " " << f.n0 << " " << f.n1 << " " << f.n2 << '\n';
    }
}

void Mesh::integrityTest() const
{
    // Test in faces

    
    for (size_t i = 0; i < m_Faces.size(); i++)
    {
        const auto& f = m_Faces.at(i);
        
        // Check if aff vertices exist
        ASSERT(f.i0 < m_Vertices.size());
        ASSERT(f.i1 < m_Vertices.size());
        ASSERT(f.i2 < m_Vertices.size());

        // Check if all neighbours have f as neighbour

        auto& nf0 = m_Faces.at(f.n0);
        ASSERT(nf0.n0 == i || nf0.n1 == i || nf0.n2 == i);
        auto& nf1 = m_Faces.at(f.n1);
        ASSERT(nf1.n0 == i || nf1.n1 == i || nf1.n2 == i);
        auto& nf2 = m_Faces.at(f.n0);
        ASSERT(nf2.n0 == i || nf2.n1 == i || nf2.n2 == i);

        i++;
    }
}

void Mesh::loadOFF(const std::string& path)
{
    m_Vertices.clear();
    m_Faces.clear();
    m_MetEdges.clear();

    std::ifstream ifs;

    ifs.open(path);
    ASSERT(ifs.is_open());

    std::string line;
    std::getline(ifs, line); // OFF

    size_t verticesNb = 0;
    size_t facesNb = 0;

    std::getline(ifs, line);
    std::stringstream ss(line);
    
    ss >> verticesNb;
    ss >> facesNb;

    std::cout << verticesNb << " vertices and " << facesNb << " faces.\n";

    while(m_Vertices.size() < verticesNb && std::getline(ifs, line))
    {
        ss = std::stringstream(line);
        Vertex v;
        ss >> v.position.x;
        ss >> v.position.y;
        ss >> v.position.z;
        addVertex(v);
    }

    std::cout << m_Vertices.size() << " vertices added.\n";

    while(m_Faces.size() < facesNb && std::getline(ifs, line))
    {
        ss = std::stringstream(line);
        size_t i0, i1, i2;
        size_t faces;
        ss >> faces;
        ASSERT(faces == 3);

        ss >> i0;
        ss >> i1;
        ss >> i2;
        addFace(i0, i1, i2);
    }

    std::cout << m_Faces.size() << " faces added.\n";
    std::cout << ".off mesh " << path << " loaded.\n";
}

void Mesh::saveOFF(const std::string& path) const
{
    std::ofstream ofs;
    ofs.open(path, std::ios_base::trunc);

    ASSERT(ofs.is_open());

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

    std::cout << "File " << path << " written.\n";
}