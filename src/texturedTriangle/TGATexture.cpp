/*
    Copyright © 2010, 2011 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "TGATexture.h"

#include "Utility/Debug.h"

using namespace std;
using namespace Corrade::Utility;
using namespace Magnum::Math;

namespace Magnum { namespace Examples {

TGATexture::TGATexture(istream& input) {
    if(!input.good()) return;

    Header header;
    input.read(reinterpret_cast<char*>(&header), sizeof(Header));

    ColorFormat colorFormat;
    InternalFormat internalFormat;

    switch(header.bpp) {
        case 24:
            colorFormat = ColorFormat::BGR;
            internalFormat = InternalFormat::RGB;
            break;
        case 32:
            colorFormat = ColorFormat::BGRA;
            internalFormat = InternalFormat::RGBA;
            break;
        default:
            Error() << "TGATexture: unsupported bits-per-pixel:" << (int) header.bpp;
            return;
    }

    size_t size = header.width*header.height*header.bpp/8;
    GLubyte* buffer = new GLubyte[size];
    input.read(reinterpret_cast<char*>(buffer), size);

    GLsizei dimensions[] = {header.width, header.height};

    setData(0, internalFormat, dimensions, colorFormat, buffer);
    delete[] buffer;
}

}}
