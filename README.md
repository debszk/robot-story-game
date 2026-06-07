# Robot Story Game

An interactive 3D robot animation project built with C++, OpenGL, GLU, and
FreeGLUT. The application combines hierarchical robot modeling, procedural
terrain, cinematic storytelling, particle effects, dynamic lighting, and
multiple camera systems in a real-time scene.

Created as a final OpenGL laboratory project at the West University of
Timisoara, the game demonstrates how core computer graphics concepts can be
combined into a visually coherent and interactive environment.

## Project Description

The scene follows three animated robots across a procedurally generated
landscape. In the default story mode, the robots approach the center of the
scene before a lightning strike causes one robot to fall. Sparks appear while
the remaining robots react and run away.

The application also includes alternative behavior modes, including random
walking and a special synchronized dance sequence. Users can pause the
animation, switch cameras and projections, change the lighting, adjust the
animation speed, and inspect the scene from an orbit camera.

## Features

- Hierarchical robot models built from cubes, cylinders, spheres, and cones
- Articulated walking, waving, falling, and reaction animations
- Scripted cinematic story controlled by a state machine
- Procedural curved terrain with calculated surface normals
- Cubic Bezier path movement with tangent-based robot orientation
- Distance-based robot interactions and a rescue beacon objective
- Lightning and additive-blended spark particle effects
- Directional, point, spotlight, moving-light, and dual-color lighting modes
- Matte, plastic, and metallic material properties
- Dynamic day/night cycle, fog, and simple projected shadows
- Front, side, top, orbit, and robot-follow camera modes
- Perspective and orthographic projections
- Four-view split-screen rendering
- BMP texture loading with procedural fallback textures
- Keyboard and mouse controls

## Technologies Used

- **C++**
- **OpenGL fixed-function pipeline**
- **GLU**
- **FreeGLUT**
- **Microsoft Visual Studio 2022**
- **MSVC v143 toolset**
- **Windows SDK**

## Requirements

- Windows 10 or Windows 11
- Visual Studio 2022 with the **Desktop development with C++** workload
- A graphics driver with OpenGL support
- FreeGLUT headers, import library, and runtime DLL

The repository already includes the FreeGLUT headers in `include/GL/` and the
import library in `lib/`.

## Installation

1. Clone the repository:

   ```powershell
   git clone <repository-url>
   cd robot-story-game
   ```

2. Confirm that these bundled dependencies are present:

   ```text
   include/GL/freeglut.h
   lib/freeglut.lib
   ```

3. Ensure that a compatible `freeglut.dll` is placed beside the built
   executable. For an x64 Debug build, the expected location is:

   ```text
   x64/Debug/freeglut.dll
   ```

4. Open `robot-story-game.sln` in Visual Studio.

## Build Instructions for Visual Studio

1. Open `robot-story-game.sln`.
2. Select **Debug** or **Release** as the solution configuration.
3. Select **x64** as the solution platform.
4. Choose **Build > Build Solution** or press `Ctrl+Shift+B`.
5. Run with **Debug > Start Without Debugging** or press `Ctrl+F5`.
6. Press `Space` after launch to start the animation; the scene starts paused.

The Visual Studio project is already configured to:

- add `include/` to the compiler include path;
- add `lib/` to the linker library path; and
- link against `opengl32.lib`, `glu32.lib`, and `freeglut.lib`.

## Texture Setup

The custom texture loader supports uncompressed 24-bit BMP files. If a texture
cannot be loaded, the program automatically creates a procedural fallback
texture, so the scene remains usable.

The active source currently attempts to load `robot_texture.bmp`,
`floor_texture.bmp`, and `sky_texture.bmp` from absolute paths inside
`init()`. For a portable GitHub setup, replace those paths with relative asset
paths and place the matching files in `assets/`, for example:

```cpp
robotTexture = loadTexture("assets/robot_texture.bmp");
floorTexture = loadTexture("assets/floor_texture.bmp");
skyTexture = loadTexture("assets/sky_texture.bmp");
```

The existing `assets/` directory contains museum-themed BMP files, but they are
not referenced by the active `robot-story-game.cpp` source.

## Controls

| Input | Action |
|---|---|
| `Space` | Pause or resume animation |
| `1` | Front camera view |
| `2` | Side camera view |
| `3` | Top camera view |
| `4` | Orbit camera view |
| `P` | Use perspective projection |
| `O` | Use orthographic projection |
| `F` | Toggle follow camera |
| `Z` / `X` / `C` | Follow robot 1, 2, or 3 |
| Arrow keys | Rotate the orbit camera |
| Left mouse drag | Rotate the orbit camera and switch to orbit view |
| Mouse wheel | Zoom the orbit camera |
| `W` | Toggle random walking mode |
| `I` | Toggle the special synchronized dance mode |
| `K` | Toggle story mode |
| `L` | Cycle through lighting modes |
| `T` | Toggle textures |
| `G` | Toggle the terrain grid and interaction guides |
| `M` | Toggle four-view split-screen mode |
| `A` / `D` | Rotate the robot models |
| `+` / `-` | Increase or decrease animation speed |
| `R` | Reset the scene |
| `Esc` | Exit the application |

## Project Structure

```text
robot-story-game/
|-- assets/                         # BMP texture assets
|-- include/GL/                     # FreeGLUT header files
|-- lib/
|   `-- freeglut.lib                # FreeGLUT import library
|-- robot-story-game.cpp            # Active application source
|-- robot-story-game.sln            # Visual Studio solution
|-- robot-story-game.vcxproj        # Visual Studio C++ project configuration
|-- robot-story-game.vcxproj.filters
|-- robot-story-game_before_museum_backup.cpp
`-- x64/                            # Generated x64 build output
```

`robot-story-game.cpp` contains the active scene rendering, robot modeling,
animation state machine, input handling, lighting, camera logic, and particle
system.

## Future Improvements

- Replace absolute texture paths with a portable asset-loading system
- Connect the existing museum assets to the active scene
- Split the single source file into focused rendering, animation, input, and
  scene modules
- Move from the fixed-function pipeline to modern OpenGL with shaders, VAOs,
  and VBOs
- Add an in-game help overlay and visible mode/status indicators
- Use frame-time-based animation for consistent speed across different systems
- Add audio for lightning, robot movement, and cinematic events
- Expand the story with additional interactions, objectives, and endings
- Add CMake or automated build configuration for easier setup
- Remove generated binaries and intermediate build files from version control

## Academic Context

This project demonstrates concepts studied during an OpenGL laboratory course,
including transformations, hierarchical modeling, curved surfaces, Bezier
curves, lighting and materials, textures, animation, camera systems, and
interactive real-time graphics.
