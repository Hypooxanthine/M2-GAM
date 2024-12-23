#include "TriangularMesh.h"

#include <fstream>
#include <sstream>
#include <unordered_set>

#include <Vroom/Core/Assert.h>

#include <glm/gtx/string_cast.hpp>

TriangularMesh::TriangularMesh()
{
}

void TriangularMesh::clear()
{
    *this = TriangularMesh();
}

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

size_t TriangularMesh::addFirstFaceForTriangulation(size_t v0, size_t v1, size_t v2)
{
    m_Faces.reserve(m_Faces.size() + 4);

    addVertex(Vertex{ { 0.f, -10000000.f, 0.f }, 0 });
    size_t iInf = m_Vertices.size() - 1;
    m_InfiniteVertexIndex = iInf;
    Vertex& infiniteVertex = m_Vertices.at(iInf);
    
    Face& f = m_Faces.emplace_back();
    size_t iF = m_Faces.size() - 1;
        f.i0 = v0;
        f.i1 = v1;
        f.i2 = v2;

    Face& fInf0 = m_Faces.emplace_back();
    size_t iFInf0 = m_Faces.size() - 1;
        fInf0.i0 = iInf;
        fInf0.i1 = v1;
        fInf0.i2 = v0;

    Face& fInf1 = m_Faces.emplace_back();
    size_t iFInf1 = m_Faces.size() - 1;
        fInf1.i0 = iInf;
        fInf1.i1 = v2;
        fInf1.i2 = v1;

    Face& fInf2 = m_Faces.emplace_back();
    size_t iFInf2 = m_Faces.size() - 1;
        fInf2.i0 = iInf;
        fInf2.i1 = v0;
        fInf2.i2 = v2;

    f.n0 = iFInf1;
    f.n1 = iFInf2;
    f.n2 = iFInf0;

    fInf0.n0 = iF;
    fInf0.n1 = iFInf2;
    fInf0.n2 = iFInf1;

    fInf1.n0 = iF;
    fInf1.n1 = iFInf0;
    fInf1.n2 = iFInf2;

    fInf2.n0 = iF;
    fInf2.n1 = iFInf1;
    fInf2.n2 = iFInf0;

    // First face the infinite vertex is turning around is the first infinite face created
    infiniteVertex.faceIndex = iFInf0;

    // For the vertices of the first face, their first face they will turn around
    // will be the first face itself
    for (glm::length_t i = 0; i < 3; i++)
        m_Vertices.at(f.indices[i]).faceIndex = iF;

    m_IsForTriangulation = true;

    return iF;
}

void TriangularMesh::faceSplit(size_t iF, const glm::vec3& vertexPosition)
{
    auto iv3 = addVertex(Vertex{ vertexPosition, 0});
    auto& vertex3 = m_Vertices.at(iv3);

    m_Faces.reserve(m_Faces.size() + 2);

    auto& f = m_Faces.at(iF);
    size_t if3 = m_Faces.size();
    auto& f3 = m_Faces.emplace_back();
    size_t if4 = m_Faces.size();
    auto& f4 = m_Faces.emplace_back();

    const glm::length_t v0 = 0, v1 = 1, v2 = 2;

    const size_t iv0 = f.indices[v0];
    const size_t iv1 = f.indices[v1];
    const size_t iv2 = f.indices[v2];

    auto& vertex0 = m_Vertices.at(iv0);
    auto& vertex1 = m_Vertices.at(iv1);
    auto& vertex2 = m_Vertices.at(iv2);

    const size_t if0 = f.neighbours[v0];
    const size_t if1 = f.neighbours[v1];
    const size_t if2 = f.neighbours[v2];

    auto& f0 = m_Faces.at(if0);
    auto& f1 = m_Faces.at(if1);
    auto& f2 = m_Faces.at(if2);

    const glm::length_t v00 = (localVertexIndex(iv1, if0) + 1) % 3;
    const glm::length_t v10 = (localVertexIndex(iv2, if1) + 1) % 3;
    const glm::length_t v20 = (localVertexIndex(iv0, if2) + 1) % 3;

    // Defining new faces (f3 & f4) vertex indices
    f3.i0 = iv3;
    f3.i1 = iv0;
    f3.i2 = iv1;

    f4.i0 = iv3;
    f4.i1 = iv1;
    f4.i2 = iv2;

    // Initial face (f) has a vertex index modified
    f.indices[v1] = iv3;

    // f0 and f4 are neighbors
    f0.neighbours[v00] = if4;
    f4.n0 = if0;

    // f2 and f3 are neighbors
    f2.neighbours[v20] = if3;
    f3.n0 = if2;

    // f and f1 were neighbors, and still are: no change

    // f3 and f4 are neighbors
    f3.n1 = if4;
    f4.n2 = if3;

    // f4 and f are neighbors
    f4.n1 = iF;
    f.neighbours[v0] = if4;

    // f and f3 are neighbors
    f.neighbours[v2] = if3;
    f3.n2 = iF;

    // Modiying the first face each vertex is turning around
    vertex0.faceIndex = if2;
    vertex1.faceIndex = if0;
    vertex2.faceIndex = if1;
    vertex3.faceIndex = iF;
}

