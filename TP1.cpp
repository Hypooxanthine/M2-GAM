#include <iostream>

#include "Mesh.h"
#include "Assert.h"

int main(int argc, char** argv)
{
    /**
    Mesh tetrahedron;

    tetrahedron.addVertex({ 0.f, 0.f, 0.f}); // 0
    tetrahedron.addVertex({ 1.f, 0.f, 0.f}); // 1
    tetrahedron.addVertex({ 0.f, 1.f, 0.f}); // 2
    tetrahedron.addVertex({ 0.f, 0.f, 1.f}); // 3

    tetrahedron.addFace(0, 2, 1); // Base
    tetrahedron.addFace(0, 1, 3);
    tetrahedron.addFace(1, 2, 3);
    tetrahedron.addFace(2, 0, 3);
    
    tetrahedron.printFaces();

    tetrahedron.integrityTest();

    tetrahedron.saveOFF("./test.off");
    */
    
    ASSERT(argc == 3);
    Mesh m;
    m.loadOFF(argv[1]);
    size_t vertexId = std::stoull(argv[2]);
    std::cout << "Laplacien de la fonction positions pour le sommet d'indice " << vertexId << " : " << m.laplacian(vertexId) << '\n';
}