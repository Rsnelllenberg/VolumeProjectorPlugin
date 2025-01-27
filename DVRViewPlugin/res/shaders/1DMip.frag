#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D directions;
uniform sampler3D volumeData;

uniform float stepSize;
uniform ivec3 brickSize;
uniform float volumeMaxValue; 
uniform int chosenDim;

void main()
{
    vec2 texSize = textureSize(directions, 0);
    ivec3 volSize = textureSize(volumeData, 0);
    ivec3 brickLayout = volSize / brickSize;

    vec2 normTexCoords = gl_FragCoord.xy / texSize;

    vec4 directionSample = texture(directions, normTexCoords);
    vec3 directionRay = directionSample.xyz;
    float lengthRay = directionSample.a;

    vec3 samplePos = worldPos + lengthRay * normalize(directionRay); // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    float maxVal = 0.0f;

    // Walk from back to front
    for (float t = lengthRay; t >= 0.0; t -= stepSize)
    {
//        samplePos -= increment;
//        vec3 volPos = samplePos / volSize;
//        vec4 sampleValue = vec4(0);
//        int indexInBrick = chosenDim % 4;
//        int brickIndex = (chosenDim - indexInBrick) / 4;
//        if(brickLayout == vec3(1,1,1)){ // No bricks (less then 5 dimesnions)
//            sampleValue = texture(volumeData, volPos);
//        } 
//        else {
//            vec3 brickPos = vec3(brickIndex % volSize.x, (brickIndex / volSize.x) % volSize.y, brickIndex / (volSize.x * volSize.y));
//            sampleValue = texture(volumeData, volPos + (brickPos * brickSize));
//        }
//        maxVal = max(maxVal, sampleValue[indexInBrick]);

        samplePos -= increment;
        vec3 volPos = samplePos / volSize;
        float sampleValue = texture(volumeData, volPos).r;
        maxVal = max(maxVal, sampleValue);
    }

    FragColor = vec4(vec3(maxVal / volumeMaxValue), 1.0);
}