TriangularMesh::Edge TriangularMesh::edgeFlip(size_t vertexIndex0, size_t vertexIndex1)
{
    //VRM_LOG_TRACE("Flipping edge between vertices {} and {}", vertexIndex0, vertexIndex1);

    // Searching for incident faces
    size_t if0 = 0, if1 = CWFaceIndex(vertexIndex0, firstFaceIndex(vertexIndex0));
    glm::length_t v00 = 0;
    glm::length_t v10 = 0;
    bool found = false;
    for (auto it = begin_turning_faces(vertexIndex0); it != end_turning_faces(vertexIndex0); ++it)
    {
        if0 = if1;
        if1 = *it;
        const auto& f1 = m_Faces.at(if1);
        auto vertexIndex0_localf1 = localVertexIndex(vertexIndex0, if1);

        if (f1.indices[(vertexIndex0_localf1 + 1) % 3] == vertexIndex1)
        {
            found = true;

            v00 = (localVertexIndex(vertexIndex0, if0) + 1) % 3;
            v10 = (localVertexIndex(vertexIndex1, if1) + 1) % 3;
            break;
        }
    }

    if (!found)
        printFacesAroundVertexCCW(vertexIndex0);
    VRM_ASSERT_MSG(found, "Edge not found.");

    auto& f0 = m_Faces.at(if0);
    auto& f1 = m_Faces.at(if1);
    size_t ifp0 = f0.neighbours[(v00 + 1) % 3];
    auto& fp0 = m_Faces.at(ifp0);
    size_t ifp1 = f1.neighbours[(v10 + 1) % 3];
    auto& fp1 = m_Faces.at(ifp1);
    size_t iv00 = f0.indices[v00];
    size_t iv11 = f1.indices[v10];
    glm::length_t vp00 = localVertexIndex(iv00, ifp0);
    glm::length_t vp10 = localVertexIndex(iv11, ifp1);

    // rearrange the faces
    f0.indices[(v00 + 2) % 3] = iv11;
    f0.neighbours[(v00 + 0) % 3] = ifp1;
    fp1.neighbours[(vp10 + 2) % 3] = if0;
    f0.neighbours[(v00 + 1) % 3] = if1;

    f1.indices[(v10 + 2) % 3] = iv00;
    f1.neighbours[(v10 + 0) % 3] = ifp0;
    fp0.neighbours[(vp00 + 2) % 3] = if1;
    f1.neighbours[(v10 + 1) % 3] = if0;

    // New first faces
    m_Vertices.at(vertexIndex0).faceIndex = if1;
    m_Vertices.at(vertexIndex1).faceIndex = if0;

    return Edge{
        .e0 = iv11,
        .e1 = iv00,
        .t0 = if1,
        .t1 = if0
    };
}

void TriangularMesh::edgeFlip(const glm::vec3& coords)
{
    if (auto containingFaceID = getFaceContainingPoint(coords); containingFaceID.has_value())
    {
        auto& f = m_Faces.at(containingFaceID.value());
        
        float minDet = std::numeric_limits<float>::max();
        size_t v0 = 0, v1 = 0;
        for (glm::length_t i = 0; i < 3; i++)
        {
            const auto& v_i = m_Vertices.at(f.indices[i]);
            const auto& v_i_next = m_Vertices.at(f.indices[(i + 1) % 3]);
            float det = glm::determinant(glm::mat3(
                v_i_next.position - v_i.position,
                coords - v_i.position,
                glm::vec3(0.f, 1.f, 0.f)
            ));

            if (det < minDet)
            {
                minDet = det;
                v0 = f.indices[i];
                v1 = f.indices[(i + 1) % 3];
            }
        }

        edgeFlip(v0, v1);
    }
    else
    {
        VRM_LOG_ERROR("Errore while flipping edge.");
    }
}

