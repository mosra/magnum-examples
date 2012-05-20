This application demonstrates use of Magnum for rendering 3D models through
JNI interface from Java application and also the other way around - using Java
plugin from Magnum for importing Stanford files.

![Java viewer](https://github.com/mosra/magnum-examples/raw/jni/src/java-viewer/screenshot.png)

Dependencies
------------

In addition to libraries and tools required by other examples, these are needed:

 * **CMake** >= 2.8.8, older versions don't have support for Java and JNI
 * **Java 6** with JNI headers installed
 * **Magnum plugins**, branch `jni`, from https://github.com/mosra/magnum-plugins

Usage
-----

After compilation run the JAR file from terminal:

    java -Djava.library.path=. -jar JNativeCanvas.jar

You can either open COLLADA or Stanford PLY files and change light colors. The
model can be rotated using **mouse drag** and zoomed using **mouse wheel**.
