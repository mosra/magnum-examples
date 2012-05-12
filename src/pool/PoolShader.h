#ifndef PoolShader_h
#define PoolShader_h

#include <AbstractShaderProgram.h>
#include <Texture.h>

class PoolShader: public Magnum::AbstractShaderProgram {
    public:
        static const unsigned int LightCount = 2;

        PoolShader();

        inline void setTransformationMatrixUniform(const Magnum::Matrix4& matrix) {
            setUniform(transformationMatrixUniform, matrix);
        }

        inline void setNormalMatrixUniform(const Magnum::Matrix3& matrix) {
            setUniform(normalMatrixUniform, matrix);
        }

        inline void setProjectionMatrixUniform(const Magnum::Matrix4& matrix) {
            setUniform(projectionMatrixUniform, matrix);
        }

        inline void setCameraDirectionUniform(const Magnum::Vector3& position) {
            setUniform(cameraDirectionUniform, position);
        }

        inline void setDiffuseTextureUniform(const Magnum::Texture2D* texture) {
            setUniform(diffuseTextureUniform, texture);
        }

        inline void setLightPositionUniform(unsigned int id, const Magnum::Vector3& light) {
            setUniform(lightUniform[id], light);
        }

        inline void setWaterTextureUniform(const Magnum::Texture2D* texture) {
            setUniform(waterTextureUniform, texture);
        }

        inline void setWaterTextureTranslationUniform(const Magnum::Vector2& translation) {
            setUniform(waterTextureTranslationUniform, translation);
        }

    private:
        GLint transformationMatrixUniform,
            normalMatrixUniform,
            projectionMatrixUniform,
            cameraDirectionUniform,
            diffuseTextureUniform,
            waterTextureUniform,
            waterTextureTranslationUniform;

        GLint lightUniform[LightCount];
};

#endif
