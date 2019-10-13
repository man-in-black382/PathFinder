# PathFinder
DirectX 12 renderer to eventually become real-time physically-based path tracer.

# Todo List
- [x] Implement hardware abstraction layer: an interface between renderer and graphic API.
- [x] Architect render pipeline around bindless resources
- [ ] Implement per-pass resource scheduling system
  - [x] Texture scheduling
  - [ ] Buffer scheduling
  - [ ] Pipeline state and root signature scheduling
    - [x] Root signatures
    - [x] Graphic states
    - [x] Compute states
    - [ ] Ray Tracing states
    
- [ ] Implement memory aliasing for pipeline-scheduled resources
- [ ] Implement asset system
  - [x] Mesh system
    - [x] Loading
    - [x] Integration in scene
    - [x] Transfer to GPU
  - [ ] Material system
    - [x] Loading
    - [ ] Integration in scene
    - [ ] Transfer to GPU
 - [ ] Integrate ImGUI as single render pass
 - [ ] Proccess user inputs
 - [ ] Area lights
 
