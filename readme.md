# TP de Maillages et geometrie algorithmique

## Description

Implementation of Laplacian operator for curvature and heat computation on a 3D mesh.
Implementation of Delaunay triangulation in 2D (WIP).

## Screenshots

![Curvature](Screenshots/LaplacianCurvatureRender.png)
![Heat diffusion](Screenshots/HeatDiffusion.png)

## Building project

### Windows

#### Visual Studio

In repository root folder, open a terminal and type :

```bash
mkdir build
cd build
cmake ..
```

#### VS Code

You can also build the project by opening the root folder on VS Code, and use the "CMake" and "CMake Tools" VS Code extensions (that is my workflow).

### Linux

Install required dependencies:
```bash
sudo apt install libglfw3-dev
```

#### CMake/Make

- In repository root folder, open a terminal and type:
```bash
mkdir build
cd build
cmake ..
cmake --build . --parallel
```

- Launching program:
```bash
cd TP # Don't use ./TP/TP directly because the program won't be able to load resource files.
./TP
```

#### VS Code

You can also build the project by opening the root folder on VS Code, and use the "CMake" and "CMake Tools" VS Code extensions. You might also need to install the [Ninja build system](https://github.com/ninja-build/ninja) if it is not already installed on your system:

```bash
sudo apt install ninja-build
```

## Dependencies

- [Vroom](https://github.com/Hypooxanthine/Vroom), my 3D library written in C++/OpenGL (I modified it a bit to fit the needs of this project)
- [imgui](https://github.com/ocornut/imgui), for the GUI