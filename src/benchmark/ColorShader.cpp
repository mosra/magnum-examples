#include "ColorShader.h"

#include "Utility/Resource.h"

using namespace Corrade::Utility;
using namespace Magnum;

ColorShader::ColorShader(bool interpolated) {
    Resource rs("data");
    attachShader(Shader::fromData(Shader::Vertex, rs.get(interpolated ? "ColorShader.vert" : "ColorShaderFlat.vert")));
    attachShader(Shader::fromData(Shader::Fragment, rs.get(interpolated ? "ColorShader.frag" : "ColorShaderFlat.frag")));

    bindAttribute(Vertex::Location, "vertex");
    bindAttribute(Color::Location, "color");
    link();

    matrixUniform = uniformLocation("matrix");
}