bool TriangularMesh::isFaceInfinite(size_t faceIndex) const
{
    if (!m_IsForTriangulation)
        return false;
        
    for (auto it = begin_turning_faces(m_InfiniteVertexIndex); it != end_turning_faces(m_InfiniteVertexIndex); ++it)
    {
        if (*it == faceIndex)
            return true;
    }

    return false;
}

std::optional<size_t> TriangularMesh::getFaceContainingPoint(const glm::vec3& vertexPosition) const
{
    glm::vec2 p = glm::vec2(vertexPosition.x, -vertexPosition.z);

    for (size_t f_i = 0; f_i < m_Faces.size(); f_i++)
    {
        if (isFaceInfinite(f_i))
            continue;
            
        const auto& f = m_Faces.at(f_i);
        bool inside = true;

        for (glm::length_t i = 0; i < 3; i++)
        {
            const glm::length_t i_next = (i + 1) % 3;
            const auto& v_i = m_Vertices.at(f.indices[i]);
            const auto& v_i_next = m_Vertices.at(f.indices[i_next]);
            const glm::vec2 v_i_2D = glm::vec2(v_i.position.x, -v_i.position.z);
            const glm::vec2 v_i_next_2D = glm::vec2(v_i_next.position.x, -v_i_next.position.z);
            
            const auto det = glm::determinant(glm::mat2(p - v_i_2D, v_i_next_2D - v_i_2D));

            if (det > 0.f)
            {
                inside = false;
                break;
            }
        }

        if (inside)
            return f_i;
    }

    return std::nullopt;
}

bool TriangularMesh::canPointSeeEdge(const glm::vec3& point, size_t vertexIndex0, size_t vertexIndex1) const
{
    const auto& v0 = m_Vertices.at(vertexIndex0).position;
    const auto& v1 = m_Vertices.at(vertexIndex1).position;

    const glm::vec2 A = glm::vec2(v0.x, -v0.z);
    const glm::vec2 B = glm::vec2(v1.x, -v1.z);
    const glm::vec2 P = glm::vec2(point.x, -point.z);

    return glm::determinant(glm::mat2(P - A, B - A)) > 0.f;
}

size_t TriangularMesh::addVertex_StreamingTriangulation(const glm::vec3& vertexPosition)
{
    if (auto containingFaceID = getFaceContainingPoint(vertexPosition); containingFaceID.has_value())
    {
        faceSplit(containingFaceID.value(), vertexPosition);
        return m_Vertices.size() - 1;
    }

    // If we couldn't find any face containing the point, we will need to add geometry outside convex hull

    bool foundFirst = false;
    auto first = end_turning_faces(m_InfiniteVertexIndex);
    auto last = end_turning_faces(m_InfiniteVertexIndex);
    
    for (auto it = begin_turning_faces(m_InfiniteVertexIndex); it != end_turning_faces(m_InfiniteVertexIndex); ++it)
    {
        const auto& f = m_Faces.at(*it);
        auto iInfVertex_local = localVertexIndex(m_InfiniteVertexIndex, *it);
        
        if (canPointSeeEdge(vertexPosition, f.indices[(iInfVertex_local + 2) % 3], f.indices[(iInfVertex_local + 1) % 3]))
        {
            if (!foundFirst)
            {
                foundFirst = true;
                first = it;
            }

            last = it;
        }
        else if (foundFirst)
            break;
    }

    // Now we make sure the first we found is really the first
    // When iterating, if we started on an edge that is visible by the new point,
    // we need to check if we can see more edges going clockwise.

    for (auto it = first; ;)
    {
        --it;
        const auto& f = m_Faces.at(*it);
        auto iInfVertex_local = localVertexIndex(m_InfiniteVertexIndex, *it);

        if (canPointSeeEdge(vertexPosition, f.indices[(iInfVertex_local + 2) % 3], f.indices[(iInfVertex_local + 1) % 3]))
        {
            first = it;
        }
        else
            break;
    }

    // Keeping track of current and next faces because we are changing geometry:
    auto currentFace = first;
    auto nextFace = std::next(first, 1);

    // Now we can add the new point.
    // For the first one, we only split the face:
    faceSplit(*currentFace, vertexPosition);

    if (*first == *last)
        return m_Vertices.size() - 1;

    currentFace = nextFace;
    ++nextFace;

    // For the other ones, we will flip an infinite edge:
    for (;;)
    {
        auto& f = m_Faces.at(*currentFace);
        auto iInfVertex_local = localVertexIndex(m_InfiniteVertexIndex, *currentFace);
        edgeFlip(
            f.indices[(iInfVertex_local + 0) % 3],
            f.indices[(iInfVertex_local + 1) % 3]
        );

        if (*currentFace == *last)
            break;

        currentFace = nextFace;
        ++nextFace;
    }

    return m_Vertices.size() - 1;
}

