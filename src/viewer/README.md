This is simply an viewer for COLLADA files. It can load scenes with one or
more objects and display them. COLLADA support is currently aimed at opening
files exported from Blender 2.6. Only triangle meshes with Phong shading
without textures are currently supported.

![Viewer](viewer.png)

Usage
-----

Pass path to COLLADA file as parameter:

    ./viewer ~/models/scene.dae

The application opens the file and displays the scene. Load progress is written
to console output. **Mouse drag** rotates the camera around the scene,
**mouse wheel** zooms in and out.
