#include "PoolShader.h"

#include <Utility/Resource.h>
#include <Shader.h>

using namespace Corrade::Utility;
using namespace Magnum;

PoolShader::PoolShader() {
    Resource rs("pool");

    Shader* vertexShader = Shader::fromData(Shader::Vertex, rs.get("PoolShader.vert"));
    Shader* fragmentShader = Shader::fromData(Shader::Fragment, rs.get("PoolShader.frag"));
    attachShader(vertexShader);
    attachShader(fragmentShader);

    link();
    delete vertexShader;
    delete fragmentShader;

    transformationMatrixUniform = uniformLocation("transformationMatrix");
    projectionMatrixUniform = uniformLocation("projectionMatrix");
    cameraDirectionUniform = uniformLocation("cameraDirection");
    diffuseTextureUniform = uniformLocation("diffuseTexture");
    lightUniform = uniformLocation("light");
}
