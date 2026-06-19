# BinaxEngine - Game Engine
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
