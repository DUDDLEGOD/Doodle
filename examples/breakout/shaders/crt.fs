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
    
    // Add subtle scanlines based on screen Y coordinates
    float scanline = sin(fragTexCoord.y * 600.0) * 0.15;
    
    // Slight vignette effect
    vec2 uv = fragTexCoord - 0.5;
    float vignette = 1.0 - dot(uv, uv) * 0.4;
    
    finalColor = (texel - vec4(scanline, scanline, scanline, 0.0)) * vignette;
}
