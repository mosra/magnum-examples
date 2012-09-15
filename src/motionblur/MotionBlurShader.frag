#define FrameCount 7

uniform sampler2D frame[FrameCount];

in vec2 textureCoordinate;

out vec4 color;

void main() {
    vec4 summedBlur = vec4(0.0, 0.0, 0.0, 0.0);
    for(int i = 0; i != FrameCount; ++i)
        summedBlur += texture(frame[i], textureCoordinate);

    color.rgb = summedBlur.rgb/FrameCount;
    color.a = 1.0;
}