bool TriangularMesh::isEdgeDelaunay(const Edge& edge) const
{
    // Paraboloid method
    const auto& t0 = m_Faces.at(edge.t0);
    const auto& t1 = m_Faces.at(edge.t1);
    auto A = m_Vertices.at(edge.e1).position;
    auto B = m_Vertices.at(edge.e0).position;
    auto C = m_Vertices.at(t0.indices[(localVertexIndex(edge.e0, edge.t0) + 1) % 3]).position;
    auto D = m_Vertices.at(t1.indices[(localVertexIndex(edge.e1, edge.t1) + 1) % 3]).position;

    // Lifting triangle t0 on the paraboloid
    A.y = std::powf(A.x, 2.f) + std::powf(A.z, 2.f);
    A.z = -A.z;
    B.y = std::powf(B.x, 2.f) + std::powf(B.z, 2.f);
    B.z = -B.z;
    C.y = std::powf(C.x, 2.f) + std::powf(C.z, 2.f);
    C.z = -C.z;
    D.y = std::powf(D.x, 2.f) + std::powf(D.z, 2.f);
    D.z = -D.z;

    // D has to be under the plane (ABC) to be inside the circonscribed triangle
    return glm::determinant(glm::mat3(B - A, C - A, D - A)) < 0.f;
}

int TriangularMesh::addVertex_StreamingDelaunayTriangulation(const glm::vec3& vertexPosition)
{
    size_t vertexIndex = addVertex_StreamingTriangulation(vertexPosition);

    // Edges to check
    std::deque<Edge> checkList;

    for (auto it = begin_turning_faces(vertexIndex); it != end_turning_faces(vertexIndex); ++it)
    {
        if (isFaceInfinite(*it))
            continue;

        const auto& f = m_Faces.at(*it);
        auto iv_local = localVertexIndex(vertexIndex, *it);
        auto i_opp = f.neighbours[iv_local];

        if (!isFaceInfinite(i_opp))
        {
            Edge& e = checkList.emplace_back();
                e.e0 = f.indices[(iv_local + 1) % 3];
                e.e1 = f.indices[(iv_local + 2) % 3];
                e.t0 = i_opp;
                e.t1 = *it;
        }

        auto nextIt = std::next(it, 1);

        if (isFaceInfinite(*nextIt))
            continue;

        Edge& e = checkList.emplace_back();
            e.e0 = f.indices[(iv_local + 2) % 3];
            e.e1 = f.indices[iv_local];
            e.t0 = *nextIt;
            e.t1 = *it;
    }

    return delaunayAlgorithm(checkList);
}

int TriangularMesh::delaunayAlgorithm(std::deque<Edge>& checkList)
{
    size_t justInCase = 1'000;
    int flipsCount = 0;

    for ( ; !checkList.empty() && justInCase != 0; --justInCase)
    {
        auto edge = checkList.front();
        checkList.pop_front();

        if (!isEdgeDelaunay(edge))
        {
            // @todo There are more edges to check, not only the edge created by the flip.
            Edge e = edgeFlip(edge.e0, edge.e1);

            bool t0Inf = isFaceInfinite(e.t0);
            bool t1Inf = isFaceInfinite(e.t1);

            if (!t0Inf)
            {
                const auto& t0 = m_Faces.at(e.t0);
                
                glm::length_t ie00 = localVertexIndex(e.e0, e.t0);
                glm::length_t ie10 = localVertexIndex(e.e1, e.t0);
                
                size_t it1p = t0.neighbours[ie10];
                size_t it0p = t0.neighbours[ie00];

                size_t iep = t0.indices[(ie00 + 1) % 3];

                if (!isFaceInfinite(it1p))
                    checkList.emplace_back(Edge{
                        .e0 = e.e0,
                        .e1 = iep,
                        .t0 = it1p,
                        .t1 = e.t0
                    });

                if (!isFaceInfinite(it0p))
                    checkList.emplace_back(Edge{
                        .e0 = iep,
                        .e1 = e.e1,
                        .t0 = it0p,
                        .t1 = e.t0
                    });
            }

            if (!t1Inf)
            {
                const auto& t1 = m_Faces.at(e.t1);

                glm::length_t ie01 = localVertexIndex(e.e0, e.t1);
                glm::length_t ie11 = localVertexIndex(e.e1, e.t1);

                size_t it1pp = t1.neighbours[ie11];
                size_t it0pp = t1.neighbours[ie01];
                size_t iepp = t1.indices[(ie11  + 1) % 3];

                if (!isFaceInfinite(it1pp))
                    checkList.emplace_back(Edge{
                        .e0 = e.e0,
                        .e1 = iepp,
                        .t0 = e.t1,
                        .t1 = it1pp
                    });

                if (!isFaceInfinite(it0pp))
                    checkList.emplace_back(Edge{
                        .e0 = iepp,
                        .e1 = e.e1,
                        .t0 = e.t1,
                        .t1 = it0pp
                    });
            }

            if (!t0Inf && !t1Inf)
                checkList.emplace_back(e);

            ++flipsCount;
        }
    }

    VRM_ASSERT_MSG(justInCase > 0, "Too many flips were made when adding a vertex to the triangulation while keeping Delaunay property.");

    return flipsCount;
}

