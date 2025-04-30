#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler3D volumeData;

uniform vec3 dimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec3 invDimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invFaceTexSize; // Pre-divided dirTexSize (1.0 / dirTexSize)
uniform vec2 invTfTexSize;  // Pre-divided tfTexSize (1.0 / tfTexSize)

uniform float stepSize;

void main()
{
    vec2 normTexCoords = gl_FragCoord.xy * invFaceTexSize;

    vec3 frontFacesPos = texture(frontFaces, normTexCoords).xyz * dimensions; // Get the front face position
    vec3 backFacesPos = texture(backFaces, normTexCoords).xyz * dimensions; // Get the back face position

    if(frontFacesPos == backFacesPos) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 directionSample = (backFacesPos - frontFacesPos); // Get the direction and length of the ray
    vec3 directionRay = normalize(directionSample);
    float lengthRay = length(directionSample);

    vec3 samplePos = frontFacesPos; // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    vec4 color = vec4(0.0);

    // Walk from front to back
    for (float t = 0.0; t <= lengthRay; t += stepSize)
    {
        vec3 volPos = samplePos * invDimensions;
        vec4 sampleColor = texture(volumeData, volPos);
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
