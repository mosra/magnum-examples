#include "ColorShader.h"

#include "Utility/Resource.h"

using namespace Corrade::Utility;
using namespace Magnum;

ColorShader::ColorShader(bool interpolated) {
    Resource rs("data");
    Shader* vertex = Shader::fromData(Shader::Vertex, rs.get(interpolated ? "ColorShader.vert" : "ColorShaderFlat.vert"));
    Shader* fragment = Shader::fromData(Shader::Fragment, rs.get(interpolated ? "ColorShader.frag" : "ColorShaderFlat.frag"));
    attachShader(vertex);
    attachShader(fragment);

    bindAttribute(Vertex::Location, "vertex");
    bindAttribute(Color::Location, "color");
    link();

    matrixUniform = uniformLocation("matrix");
}
