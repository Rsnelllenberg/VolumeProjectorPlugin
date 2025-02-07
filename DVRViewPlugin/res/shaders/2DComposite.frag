#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D directions;
uniform sampler3D volumeData;

uniform sampler2D tfTexture;

uniform float stepSize;

void main()
{
    vec2 dirTexSize = textureSize(directions, 0);
    vec2 tfTexSize = textureSize(tfTexture, 0);
    vec3 volSize = textureSize(volumeData, 0);


    vec2 normTexCoords = gl_FragCoord.xy / dirTexSize;

    vec4 directionSample = texture(directions, normTexCoords);
    vec3 directionRay = directionSample.xyz;
    float lengthRay = directionSample.a;

    vec3 samplePos = worldPos; // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    vec4 color = vec4(0.0);

    // Walk from front to back
    for (float t = 0.0; t <= lengthRay; t += stepSize)
    {
        vec3 volPos = samplePos / volSize;
        vec2 sample2DPos = texture(volumeData, volPos).rg / tfTexSize;

        vec4 sampleColor = texture(tfTexture, sample2DPos);

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
    }
    FragColor = color;
}