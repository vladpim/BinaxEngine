# BinaxEngine - Game Engine (v0.8.0)
---

![BinaxEngine Editor](resources/screenshots/binaxscreenshot.png)

---
*(Developed and maintained by a 14‑year‑old developer passionate about low‑level systems and graphics)*

## Features

### Editor & Workflow
- **ImGui-based interface** with docking, viewports, and custom themes
- **ImGuizmo integration** – translate, rotate, scale objects with snap support
- **Hierarchy panel** – parent-child object relationships with drag‑and‑drop
- **Inspector panel** – edit all components (transform, appearance, light, material, mesh, physics, audio, camera)
- **Scene view** – camera navigation (WASD + mouse) with mouse capture
- **Content browser** – asset management (models, textures, shaders, audio files)
- **Menu bar** – file operations (Save Scene, Load Scene), edit (duplicate, delete), view (wireframe, grid, gizmo, theme editor, VSync), physics (active physics, reset), skybox & shadows settings
- **Hotkeys** – Ctrl+S (save current scene), Ctrl+O (load scene), Ctrl+D (duplicate), Delete (remove object), G (toggle grid), T/R/E (gizmo mode), X (gizmo local/world), ESC (release mouse)

### Graphics & Rendering
- **OpenGL 3.3 core profile** with seamless cubemap support
- **PBR materials** – metallic, roughness, ambient occlusion, emission
- **Normal mapping** with adjustable strength (0–5x)
- **Texture support** – diffuse, normal, roughness, metallic, AO (load via file dialog)
- **UV scaling** and **World UV** projection (triplanar mapping)
- **Dynamic lights** – directional, point, spot (up to 8 active)
- **Advanced shadow mapping**:
  - Dual‑mode shadows: High Quality (sharp, up to 30 m) and Long‑distance (blurry, up to 150 m)
  - Camera‑centric orthographic projection – no clipping at world origin
  - PCF filtering (4/9 samples), bias, softness, and map size – all adjustable in real time
- **Skybox** – cubemap‑based environment, seamless filtering
- **Grid** – customizable semi‑transparent grid with distance fade
- **Visual gizmos** – camera frustum (pyramid), light range (sphere/cone), audio range (sphere) – each toggleable per object

### Audio (miniaudio)
- **2D / 3D spatial audio** – distance attenuation, listener follows active camera
- **Audio Source component** – add/remove via "Add Component" menu
- **Real‑time parameter updates** – volume, loop, min/max distance, 3D toggle – without restarting playback
- **Visual audio gizmo** – white wireframe sphere showing max distance (toggleable)
- **Supports WAV, MP3, FLAC, OGG** via miniaudio
- 
### Physics (Bullet 3.25)
- **Rigid body dynamics** – mass, gravity, collisions
- **Collider shapes** – Box, Sphere, Capsule
- **Material properties** – friction (0–1), restitution (0–1), rolling friction
- **Damping** – linear and angular drag
- **Static & dynamic bodies** – mass 0 = static (e.g., ground)
- **Physics controls** – "Active Physics" toggle, "Return" reset to initial positions
- **Real‑time synchronization** – transforms updated automatically

### Asset Import (Assimp)
- **Model formats** – OBJ, FBX, DAE, BLEND, 3DS, STL
- **Full Unicode support** – Cyrillic characters in filenames and paths
- **Hierarchical import** – preserves object hierarchy, position, rotation, and scale from nodes
- **Material extraction** – loads diffuse, normal, roughness, metallic, AO textures
- **PBR parameter reading** – metallic/roughness factors from file
- **Mesh naming** – retains original mesh names

### Component System
- **Transform** – position, rotation, scale (with non‑uniform scale warning)
- **Appearance** – color, visibility
- **Light** – type, color, intensity, range, angle, direction, volumetric cone (shafts)
- **Material** – textures, metallic/roughness override, emission, UV settings, transparency mode
- **Mesh** – switch between primitives (cube, sphere, cylinder, cone, pyramid, plane) or load external model
- **Rendering** – cast/receive shadows, enable/disable per object
- **Physics** – add/remove RigidBody, choose collider type, adjust mass, friction, restitution, rolling friction, linear/angular damping
- **Audio Source** – load audio file, volume, loop, 3D toggle, min/max distance, play/stop, show gizmo
- **Camera** – FOV, near/far plane, switch active camera, show frustum gizmo

