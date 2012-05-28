#include "Sierpinski.h"

#include <Camera.h>
#include <MeshTools/CompressIndices.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/Tipsify.h>

#include "ColorShader.h"
#include "SierpinskiData.h"

using namespace Magnum;
using namespace Corrade::Utility;

Sierpinski::Sierpinski(Object* parent): Object(parent), buffer(nullptr) {
    buffer = mesh.addBuffer(Mesh::BufferType::Interleaved);

    mesh.bindAttribute<ColorShader::Vertex>(buffer);
    mesh.bindAttribute<ColorShader::Color>(buffer);
}

void Sierpinski::setIterations(size_t iterations, bool tipsify) {
    Debug() << "Setting Sierpinski iterations to" << iterations;
    if(tipsify)
        Debug() << "Optimizing indices using Tipsify algorithm with cache size 24...";
    SierpinskiData data(iterations);
    MeshTools::interleave(&mesh, buffer, Buffer::Usage::StaticDraw, data.vertices, data.colors);
    if(tipsify)
        MeshTools::tipsify(data.indices, data.vertices.size(), 56);
    MeshTools::compressIndices(&mesh, Buffer::Usage::StaticDraw, data.indices);
}

void Sierpinski::draw(const Matrix4& transformationMatrix, Camera* camera) {
    shader->use();
    shader->setMatrixUniform(camera->projectionMatrix()*transformationMatrix);

    mesh.draw();
}
