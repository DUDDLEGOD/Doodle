#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Fetch texture color
    vec4 texel = texture(texture0, fragTexCoord);
    
    // Add scanlines based on screen Y coordinates
    float scanline = sin(fragTexCoord.y * 800.0) * 0.08;
    
    // Slight vignette effect
    vec2 uv = fragTexCoord - 0.5;
    float vignette = 1.0 - dot(uv, uv) * 0.5;
    
    // Apply chromatic aberration
    float amount = 0.002;
    float r = texture(texture0, fragTexCoord + vec2(amount, 0.0)).r;
    float g = texel.g;
    float b = texture(texture0, fragTexCoord - vec2(amount, 0.0)).b;
    vec4 chromatic = vec4(r, g, b, texel.a);
    
    finalColor = (chromatic - vec4(scanline, scanline, scanline, 0.0)) * vignette * colDiffuse;
}
