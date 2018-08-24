Reducing size of example images
-------------------------------

Apply `pngcrush` to all screenshots for smaller file sizes:

    for f in $(ls *.png); do pngcrush -ow $f; done
