This is simply an viewer for 3D scene files, such as OpenGEX or COLLADA ones.
Only triangle meshes with Phong shading with optional diffuse texture are
currently supported.

![Viewer](viewer.png)

Usage
-----

Pass path to an OpenGEX or COLLADA scene file as parameter:

    ./magnum-viewer scene.ogex

The application opens the file using `AnySceneImporter` which detects the file
type, delegates the import to plugin dedicated for given format (such as
`OpenGexImporter` or `ColladaImporter`) and then displays the scene. Loading
progress is written to console output. **Mouse drag** rotates the camera around
the scene, **mouse wheel** zooms in and out.

Sample OpenGEX scene is supplied alonside the source. If you install the
examples, the scene is also copied into `<prefix>/share/magnum/examples/viewer/`.
Running the example with the bundled scene can be then done like this:

    ./magnum-viewer <path-to-example-source>/scene.ogex

Documentation
-------------

This example is thoroughly explained in the documentation, which you can read
also online at http://mosra.cz/blog/magnum-doc/examples-viewer.html .
