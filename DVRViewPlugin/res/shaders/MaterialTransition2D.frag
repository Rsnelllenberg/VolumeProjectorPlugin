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

float getMaterialID(vec2 tfTexSize, float[5] materials, vec3[5] samplePositions, int i, int maxI) {
    float firstMaterial = materials[min(i + 1, maxI) % 5];
    float previousMaterial = materials[min(i + 2, maxI) % 5];

    float currentMaterial = materials[min(i + 3, maxI) % 5];

    float nextMaterial = materials[min(i + 4, maxI) % 5];
    float lastMaterial = materials[min(i + 5, maxI) % 5];

    if(firstMaterial == previousMaterial && nextMaterial == lastMaterial && currentMaterial != previousMaterial && currentMaterial != nextMaterial){
        vec3 samplePos = round(samplePositions[min(i + 3, maxI) % 5] + vec3(0.5f)) - vec3(0.5f); // Sample the nearest voxel center instead
        vec2 sample2DPos = texture(volumeData, samplePos / dimensions).rg / tfTexSize;
        currentMaterial = texture(tfTexture, sample2DPos).r + 0.5f;

        // Update the materials array with the new material
        materials[min(i + 3, maxI) % 5] = currentMaterial;
        samplePositions[min(i + 3, maxI) % 5] = samplePos;
    }

    return currentMaterial;
}

vec3 findSurfacePos(vec3 startPos, vec3 direction, float currentMaterial, vec2 tfTexSize, int iterations, float[5] materials) {
    vec3 lowPos = startPos;
    vec3 highPos = startPos + direction;
    float epsilon = 0.01; // Small value to handle precision issues
    bool foundSurface = false;
    for (int i = 0; i < iterations; ++i) 
    {
        vec3 midPos = ((lowPos + highPos) * 0.5);
        vec2 sample2DPos = texture(volumeData, midPos / dimensions).rg / tfTexSize; 
        float midMaterial = texture(tfTexture, sample2DPos).r + 0.5f;
        // Check if the material transition is detected
        if (abs(midMaterial - currentMaterial) > epsilon)
        {
            highPos = midPos;
            foundSurface = true;
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
    if (!foundSurface)
    {
        return vec3(-1);
    }
    return (lowPos + highPos) * 0.5;
}

vec4 applyShading(vec3 previousPos, vec3 increment, vec3 directionRay, vec2 tfTexSize, vec4 sampleColor, float previousMaterial, vec3 surfacePos, int iterations, float[5] materials) {
    // check if a clipping plane was hit
    vec3 minfaces = 1.0 + sign((u_minClippingPlane * dimensions) - previousPos);
    vec3 maxfaces = 1.0 + sign(previousPos - (u_maxClippingPlane * dimensions));

    // compute the surface normal (eventually normalize later)
    vec3 surfaceGradient = maxfaces - minfaces;

    vec3 normal;

    // if on clipping plane calculate color and return it
    if (!all(equal(surfaceGradient, vec3(0).xyz))) {
        normal = normalize(surfaceGradient);
    } else {
        // Find a surface triangle by finding three other closeby points on the surface
        vec3 upDirection = normalize(cross(directionRay, vec3(0.0, 1.0, 0.0)));
        vec3 bottomLeftDirection = normalize(cross(directionRay, vec3(-1.0, -1.0, 0.0)));
        vec3 bottomRightDirection = normalize(cross(directionRay, vec3(1.0, 1.0, 0.0)));

        float offsetLength = 0.3;

        // Define the offset positions
        vec3 upOffsetPos = previousPos - increment + upDirection * offsetLength;
        vec3 bottomLeftOffsetPos = previousPos - increment + bottomLeftDirection * offsetLength;
        vec3 bottomRightOffsetPos = previousPos - increment + bottomRightDirection * offsetLength;

        vec3 upPos = findSurfacePos(upOffsetPos, increment * 4, previousMaterial, tfTexSize, iterations + 2, materials);
        vec3 bottomLeftPos = findSurfacePos(bottomLeftOffsetPos, increment * 4, previousMaterial, tfTexSize, iterations + 2, materials);
        vec3 bottomRightPos = findSurfacePos(bottomRightOffsetPos, increment * 4, previousMaterial, tfTexSize, iterations + 2, materials);

        // Check if any of the positions are vec3(-1)
        int invalidCount = 0;
        if (upPos == vec3(-1)) {
            upPos = surfacePos;
            invalidCount++;
        }
        if (bottomLeftPos == vec3(-1)) {
            bottomLeftPos = surfacePos;
            invalidCount++;
        }
        if (bottomRightPos == vec3(-1)) {
            bottomRightPos = surfacePos;
            invalidCount++;
        }

        // Skip shading if 2 or more positions are invalid
        if (invalidCount < 2) {
            // Calculate the two possible normals of the surface
            vec3 normal1 = normalize(cross(bottomLeftPos - bottomRightPos, upPos - bottomRightPos));
            vec3 normal2 = -normal1;
            normal = dot(normal1, normalize(directionRay)) < dot(normal2, normalize(directionRay)) ? normal1 : normal2;

            // Phong shading calculations
            vec3 ambient = 0.1 * sampleColor.rgb; // Ambient component

            vec3 lightDir = normalize(lightPos - surfacePos);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * sampleColor.rgb; // Diffuse component

            vec3 viewDir = normalize(camPos - surfacePos);
            vec3 reflectDir = reflect(-lightDir, normal);
            float specular = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);

            vec3 phongColor = ambient + diffuse + specular;
            sampleColor.rgb = phongColor;
        }
    }
    return sampleColor;
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

    float[5] materials = float[5](0.0, 0.0, 0.0, 0.0, 0.0);
    vec3[5] samplePositions = vec3[5](samplePos, samplePos, samplePos, samplePos, samplePos);

    int i = 0;
    int maxI = 5;
    float t = 0.0;
    // Walk from front to back
    while (t <= lengthRay + 2 * stepSize)
    {
        vec2 sample2DPos = texture(volumeData, samplePos / dimensions).rg / tfTexSize; 
        float newMaterial = texture(tfTexture, sample2DPos).r + 0.5f;
        
        materials[i % 5] = newMaterial;
        samplePositions[i % 5] = samplePos;

        float currentMaterial = getMaterialID(tfTexSize, materials, samplePositions, i, maxI);

        vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) / matTexSize);
         
        // If we have a surface, add shading to it by finding its normal
        if(useShading && previousMaterial != currentMaterial && sampleColor.a > 0.01){
            // Use a bisection method to find the accurate surface position
            int iterations = 10;
            vec3 previousPos = samplePositions[min(i + 2, maxI) % 5];
            vec3 surfacePos = findSurfacePos(previousPos, increment, previousMaterial, tfTexSize, iterations, materials);
            sampleColor = applyShading(previousPos, increment, directionRay, tfTexSize, sampleColor, previousMaterial, surfacePos, iterations, materials);
        } else if (previousMaterial == currentMaterial) {   
            sampleColor.a *= stepSize; // Compensate for the step size 
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

        samplePos += increment;
        previousMaterial = currentMaterial;
        t += stepSize;
        i += 1;
        if (t <= lengthRay){
            maxI += 1;
        }
    }
    FragColor = color;
}
