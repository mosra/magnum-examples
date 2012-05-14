#include "PoolShader.h"

#include <sstream>
#include <Utility/Resource.h>
#include <Shader.h>

using namespace std;
using namespace Corrade::Utility;
using namespace Magnum;

PoolShader::PoolShader() {
    Resource rs("pool");
    ostringstream ss;
    ss << LightCount;

    /* Vertex shader */
    attachShader(Shader::fromData(Shader::Type::Vertex, rs.get("PoolShader.vert")));

    /* Fragment shader */
    Shader fragmentShader(Shader::Type::Fragment);
    fragmentShader.addSource("#version 150\n#define POOL_LIGHT_COUNT " + ss.str() + '\n');
    fragmentShader.addSource(rs.get("PoolShader.frag"));
    attachShader(fragmentShader);

    link();

    transformationMatrixUniform = uniformLocation("transformationMatrix");
    normalMatrixUniform = uniformLocation("normalMatrix");
    projectionMatrixUniform = uniformLocation("projectionMatrix");
    cameraDirectionUniform = uniformLocation("cameraDirection");
    diffuseTextureUniform = uniformLocation("diffuseTexture");
    specularTextureUniform = uniformLocation("specularTexture");
    waterTextureUniform = uniformLocation("waterTexture");
    waterTextureTranslationUniform = uniformLocation("waterTextureTranslation");

    for(unsigned int i = 0; i != LightCount; ++i) {
        ss.str("");
        ss << "light[" << i << ']';
        lightUniform[i] = uniformLocation(ss.str());
    }
}
