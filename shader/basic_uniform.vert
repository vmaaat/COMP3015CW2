#version 460

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec2 VertexTexCoord;
layout (location = 2) in vec3 VertexNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    vec4 worldPos = model * vec4(VertexPosition, 1.0);

    FragPos = worldPos.xyz;
    Normal = normalize(mat3(transpose(inverse(model))) * VertexNormal);
    TexCoord = VertexTexCoord;
    FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}