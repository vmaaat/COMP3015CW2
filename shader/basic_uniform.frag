#version 460

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace;

out vec4 FragColor;

uniform sampler2D diffuseTex;
uniform sampler2D shadowMap;

struct Light {
    vec3 position;
    vec3 color;
};

uniform Light light1;
uniform Light light2;
uniform vec3 viewPos;

uniform bool fogEnabled;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogAmount;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}

vec3 CalcLight(Light light, vec3 norm, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 64.0);

    float ambientStrength  = 0.12;
    float diffuseStrength  = 0.9;
    float specularStrength = 0.5;

    vec3 ambient  = ambientStrength * light.color;
    vec3 diffuse  = diffuseStrength * diff * light.color;
    vec3 specular = specularStrength * spec * light.color;

    return ambient + diffuse + specular;
}

void main()
{
    vec3 albedo = texture(diffuseTex, TexCoord).rgb;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 lighting = CalcLight(light1, norm, viewDir)
                  + CalcLight(light2, norm, viewDir);

    float shadow = ShadowCalculation(FragPosLightSpace);

    lighting = mix(lighting, lighting * 0.55, shadow);
    lighting = clamp(lighting, vec3(0.0), vec3(2.0));

    vec3 finalColor = albedo * lighting;

    if (fogEnabled)
    {
        float distanceFromCamera = length(viewPos - FragPos);
        float fogFactor = 1.0 - exp(-fogDensity * distanceFromCamera);
        fogFactor = clamp(fogFactor, 0.0, 0.85);
        fogFactor *= fogAmount;

        finalColor = mix(finalColor, fogColor, fogFactor);
    }

    FragColor = vec4(finalColor, 1.0);
}