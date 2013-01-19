#ifndef Magnum_Examples_PoolShader_h
#define Magnum_Examples_PoolShader_h

#include <Math/Matrix4.h>
#include <Math/Matrix3.h>
#include <AbstractShaderProgram.h>

namespace Magnum { namespace Examples {

class PoolShader: public Magnum::AbstractShaderProgram {
    public:
        enum: std::size_t {
            LightCount = 2
        };

        enum: GLint {
            DiffuseTextureLayer = 0,
            SpecularTextureLayer = 1,
            WaterTextureLayer = 2
        };

        PoolShader();

        inline PoolShader* setTransformationMatrix(const Matrix4& matrix) {
            setUniform(transformationMatrixUniform, matrix);
            return this;
        }

        inline PoolShader* setNormalMatrix(const Matrix3& matrix) {
            setUniform(normalMatrixUniform, matrix);
            return this;
        }

        inline PoolShader* setProjectionMatrix(const Matrix4& matrix) {
            setUniform(projectionMatrixUniform, matrix);
            return this;
        }

        inline PoolShader* setCameraDirection(const Vector3& position) {
            setUniform(cameraDirectionUniform, position);
            return this;
        }

        inline PoolShader* setLightPosition(std::size_t id, const Vector3& light) {
            setUniform(lightUniform[id], light);
            return this;
        }

        inline PoolShader* setWaterTextureTranslation(const Vector2& translation) {
            setUniform(waterTextureTranslationUniform, translation);
            return this;
        }

    private:
        GLint transformationMatrixUniform,
            normalMatrixUniform,
            projectionMatrixUniform,
            cameraDirectionUniform,
            waterTextureTranslationUniform;

        GLint lightUniform[LightCount];
};

}}

#endif
