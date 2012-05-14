#ifndef Magnum_Shaders_PhongShader_h
#define Magnum_Shaders_PhongShader_h
/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

/** @file
 * @brief Class Magnum::Examples::PhongShader
 */

#include <array>

#include "AbstractShaderProgram.h"

namespace Magnum { namespace Examples {

/**
@brief Phong shader

@requires_gl33 The shader is written in GLSL 3.3, although it should be trivial
    to port it to older versions.
*/
class PhongShader: public AbstractShaderProgram {
    public:
        typedef Attribute<0, Vector4> Vertex;   /**< @brief Vertex position */
        typedef Attribute<1, Vector3> Normal;   /**< @brief Normal direction */

        static const unsigned int LightCount = 3; /**< @brief Light count */

        /** @brief Constructor */
        PhongShader();

        /**
         * @brief %Ambient color
         *
         * If not set, default value is `(0.0f, 0.0f, 0.0f)`.
         */
        inline void setAmbientColorUniform(const Vector3& color) {
            setUniform(ambientColorUniform, color);
        }

        /** @brief Diffuse color */
        inline void setDiffuseColorUniform(const Vector3& color) {
            setUniform(diffuseColorUniform, color);
        }

        /**
         * @brief Specular color
         *
         * If not set, default value is `(1.0f, 1.0f, 1.0f)`.
         */
        inline void setSpecularColorUniform(const Vector3& color) {
            setUniform(specularColorUniform, color);
        }

        /**
         * @brief Shininess
         *
         * The larger value, the harder surface (smaller specular highlight).
         * If not set, default value is `80.0f`.
         */
        inline void setShininessUniform(GLfloat shininess) {
            setUniform(shininessUniform, shininess);
        }

        /** @brief Transformation matrix */
        inline void setTransformationMatrixUniform(const Matrix4& matrix) {
            setUniform(transformationMatrixUniform, matrix);
        }

        /** @brief Projection matrix */
        inline void setProjectionMatrixUniform(const Matrix4& matrix) {
            setUniform(projectionMatrixUniform, matrix);
        }

        /**
         * @brief %Light position
         * @param id        Light ID
         * @param position  Light position
         */
        inline void setLightUniform(unsigned int id, const Vector3& position) {
            setUniform(lightUniform[id], position);
        }

        /**
         * @brief %Light color
         * @param id        Light ID
         * @param color     Light color
         */
        inline void setLightColorUniform(unsigned int id, const Vector3& color) {
            setUniform(lightColorUniform[id], color);
        }

    private:
        std::array<GLint, LightCount> lightUniform;
        std::array<GLint, LightCount> lightColorUniform;

        GLint ambientColorUniform,
            diffuseColorUniform,
            specularColorUniform,
            shininessUniform,
            transformationMatrixUniform,
            projectionMatrixUniform;
};

}}

#endif
