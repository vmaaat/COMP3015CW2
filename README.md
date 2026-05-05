# COMP3015 - Assignment 2

## Interactive OpenGL Rendering Scene

------------------------------

### Overview

This project extends a modern OpenGL rendering pipeline into an interactive real-time scene implementing in C++ using custom GLSL shaders.

The scene now includes:
- An imported OBJ model (cat statue)
- Procedural textured terrain
- Cubemap skybox
- Multi-light Blinn-Phong shading model
- First person camera system
- Shadow mapping
- Post-processing (bloom effect)
- Distance-based fog (toggleable)
- Player movement system with sprint and stamina
- Object interaction system with visual feedback
- On-screen UI elements (stamina bar and reticle)

The focus of this project was expanding from static rendering into a fully interactive graphics application which combines lighting, post-processing and gameplay style systems.

------------------------------

### Development Environment

- IDE: Visual Studio 2022
- Operating System: Windows 10
- Architecture: x64
- Graphics API: OpenGL 4.6

External libraries used:
- GLFW - Window creation and input handling
- GLAD - OpenGL function loader
- TinyOBJLoader - OBJ model loading
- stb_image - Texture loading

------------------------------

### How the Project Works

# Program Structure
- main.cpp: creates the window and initialises the scene.
- scenebasic_uniform.cpp / .h
- Handles;
- - Shader compilation
  - Model and texture loading
  - Player movement and input
  - Lighting and shadow updates
  - Interaction Logic
# Shaders
- shader/basic_uniform.vert / frag: Main lighting, shadows and fog
- shader/shadow_map.vert / frag: shadow depth pass
- shader/skybox.vert / frag: cubemap skybox
- screen_quad.vert / simple_bloom.frog: post-processing

------------------------------

### Rendering Features

A per-fragment Blinn-Phong shading model is implemented.

Each light contributes:
- Ambient lighting
- Diffuse reflection (Lambertian)
- Specular highlights (Blinn-Phong)

Two light sources:
1. Dynamic light attached to the player (flashlight effect)
2. Static scene light

Lighting can be toggled at runtime.

------------------------------

### Shadow Mapping

- Implemented using a depth framebuffer (FBO)
- Scene rendering from light's perspective to generate shadow map
- Applied in fragment shader using depth comparison

This produces real-time shadows under scene objects.

------------------------------

### Skybox

- Implemented using a cubemap texture
- Rendered with depth adjustments to appear infinitely distant

------------------------------

### Post-Processing (Bloom)

- Scene rendered to an off-screen framebuffer
- Bright fragments are extracted and blended back into the scene
- Controlled via a fullscreen quad shader

Bloom can be toggled on/off during runtime.

------------------------------

### Fog System (Interactive Feature)

- Distance-based fog implemented in fragment shader
- Smoothly blends scene colour with fog colour based on depth
- Triggered via interaction with the statue

Fog transitions are animated using a fade-in / fade-out effect.

------------------------------

### Controls

- W / A / S / D - movement with acceleration and smoothing 
- Mouse - First-person camera using yaw and pitch
- Shift - Sprint mechanic with stamina
- E - Interact
- B - Bloom toggle off / on
  
------------------------------

### Problems Encountered 

Several issues were encountered and resolved:

- Texture flipping inconsistencies (stb_image)
- Shadow artefacts and depth precision issues
- Framebuffer configuration errors
- Post-processing causing darkened output
- Camera collision and boundary handling

These were resolved through shader adjustments, correct OpenGL state management and improved logic handling.

------------------------------

### Executable

A release build is included.

To run:

- Ensure all shader and media folders are in the correct directory
- Run the executable from the project root

------------------------------

### Youtube Video

https://youtu.be/KmUoV9hAuHA

------------------------------

### Screenshots

<img width="796" height="598" alt="image" src="https://github.com/user-attachments/assets/c2e7a98e-d59a-4c32-8819-b11e7126455f" />
<img width="795" height="593" alt="image" src="https://github.com/user-attachments/assets/1da3a0cf-5cf9-4636-bf46-8194ea4cedc0" />
<img width="791" height="591" alt="image" src="https://github.com/user-attachments/assets/2d0f9b97-e18a-4190-90d1-97c397a64f9a" />

------------------------------

### Assets Used

Statue 3D Model - https://sketchfab.com/3d-models/statue-ecd586875a8845cebb74aa9ea11756ed

Skybox Texture - https://opengameart.org/content/cloudy-skyboxes-0

Grass Texture - https://opengameart.org/content/30-grass-textures-tilable
