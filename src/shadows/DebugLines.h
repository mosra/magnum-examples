#pragma once

#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Buffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/SceneGraph/SceneGraph.h>

using namespace Magnum;

class DebugLines
{
public:
	typedef Shaders::VertexColor3D Shader;

	DebugLines();

	struct Point {
		Vector3 position;
		Color3 color;
	};
	
	void reset();

	void addLine(Point&& p0, Point&& p1) {
		lines.push_back(std::move(p0));
		lines.push_back(std::move(p1));
	}

	void addLine(Vector3 p0, Vector3 p1, Color3 col) {
		addLine({p0,col},{p1,col});
	}

	void addFrustum(const Matrix4& imvp, Color3 col);

    void draw(const Magnum::Matrix4& transformationProjectionMatrix);

protected:

    std::vector<Point> lines;
	Magnum::Buffer buffer;
	Magnum::Mesh mesh;
	Shader shader;
};