template<>
struct std::hash<TriangularMesh::Edge>
{
    std::size_t operator()(const TriangularMesh::Edge& e) const noexcept
    {
        auto hasher = std::hash<size_t>();
        auto seed = hasher(e.e0);
        hash_combine(seed, e.e1);
        return seed;
    }
};

bool operator==(const TriangularMesh::Edge& left, const TriangularMesh::Edge& right)
{
    return left.e0 == right.e0 && left.e1 == right.e1;
}

int TriangularMesh::delaunayAlgorithm()
{
    std::unordered_set<Edge> edges;

    for (size_t i = 0; i < m_Faces.size(); i++)
    {
        if (isFaceInfinite(i))
            continue;
        
        const auto& f = m_Faces.at(i);

        for (glm::length_t j = 0; j < 3; j++)
        {
            auto i_neighbour = f.neighbours[j];

            if (isFaceInfinite(i_neighbour))
                continue;

            const auto& f_neighbour = m_Faces.at(i_neighbour);

            Edge e;
            e.e0 = f.indices[(j + 1) % 3];
            e.e1 = f.indices[(j + 2) % 3];

            if (edges.contains(e))
                continue;

            std::swap(e.e0, e.e1);

            e.t0 = i;
            e.t1 = i_neighbour;

            edges.insert(e);
        }
    }

    std::deque<Edge> checkList(edges.begin(), edges.end());

    return delaunayAlgorithm(checkList);
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
    VRM_LOG_INFO("Faces around vertex {}:", vertexIndex);

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
        
        // Check if all vertices exist
        VRM_ASSERT(f.i0 < m_Vertices.size());
        VRM_ASSERT(f.i1 < m_Vertices.size());
        VRM_ASSERT(f.i2 < m_Vertices.size());
        VRM_ASSERT(f.n0 < m_Faces.size());
        VRM_ASSERT(f.n1 < m_Faces.size());
        VRM_ASSERT(f.n2 < m_Faces.size());

        // Check if all neighbours have f as neighbour

        auto& nf0 = m_Faces.at(f.n0);
        VRM_ASSERT(nf0.n0 == i || nf0.n1 == i || nf0.n2 == i);
        auto& nf1 = m_Faces.at(f.n1);
        VRM_ASSERT(nf1.n0 == i || nf1.n1 == i || nf1.n2 == i);
        auto& nf2 = m_Faces.at(f.n2);
        VRM_ASSERT(nf2.n0 == i || nf2.n1 == i || nf2.n2 == i);
    }

    // Vertices: test if each vertex starts on a face containing the vertex
    for (size_t i = 0; i < m_Vertices.size(); i++)
    {
        auto& v = m_Vertices.at(i);
        auto& f = m_Faces.at(v.faceIndex);

        VRM_ASSERT(f.i0 == i || f.i1 == i || f.i2 == i);
    }

    VRM_LOG_INFO("Integrity test success.");
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

    for (size_t i = 0; i < m_Faces.size(); i++)
    {
        if (isFaceInfinite(i))
            continue;

        const Face& f = m_Faces.at(i);

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
        if (isFaceInfinite(m_Vertices.at(i).faceIndex))
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