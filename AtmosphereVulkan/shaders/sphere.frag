
#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(set = 0, binding = 1) uniform sampler2D earthTex;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 objectColor = texture(earthTex, fragUV).rgb;

    // Surface normal
    vec3 normal = normalize(fragNormal);
    
    // Direction toward the light
    vec3 lightDirection = normalize(vec3(0.0, 1.0, 1.0));

    // Direction toward the camera (view space origin)
    vec3 viewDirection = normalize(-fragPosition);

    // Ambient lighting
    float ambientStrength = 0.15;
    vec3 ambient = objectColor * ambientStrength;

    // Diffuse lighting
    float diffuseIntensity = max(
        dot(normal, lightDirection),
        0.0
    );

    vec3 diffuse = objectColor * diffuseIntensity;

    // Specular lighting
    vec3 reflectionDirection = reflect(-lightDirection, normal);

    float shininess = 32.0;

    float specularIntensity = pow(
        max(dot(viewDirection, reflectionDirection), 0.0),
        shininess
    );

    vec3 specular = vec3(0.25) * specularIntensity; // Subtle ocean/surface highlight

    // Combine lighting
    vec3 finalColor = ambient + diffuse + specular;

    outColor = vec4(finalColor, 1.0);;
}
