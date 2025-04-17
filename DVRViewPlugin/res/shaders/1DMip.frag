#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler3D volumeData;

uniform float stepSize;

uniform vec3 dimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec3 invDimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invFaceTexSize; // Pre-divided faceTexSize (1.0 / faceTexSize)

uniform float volumeMaxValue; 
uniform int chosenDim;

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

    vec3 samplePos = frontFacesPos + lengthRay * normalize(directionRay); // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    float maxVal = 0.0f;

    // Walk from back to front
    for (float t = lengthRay; t >= 0.0; t -= stepSize)
    {
        samplePos -= increment;
        vec3 volPos = samplePos * invDimensions;
        float sampleValue = texture(volumeData, volPos).r;
        maxVal = max(maxVal, sampleValue);
    }

    FragColor = vec4(vec3(maxVal / volumeMaxValue), 1.0);
}
