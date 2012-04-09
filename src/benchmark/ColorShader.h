#ifndef ColorShader_h
#define ColorShader_h

#include <AbstractShaderProgram.h>

class ColorShader: public Magnum::AbstractShaderProgram {
    public:
        typedef Attribute<0, Magnum::Vector4> Vertex;
        typedef Attribute<1, Magnum::Vector3> Color;

        ColorShader(bool interpolated);

        void setMatrixUniform(const Magnum::Matrix4& matrix) {
            setUniform(matrixUniform, matrix);
        }

    private:
        GLint matrixUniform;
};

#endif