### Scene Serialization (.bxlvl)
- **Complete scene save/load** – objects, hierarchy, transforms, meshes (primitive or model), PBR materials, lights, cameras, physics bodies, audio sources, fog
- **Embedded editor settings** – VSync, snap increments, shadow configs, background color, ambient strength – all stored inside the scene file (no separate config)
- **Smart workflows** – Ctrl+S overwrites the current scene without dialogs; "Save As" and "Load" via native file picker

---

## What's New in v0.8.0

### Rendering and Dynamic Shadows
- **Adaptive Dual‑Zone Shadows** – two independent shadow modes: High Quality Mode (sharp, up to 30 m) and Long‑Distance Mode (blurry, up to 150 m), switchable via checkbox.
- **Camera‑Centric Shadow Maps** – shadow view‑frustum now follows the active camera, eliminating clipping and artifacts at the world origin.
- **Exposed Shadow Parameters** – distance, map resolution (1024–8192), PCF samples, bias, and softness are fully adjustable in the Editor UI and saved per scene.

### Advanced Material Transparency
- **Alpha Test Support** – ideal for foliage, grass, and cutout textures; pixels below a cutoff are discarded. Shadows can also use alpha testing (`Alpha Test Shadows`), so leaves cast accurate geometric shadows.
- **Alpha Blend Support** – alpha‑blending for glass, water, and translucent surfaces.
- **Material Properties** – runtime controls for `Alpha`, `Alpha Cutoff`, and `Transparent` flags in the Material Inspector.

### Robust Scene Serialization
- **Unified `.bxlvl` (JSON) Format** – scenes now store the complete world state, including hierarchy, meshes (both primitives and models), PBR materials, lights, cameras, Bullet Physics components, audio sources, and fog settings.
- **Embedded Editor Settings** – all editor preferences (VSync, snapping, shadow configs, viewport background, etc.) are now stored directly inside each `.bxlvl` scene file instead of an external global file.
- **Smarter Workflows** – native file dialogs for Load/Save; pressing **Ctrl+S** instantly overwrites the current scene without dialog popups.

### Assimp Model Loading & Material Fixes
- **Unicode Support** – fixed file path resolution to fully support Cyrillic characters in model names and directory structures.
- **Hierarchical Node Transforms** – correctly extracts and propagates local position, rotation, and scale from complex Assimp node hierarchies.
- **VRAM and Format Optimization** – resolved a critical crash caused by pushing 1‑ or 2‑channel textures (Roughness, Metallic, AO) into `GL_RGBA`. Now properly uses `GL_RED` / `GL_RG` and internal formats `GL_R8` / `GL_RG8`.
- **Material Multipliers** – added explicit uniform controls for **AO Strength**, **Roughness Strength**, and **Normal Strength** (range 0.0 – 5.0) in the PBR shader.
- **Texture Lifecycle** – added safe `Clear()` methods for all texture slots to prevent memory leaks during runtime switching.

### General Improvements
- **gui.ini** – ImGui window layout is now saved next to the executable, not in `saves/`.
- **Performance** – gizmo rendering uses precomputed geometry and batched draw calls.
- **Stability** – eliminated duplicate implementations of `SaveScene` / `LoadScene`, fixed double declaration of `mat` in `GameObject::Draw`, added missing includes.

---

## Fixes & Improvements (v0.8.0)
- Fixed shadow clipping when camera moves away from origin.
- Fixed crash when loading 1‑channel (roughness, metallic, AO) textures.
- Fixed double-declaration of `mat` in `GameObject::Draw`.
- Removed redundant `SaveScene` / `LoadScene` implementations.
- Added missing `#include <filesystem>`, `#include <nlohmann/json.hpp>`, etc.
- Fixed uniform bindings for AO and Roughness in forward shader.
- Fixed 3D audio listener orientation update.
- Moved `gui.ini` to executable directory.

---

## Installation & Build

### Prerequisites
- Windows 10/11
- Visual Studio 2022 (with C++ development tools)
- CMake 3.15+
- Git

