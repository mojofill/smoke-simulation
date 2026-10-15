#version 450

layout(location = 0) in vec2 outNDC;
layout(location = 0) out vec4 outColor;
// ping
layout (binding = 0, rgba8) uniform readonly image2D smokeMapInput;
layout (binding = 1, r32f)  uniform readonly image2D horizontalVelocitiesInput;
layout (binding = 2, r32f)  uniform readonly image2D verticalVelocitiesInput;
layout (binding = 3, r32f)  uniform readonly image2D pressureMapInput;
layout (binding = 4, r32f)  uniform readonly image2D solidMapInput; // technically dont need ping pong for solid map, since unchanging

// pong
layout (binding = 5, rgba8) uniform writeonly image2D smokeMapOutput;
layout (binding = 6, r32f)  uniform writeonly image2D horizontalVelocitiesOutput;
layout (binding = 7, r32f)  uniform writeonly image2D verticalVelocitiesOutput;
layout (binding = 8, r32f)  uniform writeonly image2D pressureMapOutput;
layout (binding = 9, r32f)  uniform writeonly image2D solidMapOutput;

// ubo
layout (binding = 10) uniform UniformBufferObject {
    int width;
    int height;
    float density;
    float dt;
    float weightSOR;
};

float calculateVelocityDivergenceAtCell(int x, int y) {
    float velocityTop = imageLoad(verticalVelocitiesInput, ivec2(x, y + 1)).x;
    float velocityLeft = imageLoad(horizontalVelocitiesInput, ivec2(x, y)).x;
    float velocityRight = imageLoad(horizontalVelocitiesInput, ivec2(x + 1, y)).x;
    float velocityBottom = imageLoad(verticalVelocitiesInput, ivec2(x, y)).x;

    float gradientX = velocityRight - velocityLeft; // finite difference approximation
    float gradientY = velocityTop - velocityBottom;

    float divergence = gradientX + gradientY;
    return divergence;
}

void main() {
    vec2 uv = outNDC * 0.5 + 0.5;
    ivec2 imgSize = imageSize(smokeMapInput);
    ivec2 texCoord = ivec2(uv * vec2(imgSize));

    float pressure = imageLoad(pressureMapInput, texCoord).x;
    float divergence = calculateVelocityDivergenceAtCell(texCoord.x, texCoord.y);
    vec4 blue = vec4(0.0, 0.0, 1.0, 1.0);
    vec4 red = vec4(1.0, 0.0, 0.0, 1.0);
    vec4 green = vec4(0.0, 1.0, 0.0, 1.0);
    vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
    vec4 pressureColor = vec4(0);
    vec4 divergenceColor = vec4(0);
    float k = 1;
    if (pressure < 0) {
        pressureColor = -divergence * blue * k;
    }
    else if (pressure >= 0) {
        pressureColor = divergence * red * k;
    }

    k = 0.00001;
    if (divergence < 0) {
        divergenceColor = -pressure * green * k;
    }
    else {
        divergenceColor = pressure * yellow * k;
    }
    
    // oh well. it PROBABLY works i just dont know

    vec4 smokeColor = imageLoad(smokeMapInput, texCoord);

    // for just smoke colors, leave this line
    outColor = vec4(smokeColor);
    // for pressure and divergence shading, uncomment this line and comment the line above
    outColor = mix(pressureColor, divergenceColor, 0.5);
    outColor = 2 * mix(smokeColor, pressureColor, 0.5);
    if (texCoord.x == width - 1 || texCoord.y == height - 1) outColor = vec4(0);
}
