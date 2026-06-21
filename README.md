# Voxel Projekt

Ein **Voxel-Projekt** mit unendlichem Terrain, ähnlich zu **Minecraft**.

Das Projekt ist in **C++** geschrieben und nutzt **WGPU-Dawn** und **CMake**.  
Da es auf **WGPU** basiert, könnte es theoretisch auch im **Web** laufen.

## Features

- **Unendliches Terrain**
- **Voxel-basierte Weltgenerierung**
- **Per-Voxel-Normals**
- **Terrain Editing per Raycasting**
  - **Voxel entfernen und hinzufügen**
- **Flexible Terrain-Generierung mit dem FastNoise2 Node Graph**
- **Verschiedene Materialien**
- **MSAA**
- **Dynamic Memory Allocation**
- **Dynamic Memory Defragmentation**

## Optimizations

Das Projekt enthält eine Menge Optimierungen, damit das Ganze möglichst performant läuft:

- **Binary Greedy Meshing**
- **Level of Details (LODs)**
- **Backface Culling**
- **Frustum Culling**
- **Vertex Pulling**
- **SIMD Optimizations**
- **Spatial Hashing**
- **Multithreading**
- **Custom Buffer Mapping**
- **Bit packing**