### Dependencies (Important!)

BinaxEngine relies on several third‑party libraries. All of them **must** be placed inside the `libs/` folder of the repository with the exact directory structure expected by the CMake build script (see `CMakeLists.txt`).

You can download the required libraries from their official sources or use pre‑built binaries. The expected structure is:

```
libs/
├── glfw/          # GLFW 3.x (include + lib)
├── glew/          # GLEW 2.x (include + lib)
├── imgui/         # Dear ImGui (source files + backends + ImGuizmo)
├── glm/           # GLM (header‑only)
├── stb/           # stb_image.h (header‑only)
├── assimp/        # Assimp 5.x (include + lib)
├── bullet/        # Bullet Physics 3.x (source or prebuilt)
├── miniaudio/     # miniaudio.h (header‑only)
└── json/          # nlohmann/json (header‑only)
```

For detailed paths and library names, refer to the `CMakeLists.txt` file. If a library is not found, CMake will produce an error or warning during configuration.

### Build Steps

1. Clone the repository:
   ```bash
   git clone https://github.com/VladPim/BinaxEngine.git
   cd BinaxEngine
   ```

2. Make sure all dependencies are placed in the `libs/` folder as described above.

3. Build with CMake:
   ```bash
   mkdir build
   cd build
   cmake .. -A x64
   cmake --build . --config Release
   ```

4. Run the executable:
   ```
   build/Release/BinaxEngine.exe
   ```

---

## Editor Controls

| Action                        | Keys                                         |
|-------------------------------|----------------------------------------------|
| Move camera                   | WASD + Space (up) / Shift (down)             |
| Rotate camera                 | Mouse (after pressing "Capture Mouse")       |
| Capture / release mouse       | Button in menu bar or **ESC**                |
| Select object                 | Left‑click in Hierarchy or Scene View        |
| Delete object                 | **Delete**                                   |
| Duplicate object              | **Ctrl + D**                                 |
| Save scene (overwrite)        | **Ctrl + S**                                 |
| Load scene                    | **Ctrl + O** (or via File menu)              |
| Gizmo mode (Translate)        | **T**                                        |
| Gizmo mode (Rotate)           | **R**                                        |
| Gizmo mode (Scale)            | **E**                                        |
| Gizmo local / world toggle    | **X**                                        |
| Toggle grid                   | **G**                                        |

---

## Example: Creating a Physics Scene
1. Add a **Cube** (via `+ Add Object → Cube`) – it gets a Box collider and is static by default.
2. Add a second **Cube**, move it up (e.g., Y = 3).
3. Select the second cube, click `Add Component → Physics`.
4. Enable physics via `Physics → Active Physics`.
5. The top cube will fall onto the bottom one and stop.
6. Tweak mass, friction, restitution, and damping in the Inspector.

---

## Example: Adding 3D Sound
1. Create an empty object or select an existing one.
2. Go to `Inspector → Components → Add Component → Audio Source`.
3. Load a sound file (WAV, MP3, etc.) via the `...` button.
4. Enable **3D Sound**, set **Min Distance** (e.g., 1) and **Max Distance** (e.g., 20).
5. Press **Play** – move the camera closer/farther to hear the attenuation.
6. Enable **Show Gizmo (Range)** to visualise the max distance sphere.

---

## Project Structure
```
BinaxEngine/
├── assets/                 # shaders, textures, models
├── libs/                   # third‑party libraries (GLFW, GLEW, Bullet, Assimp, ImGui, GLM, STB, miniaudio, nlohmann/json)
├── resources/              # icons, fonts, skyboxes
├── src/
│   ├── Audio/              # miniaudio wrapper
│   ├── Editor/             # ImGui editor code
│   ├── Graphics/           # OpenGL, shaders, meshes, materials, models, primitives, skybox
│   ├── Physics/            # Bullet wrapper
│   ├── Scene/              # GameObject, SceneManager, Camera, Frustum
│   └── main.cpp            # entry point
├── CMakeLists.txt
└── README.md
```

---

## Planned for Future Releases
- Particle system
- Scripting (Lua)
- Post‑processing (bloom, HDR)
- Terrain system
- Animation (skeletal)
- More audio features (attenuation curves, Doppler effect)

---
