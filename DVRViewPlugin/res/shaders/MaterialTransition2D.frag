#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D directions;
uniform sampler2D materialTexture; // index 0 is no material present (air), the tfTexture should have the same
uniform sampler2D tfTexture;
uniform sampler3D volumeData; // contains the 2D positions of the DR

uniform vec3 dimensions;

uniform float stepSize;

void main()
{
    vec2 dirTexSize = textureSize(directions, 0);
    vec2 tfTexSize = textureSize(tfTexture, 0);
    vec2 matTexSize = textureSize(materialTexture, 0);

    vec2 normTexCoords = gl_FragCoord.xy / dirTexSize;

    vec4 directionSample = texture(directions, normTexCoords);
    vec3 directionRay = directionSample.xyz;
    float lengthRay = directionSample.a;

    vec3 samplePos = worldPos; // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    vec4 color = vec4(0.0);
    float previousMaterial = 0;
    // Walk from front to back
    for (float t = 0.0; t <= lengthRay; t += stepSize)
    {
        vec3 volPos = samplePos / dimensions;
        vec2 sample2DPos = texture(volumeData, volPos).rg / tfTexSize;

        float currentMaterial = texture(tfTexture, sample2DPos).r * 255;
        vec4 sampleColor = texture(materialTexture, vec2(previousMaterial, currentMaterial) / matTexSize);

       //vec4 sampleColor = vec4(vec2(previousMaterial, currentMaterial), 0.0, currentMaterial);

        // Perform alpha compositing (front to back)
        vec3 outRGB = color.rgb + (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
        float outAlpha = color.a + (1.0 - color.a) * sampleColor.a;
        color = vec4(outRGB, outAlpha);

        // Early stopping condition
        if (color.a >= 1.0)
        {
            break;
        }

        samplePos += increment;
        previousMaterial = currentMaterial;
    }
    FragColor = color;
}