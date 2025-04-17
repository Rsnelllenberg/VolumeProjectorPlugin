#version 330
out vec4 FragColor;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler3D volumeData;

uniform sampler2D tfTexture;

uniform vec3 dimensions;
uniform vec3 invDimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invFaceTexSize; // Pre-divided FaceTexSize (1.0 / FaceTexSize)
uniform vec2 invTfTexSize;  // Pre-divided tfTexSize (1.0 / tfTexSize)

uniform float stepSize;

void main()
{
    vec2 normTexCoords = gl_FragCoord.xy * invFaceTexSize;

    
    vec3 frontFacesPos = texture(frontFaces, normTexCoords).xyz * dimensions;
    vec3 backFacesPos = texture(backFaces, normTexCoords).xyz * dimensions;

    if(frontFacesPos == backFacesPos) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 directionSample = backFacesPos - frontFacesPos; // Get the direction and length of the ray
    vec3 directionRay = normalize(directionSample);
    float lengthRay = length(directionSample);

    vec3 samplePos = frontFacesPos; // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    vec4 color = vec4(0.0);

    // Walk from front to back
    for (float t = 0.0; t <= lengthRay; t += stepSize)
    {
        vec3 volPos = samplePos * invDimensions; // Convert 3D world position to normalized volume coordinates
        vec2 sample2DPos = texture(volumeData, volPos).rg * invTfTexSize; // Convert 3D volume position to 2D texture coordinates

        vec4 sampleColor = texture(tfTexture, sample2DPos);
        sampleColor.a *= stepSize; // Compensate for the step size

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