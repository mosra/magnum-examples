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
    attachShader(Shader::fromData(Shader::Vertex, rs.get("PoolShader.vert")));

    /* Fragment shader */
    Shader fragmentShader(Shader::Fragment);
    fragmentShader.addSource("#define POOL_LIGHT_COUNT " + ss.str() + '\n');
    fragmentShader.addSource(rs.get("PoolShader.frag"));
    attachShader(fragmentShader);

    link();

    transformationMatrixUniform = uniformLocation("transformationMatrix");
    projectionMatrixUniform = uniformLocation("projectionMatrix");
    cameraDirectionUniform = uniformLocation("cameraDirection");
    diffuseTextureUniform = uniformLocation("diffuseTexture");
    waterTextureUniform = uniformLocation("waterTexture");
    waterTextureTranslationUniform = uniformLocation("waterTextureTranslation");

    for(unsigned int i = 0; i != LightCount; ++i) {
        ss.str("");
        ss << "light[" << i << ']';
        lightUniform[i] = uniformLocation(ss.str());
    }
}
