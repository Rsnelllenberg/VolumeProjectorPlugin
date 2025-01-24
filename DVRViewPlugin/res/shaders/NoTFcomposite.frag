#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D directions;
uniform sampler3D volumeData;

uniform float stepSize;
uniform vec3 brickSize;

void main()
{
    vec2 texSize = textureSize(directions, 0);
    vec3 volSize = textureSize(volumeData, 0);
    vec3 brickLayout = volSize / brickSize;

    vec2 normTexCoords = gl_FragCoord.xy / texSize;

    vec4 directionSample = texture(directions, normTexCoords);
    vec3 directionRay = directionSample.xyz;
    float lengthRay = directionSample.a;

    vec3 samplePos = worldPos + lengthRay * normalize(directionRay); // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    

    vec4 color = vec4(0.0);

    // Walk from back to front
    for (float t = lengthRay; t >= 0.0; t -= stepSize)
    {
        samplePos -= increment;
        vec3 volPos = samplePos / volSize;
        vec4 sampleValue = vec4(0);
//        if(brickLayout == vec3(1,1,1)){ // No bricks (less then 5 dimesnions)
            sampleValue = texture(volumeData, volPos);
//        } // BROKEN doesn' t work
//        else {
//            for(int i = 0; i < brickLayout.x; i++){
//                for(int j = 0; j < brickLayout.y; j++){
//                    for(int k = 0; k < brickLayout.z; k++){
//                        vec3 brickPos = vec3(i,j,k);
//                        sampleValue += texture(volumeData, volPos + (brickPos * brickSize));
//                    }
//                }
//            }
//            sampleValue /= brickLayout.x * brickLayout.y * brickLayout.z; //Temp average
//        }



        // TODO: Apply the transfer function
        vec4 sampleColor = sampleValue;


        // Perform alpha compositing (back to front)
        vec3 outRGB = sampleColor.a * vec3(sampleColor) + (1.0 - sampleColor.a) * vec3(color);
        float outAlpha = sampleColor.a + (1.0 - sampleColor.a) * color.a;
        color = vec4(outRGB, outAlpha);
    }
    FragColor = color;
}
