# Helios

## About

Helios is an implicit surface renderer based on the common [Sphere Tracing](https://graphics.cs.illinois.edu/sites/default/files/zeno.pdf) algorithm, implemented using OpenGL 4.3 [Compute Shaders](https://www.opengl.org/wiki/Compute_Shader).

### Features

* Sphere tracing renderer with soft shadows and ambient occlusion support
* Every parameter of the rendering algorithm is exposed on the GUI to tweak the quality
* Live editing of the scene file through hot reloading of GLSL code
* Parsing of GLSL sources to expose user-declared uniforms on the GUI automatically

**NOTE:** This is still an early release and a lot of things may break. Any issue/problem report is more than welcome!

![image](http://corralx.github.io/public/projects/helios.preview.jpg "Helios")

### Usage

The **g** key can be used at any time to show or hide the GUI.
The algorithm and scene parameters can be tweaked there.

In the resources folder there are 3 files that might be of interest:
* **config.json** where some parameters might be adjusted, like the resolution or the fullscreen mode
* **raymarch_library.comp** which contains several utilities and distance functions
* **raymarch_scene.comp** which contains the definition of the **scene** function that is called by the raymarch algorithm to evaluate the distance field

A separate library file is provided to let the user easily switch between different libraries (like using the one provided by the awesome [mercury demogroup](http://mercury.sexy/hg_sdf/)).

**NOTE:** For the **Open Scene File** button on the GUI to work, you need to have a default program association set up in your desktop environment for the **.comp** extension!

### References

* https://graphics.cs.illinois.edu/sites/default/files/zeno.pdf
* http://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf
* the amazing blog of [Inigo Quilez](http://www.iquilezles.org/)
* everything on https://www.shadertoy.com/
* http://9bitscience.blogspot.it/2013/07/raymarching-distance-fields_14.html
* http://blog.hvidtfeldts.net/index.php/2011/06/distance-estimated-3d-fractals-part-i/

### Compatibility

Helios has been written since the beginning with pretty high prerequisites, requiring both a modern OpenGL implementation with compute shader and explicit uniform location support, and a C++ compiler implementing the latest draft of the Filesystem TR, planned to be released as part of the next C++17 standard. This is because it wanted to be an occasion to investigate those features and try them out first hand.

To compile it from sources you need a fairly recent compiler, being Visual Studio 2015 on Windows or GCC 5.3+/Clang 3.8+ on Linux. Note that however, Clang does not seems to have a C++ filesystem library implementation in its libc++ standard library, so the GNU libstdc++ version must be used (which is the default on Linux). Moreover the project depends on SDL2 for the window and input management, whose sources are not included in this repository, and the path must be set up through CMake.

**NOTE:** Mac OS X is not supported because neither the Apple nor the Nvidia implementations of OpenGL provide OpenGL 4.3 and compute shader support.

Helios has been known to work correctly on the latest Nvidia, AMD and Intel driver, even though the Intel implementation of compute shaders seems to be unstable and might sometimes crash your driver.

## Binaries

A precompiled Windows version can be found in the [release section](https://github.com/Corralx/helios/releases) of this GitHub repository. Note that to run it requires a 64-bit operative system, a GPU with OpenGL 4.3 support, and the [Visual C++ 2015 Redistributable](https://www.microsoft.com/en-us/download/details.aspx?id=48145).

Binaries for the 32-bit version of Windows or for any Linux distributions are not currently available.

## Future Work

* Investigate the performance implication of the over-relaxation technique presented in [Enhanced Sphere Tracing](http://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf)
* Provide the user a more complex BRDF-based shading model and reflection/refraction support
* Provide a way to debug/visualize the raymarch process (normals, iterations, ...)
* Implement a first person camera to move freely in the scene
* Re-implement antialiasing support which has been scrapped during the rewrite (either using MSAA or some postproces filter like FXAA or MLAA)

## License

It is licensed under the very permissive [MIT License](https://opensource.org/licenses/MIT).
For the complete license see the provided [LICENSE](https://github.com/Corralx/helios/blob/master/LICENSE.md) file in the root directory.

## Thanks

Several open-source third-party libraries are currently used in this project:
* [SDL2](https://www.libsdl.org/index.php) for window and input management
* [gl3w](https://github.com/skaslev/gl3w) as OpenGL extensions loader
* [glm](http://glm.g-truc.net/0.9.7/index.html) as math library
* [imgui](https://github.com/ocornut/imgui) as GUI library
* [rapidjson](http://rapidjson.org/) as JSON parser
* [glslang](https://github.com/KhronosGroup/glslang) as GLSL parser
