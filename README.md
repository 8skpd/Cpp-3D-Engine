# 3D Software Renderer

A lightweight, educational 3D engine written in C++ with **software rendering** (CPU-based rasterization). Built from scratch — OpenGL is used only to display the final framebuffer.

![Demo](demo.png) <!-- optional: add screenshot later -->

## ✨ Features

- **Primitive generation**: cube, sphere, torus, pyramid, Möbius strip  
- **OBJ model loader** (supports vertices, normals, and automatic triangulation)  
- **Interactive camera**: orbit around the scene with adjustable distance and FOV  
- **Lighting models**: Lambert (diffuse) and Phong (diffuse + specular)  
- **Visibility algorithms**: Back Face Culling, Z-buffer, Painter’s Algorithm  
- **Rasterization**: triangle filling, Bresenham’s line algorithm  
- **Real-time UI**: Dear ImGui for lighting, camera, and rendering controls  
- **Cross-platform**: tested on Ubuntu Linux (should work on Windows too)

## 🛠 Build Instructions

### Prerequisites (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev
git clone https://github.com/your-username/3d-software-renderer.git
cd 3d-software-renderer

# Initialize Dear ImGui (submodule)
git submodule update --init

# Build
cmake -B build
cmake --build build

# Run
./build/3D_Engine
```


This project was developed as part of a computer graphics course to demonstrate core concepts of 3D rendering:

3D transformations and camera systems
Lighting and shading models
Visibility determination
Software rasterization
It serves as an educational tool to understand how modern graphics engines work under the hood.
