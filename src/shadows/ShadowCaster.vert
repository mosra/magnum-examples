uniform highp mat4 transformationMatrix;

in highp vec4 position;

void main()
{
	gl_Position = transformationMatrix * position;
}
