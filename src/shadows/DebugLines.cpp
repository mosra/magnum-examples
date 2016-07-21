#include "DebugLines.h"
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Renderer.h>
#include "ShadowLight.h"


DebugLines::DebugLines()
:	mesh(Magnum::MeshPrimitive::Lines)
{
	mesh.addVertexBuffer(buffer, 0, Shader::Position(), Shader::Color());
}

void DebugLines::reset()
{
	lines.clear();
	buffer.invalidateData();
}

void DebugLines::draw(const Magnum::Matrix4& transformationProjectionMatrix)
{
	if (!lines.empty()) {
		Magnum::Renderer::disable(Magnum::Renderer::Feature::DepthTest);
		buffer.setData(lines, Magnum::BufferUsage::StreamDraw);
		shader.setTransformationProjectionMatrix(transformationProjectionMatrix);
		mesh.draw(shader);
		Magnum::Renderer::enable(Magnum::Renderer::Feature::DepthTest);
	}
}

void DebugLines::addFrustum(const Matrix4 &imvp, Color3 col) {

	auto worldPointsToCover = ShadowLight::getFrustumCorners(imvp, 1.0f, -1.0f);

	addLine(worldPointsToCover[0], worldPointsToCover[1], col);
	addLine(worldPointsToCover[1], worldPointsToCover[3], col);
	addLine(worldPointsToCover[3], worldPointsToCover[2], col);
	addLine(worldPointsToCover[2], worldPointsToCover[0], col);

	addLine(worldPointsToCover[0], worldPointsToCover[4], col);
	addLine(worldPointsToCover[1], worldPointsToCover[5], col);
	addLine(worldPointsToCover[2], worldPointsToCover[6], col);
	addLine(worldPointsToCover[3], worldPointsToCover[7], col);

	addLine(worldPointsToCover[4], worldPointsToCover[5], col);
	addLine(worldPointsToCover[5], worldPointsToCover[7], col);
	addLine(worldPointsToCover[7], worldPointsToCover[6], col);
	addLine(worldPointsToCover[6], worldPointsToCover[4], col);
}