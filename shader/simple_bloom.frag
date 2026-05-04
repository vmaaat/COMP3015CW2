#version 460

in vec2 TexCoord;
out vec4 FragColor;

uniform bool bloomEnabled;

uniform sampler2D sceneTexture;

void main()
{
    vec3 color = texture(sceneTexture, TexCoord).rgb;

    // Very simple bloom-style glow:
    // bright parts are boosted slightly and blended back into the scene.
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));

    vec3 glow = vec3(0.0);

    if (bloomEnabled && brightness > 0.2)
    {
        glow = color * 0.35;
    }

    vec3 finalColor = color + glow;
    finalColor = clamp(finalColor, 0.0, 1.0);

    FragColor = vec4(finalColor, 1.0);
}