/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/System.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

using namespace Magnum;

int main(int argc, char** argv) {
    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "GIF to load", "file.gif")
        .addOption("importer", "StbImageImporter")
            .setHelp("importer", "importer plugin to use")
        .setGlobalHelp("Plays back an animated GIF. In the terminal.")
        .parse(argc, argv);

    if(!Debug::isTty()) {
        Error{} << "Not running in a TTY, can't play.";
        return 1;
    }

    /* Load GIF importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate(args.value("importer"));
    if(!importer || !importer->openFile(args.value("file")))
        return 2;

    /* Get the first image, decide how many pixels to cut away to fit on 80
       chars */
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    if(!image) return 3;

    #ifndef CORRADE_NO_ASSERT
    const Vector2i imageSize = image->size();
    #endif
    const std::ptrdiff_t skip = (image->size().x() + 39)/40;
    Debug{} << Debug::color << Debug::packed
        << image->pixels<Color4ub>().every({skip, skip}).flipped<0>();

    /* Right now, until a better API for video playback is in place,
       StbImageImporter frame delays as ints in its importer state. If nothing
       is there, this is not an animation -- print just the first image and
       exit. */
    if(!importer->importerState()) return 0;

    /* Query total image count, frame delays in milliseconds */
    const Int imageCount = importer->image2DCount();
    const auto frameDelays = Containers::arrayView(
        reinterpret_cast<const Int*>(importer->importerState()), imageCount);

    /* Loop through the images, print them */
    for(Int i = 1; i != imageCount; ++i) {
        if(!(image = importer->image2D(i))) return 3;
        CORRADE_INTERNAL_ASSERT(image->size() == imageSize);

        /* Move the cursor back */
        auto pixels = image->pixels<Color4ub>().every({skip, skip}).flipped<0>();
        Utility::print("\033[{}A", pixels.size()[0]);

        /* Print another frame and wait */
        Debug{} << Debug::color << Debug::packed << pixels;
        Utility::System::sleep(frameDelays[i]);
    }
}
