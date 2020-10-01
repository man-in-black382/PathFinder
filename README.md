# PathFinder
An attempt to build a modern renderer using modern graphic APIs.

# Engine Architecture
Pillars of the architecture are __Bindless Resources__ and [Render Graph](https://medium.com/@pavlo.muratov/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3). 
Relying only on hardware that supports unbounded arrays of textures in shaders greatly reduces the complexity of resource binding model, which leads to less complicated code, smaller amount of root signatures and descriptor heaps. One GPU (CB, SR, UA) descriptor heap and a few root signatures are enough to cover any rendering scenario.
Render pass system enables automatic behind-the-scenes resource state tracking and optimization, resource memory aliasing, pipeline states management, command lists management and more, while providing a simple unified interface that allows user to prototype an arbitrary but efficient render pipleline fast without concentrating on low-level management.

# Graphic Pipeline Features
* Deferred rendering
* Area lighting via Linearly Transformed Cosines
* PBR Camera and Light sources
* PBR Materials: Standard metal/dielectric material model with Height-Correlated GGX for specular and Disney diffuse
* Parametric Tonemapper
* Bloom via camera's Saturation-based ISO Sensitivity
* Ray-traced shadows
* Spatiotemporal Denoising

# Showcase
[![PathFinder](https://imgur.com/iWwM3OB.png)](https://youtu.be/vrCa5Fn-EMg)

