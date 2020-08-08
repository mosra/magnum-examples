/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Janos Meny <janos.meny@gmail.com>

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

#include "DeferredGeometryShader.h"

#include <Corrade/Utility/Resource.h>
#include <Corrade/Containers/Reference.h>

#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Context.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Color.h>

namespace Magnum { namespace Examples {

DeferredGeometryShader::DeferredGeometryShader() {
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    const Utility::Resource rs{"ssao-data"};

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    vert.addSource(rs.get("DeferredGeometry.vert"));

    frag.addSource(rs.get("DeferredGeometry.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    bindAttributeLocation(Position::Location, "position");
    bindAttributeLocation(Normal::Location, "normal");

    bindFragmentDataLocation(AlbedoOutput, "color");
    bindFragmentDataLocation(PositionsOutput, "position");
    bindFragmentDataLocation(NormalsOutput, "normal");

    _transformationUniform = uniformLocation("transformation");
    _normalMatrixUniform = uniformLocation("normalMatrix");
    _projectionUniform = uniformLocation("projection");
    _diffuseColorUniform = uniformLocation("diffuseColor");
}

DeferredGeometryShader& DeferredGeometryShader::setTransformation(const Matrix4& transformation){
    setUniform(_transformationUniform, transformation);
    return *this;
}

DeferredGeometryShader& DeferredGeometryShader::setNormalMatrix(const Matrix3x3& normalMatrix){
   setUniform(_normalMatrixUniform, normalMatrix);
   return *this;
}

DeferredGeometryShader& DeferredGeometryShader::setProjection(const Matrix4& projection) {
    setUniform(_projectionUniform, projection);
    return *this;
}

DeferredGeometryShader& DeferredGeometryShader::setDiffuseColor(const Color4 &color) {
    setUniform(_diffuseColorUniform, color);
    return *this;
}

}}