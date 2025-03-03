#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D directions;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air), the tfTexture should have the same
uniform sampler2D tfTexture;
uniform sampler3D volumeData; // contains the 2D positions of the DR

uniform vec3 dimensions;

uniform float stepSize;
uniform vec3 camPos; 
uniform vec3 lightPos;

uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

uniform bool useShading;

float getMaterialID(vec3 volumePos, vec2 tfTexSize) {
    vec2 sample2DPos = texture(volumeData, volumePos).rg / tfTexSize;
    return texture(tfTexture, sample2DPos).r + 0.5f;
}

vec3 findSurfacePos(vec3 startPos, vec3 direction, float currentMaterial, vec2 tfTexSize, int iterations) {
    vec3 lowPos = startPos;
    vec3 highPos = startPos + direction;
    float epsilon = 0.01; // Small value to handle precision issues

    for (int i = 0; i < iterations; ++i) 
    {
        vec3 midPos = ((lowPos + highPos) * 0.5);
        float midMaterial = getMaterialID(midPos / dimensions, tfTexSize);

        // Check if the material transition is detected
        if (abs(midMaterial - currentMaterial) > epsilon)
        {
            highPos = midPos;
        }
        else
        {
            lowPos = midPos;
        }

        // Early exit if the positions are very close
        if (length(highPos - lowPos) < epsilon)
        {
            break;
        }
    }
    return (lowPos + highPos) * 0.5;
}

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
    vec3 previousPos = samplePos;
    // Walk from front to back
    for (float t = 0.0; t <= lengthRay; t += stepSize)
    {
        vec3 volPos = samplePos / dimensions;
        float currentMaterial = getMaterialID(volPos, tfTexSize); 
        vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) / matTexSize);
        
        // If we have a surface, add shading to it by finding its normal
        if(useShading && previousMaterial != currentMaterial){
            // Use a bisection method to find the accurate surface position
            int iterations = 10;
            vec3 surfacePos = findSurfacePos(previousPos, increment, previousMaterial, tfTexSize, iterations);

            // check if a clipping plane was hit
            vec3 previousPos = samplePos.xyz - (increment + 0.001f);
            vec3 minfaces = 1.0 + sign((u_minClippingPlane * dimensions) - previousPos);
            vec3 maxfaces = 1.0 + sign(previousPos - (u_maxClippingPlane * dimensions));

            // compute the surface normal (eventually normalize later)
            vec3 surfaceGradient = maxfaces - minfaces;

            vec3 normal = -normalize(directionRay);

            // if on clipping plane calculate color and return it
            if (!all(equal(surfaceGradient, vec3(0).xyz))) {
                normal = normalize(surfaceGradient);
            } else {

                // Find a surface triangle by finding two other closeby points on the surface
                vec3 upDirection = normalize(cross(directionRay, vec3(1.0, 0.0, 0.0)));
                vec3 leftDirection = normalize(cross(directionRay, vec3(0.0, 1.0, 0.0)));

                float offsetLength = 0.1;

                // Define the offset positions
                vec3 horizontalOffsetPos = previousPos + leftDirection * offsetLength;
                vec3 verticalOffsetPos = previousPos + upDirection * offsetLength;
            
                // Check if the offset positions do not yield the current material
                if (getMaterialID(horizontalOffsetPos / dimensions, tfTexSize) != currentMaterial) {
                    horizontalOffsetPos = previousPos - leftDirection * offsetLength;
                }
                if (getMaterialID(verticalOffsetPos / dimensions, tfTexSize) != currentMaterial) {
                    verticalOffsetPos = previousPos - upDirection * offsetLength;
                }


                vec3 verticalPos = findSurfacePos(verticalOffsetPos, increment * 2, previousMaterial, tfTexSize, iterations + 1);
                vec3 horizontalPos = findSurfacePos(horizontalOffsetPos, increment * 2, previousMaterial, tfTexSize, iterations + 1);

                // Calculate the two possible normals of the surface
                vec3 normal1 = normalize(cross(verticalPos - surfacePos, horizontalPos - surfacePos));
                vec3 normal2 = -normal1;
                normal = dot(normal1, normalize(directionRay)) < dot(normal2, normalize(directionRay)) ? normal1 : normal2;
            }

            // Phong shading calculations
            vec3 ambient = 0.1 * sampleColor.rgb; // Ambient component

            vec3 lightDir = normalize(lightPos - surfacePos);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * sampleColor.rgb; // Diffuse component

            vec3 viewDir = normalize(camPos - surfacePos);
            vec3 reflectDir = reflect(-lightDir, normal);
            float specular = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);

//            vec3 phongColor = vec3(diff);
            vec3 phongColor = ambient + diffuse + specular;
            sampleColor.rgb = phongColor;
        }
        // Perform alpha compositing (front to back)
        vec3 outRGB = color.rgb + (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
        float outAlpha = color.a + (1.0 - color.a) * sampleColor.a;
        color = vec4(outRGB, outAlpha);

        // Early stopping condition
        if (color.a >= 1.0)
        {
            break;
        }

        previousPos = samplePos;
        samplePos += increment;
        previousMaterial = currentMaterial;
    }
    FragColor = color;
}
