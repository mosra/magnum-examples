#include "PoolShader.h"

#include <Utility/Resource.h>
#include <Shader.h>

namespace Magnum { namespace Examples {

PoolShader::PoolShader() {
    Corrade::Utility::Resource rs("pool");

    /* Vertex shader */
    attachShader(Shader::fromData(Version::GL320, Shader::Type::Vertex, rs.get("PoolShader.vert")));

    /* Fragment shader */
    Shader fragmentShader(Version::GL320, Shader::Type::Fragment);
    fragmentShader.addSource("#define POOL_LIGHT_COUNT " + std::to_string(LightCount) + '\n');
    fragmentShader.addSource(rs.get("PoolShader.frag"));
    attachShader(fragmentShader);

    link();

    transformationMatrixUniform = uniformLocation("transformationMatrix");
    normalMatrixUniform = uniformLocation("normalMatrix");
    projectionMatrixUniform = uniformLocation("projectionMatrix");
    cameraDirectionUniform = uniformLocation("cameraDirection");
    waterTextureTranslationUniform = uniformLocation("waterTextureTranslation");

    for(std::size_t i = 0; i != LightCount; ++i)
        lightUniform[i] = uniformLocation("light[" + std::to_string(i) + ']');

    setUniform(uniformLocation("diffuseTexture"), DiffuseTextureLayer);
    setUniform(uniformLocation("specularTexture"), SpecularTextureLayer);
    setUniform(uniformLocation("waterTexture"), WaterTextureLayer);
}

}}
