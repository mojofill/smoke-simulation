#version 450

layout(location = 0) in vec2 inPos;
layout(location = 0) out vec2 outNDC;
layout (binding = 0, r32f) uniform readonly image2D smokeMapInput;
layout (binding = 1, r32f) uniform readonly image2D horizontalVelocitiesInput;
layout (binding = 2, r32f) uniform readonly image2D verticalVelocitiesInput;

void main() {
    outNDC = inPos;
    gl_Position = vec4(inPos, 0.0, 1.0);
}
