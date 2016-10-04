Shadow Map Example
------------------

This example shows an example of shadow mapping using a Parallel Split /
Cascade shadow mapping technique with a single, directional light source.

It is intended to be a basis to start including your own shadow mapping system
in your own project.

![Shadows](shadows1.png)
![Shadow Debug Camera](shadows2.png)

Keyboard Controls
-----------------

### Movement/view

* *Cursor keys* and *Page Up / Down* - Translate camera
* *Left mouse drag* - rotate the camera
* *F1* Switch to main camera
* *F2* Switch to debug camera

### Shadow configuration changes - watch the console output for changes

* *F3* Change shadow face culling mode
* *F4* Shadow map alignment - camera/static
* *F5* / *F6* - Change layer split exponent
* *F7* / *F8* - Tweak bias
* *F9* / *F10* - Change number of layers
* *F11* / *F12* - Change shadow map resolution
