# PathFinder
An attempt to build a modern renderer using modern graphic APIs.

# Engine Architecture
Pillars of the architecture are __Bindless Resources__ and __Render Graph__. 
Relying only on hardware that supports unbounded arrays of textures in shaders greatly reduces the complexity of resource binding model, which leads to less complicated code, smaller amount of root signatures and descriptor heaps. One GPU (CB, SR, UA) descriptor heap and a few root signatures are enough to cover any rendering scenario.
Render pass system enables automatic behind-the-scenes resource state tracking and optimization, resource memory aliasing, pipeline states management, command lists management and more, while providing a simple unified interface that allows user to prototype an arbitrary but efficient render pipleline fast without concentrating on low-level management.

# Graphic Pipleline
At the moment it's a basic deferred renderer with a tone mapper and 1 type of PBR material: Cook-Torrance with GGX for specular and Lambertian diffuse. 
