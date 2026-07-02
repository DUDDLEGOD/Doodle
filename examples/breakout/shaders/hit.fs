#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float time;
uniform vec2 resolution;
out vec4 finalColor;

// https://fgarlin.com/blog/gpu-rng/
float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    float tx = fragTexCoord.x;
    float ty = fragTexCoord.y;

    float r, g, b;
    float dx, dy;
    vec2 uv;

    // R Channel: Shifted by high-speed horizontal, slow vertical
    dx = (rand(vec2(floor(ty / 0.1), floor(time * 11.0))) - 0.5) * 0.05;
    dy = (rand(vec2(floor(tx / 0.1), floor(time * 3.0))) - 0.5) * 0.04 * 6.875;
    uv = vec2(tx + dx, 1.0 - ty + dy);
    if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0) {
        r = texture(texture0, uv).x;
    } else {
        r = 0.0;
    }

    // G Channel: Shifted by slow horizontal, high-speed vertical
    dx = (rand(vec2(floor(ty / 0.1), floor(time * 3.0))) - 0.5) * 0.05;
    dy = (rand(vec2(floor(tx / 0.1), floor(time * 11.0))) - 0.5) * 0.04 * 6.875;
    uv = vec2(tx + dx, 1.0 - ty + dy);
    if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0) {
        g = texture(texture0, uv).y;
    } else {
        g = 0.0;
    }

    // B Channel: Shifted by medium-speed horizontal, medium-speed vertical
    dx = (rand(vec2(floor(ty / 0.1), floor(time * 6.0))) - 0.5) * 0.05;
    dy = (rand(vec2(floor(tx / 0.1), floor(time * 7.0))) - 0.5) * 0.04 * 6.875;
    uv = vec2(tx + dx, 1.0 - ty + dy);
    if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0) {
        b = texture(texture0, uv).z;
    } else {
        b = 0.0;
    }

    // Set final color using max of RGB for alpha, multiplied by fragColor to keep the blue base styling
    finalColor = vec4(r, g, b, max(max(r, g), b) * fragColor.a) * fragColor;
}
