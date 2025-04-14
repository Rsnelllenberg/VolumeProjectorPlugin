#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air), the tfTexture should have the same
uniform sampler3D volumeData; // contains the Material IDs of the DR

uniform vec3 dimensions;

uniform float stepSize;
uniform vec3 camPos; 
uniform vec3 lightPos;

uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

uniform bool useShading;

float getMaterialID(vec3 volumePos, float[5] materials, float[5] pointCenterDistances) {
    float material = texture(volumeData, volumePos).r; //material ID using NN
    if(useShading){

        materials[2] = material;

        vec3 samplePos = volumePos * dimensions;
        pointCenterDistances[2] = length(floor(samplePos) + 0.5f - samplePos);

        // Separate materials into groups
        vec3 groupValues = vec3(0.0);
        int start = (materials[0] == materials[1]) ? 0 : 1;
        int end = (materials[3] == materials[4]) ? 4 : 3;
        for (int i = start; i <= end; i++) {
            int groupIndex;
            if(materials[2] == materials[i]){
                groupIndex = 1;
            } else{
                groupIndex = (i < 2) ? 0 : 2;
            }
            float distanceCurrentPos = abs(i - 2);
            float value = (1.0 - pointCenterDistances[i]) * pow((1.0 / (1.0 + distanceCurrentPos)), 1.5);
            groupValues[groupIndex] += value;
        }

        // Determine the group with the largest value
        int maxGroupIndex = 0;
        if (groupValues[1] > groupValues[0]) maxGroupIndex = 1;
        if (groupValues[2] > groupValues[maxGroupIndex]) maxGroupIndex = 2;

        // Assign the current material based on the group with the largest value
        return materials[maxGroupIndex * 2];
    }
    else{
        return material;
    }
}

vec3 findSurfacePos(vec3 startPos, vec3 direction, float currentMaterial, int iterations, float[5] materials, float[5] pointCenterDistances) {
    vec3 lowPos = startPos;
    vec3 highPos = startPos + direction;
    float epsilon = 0.01; // Small value to handle precision issues
    bool foundSurface = false;
    for (int i = 0; i < iterations; ++i) 
    {
        vec3 midPos = ((lowPos + highPos) * 0.5);
        float midMaterial = getMaterialID(midPos / dimensions, materials, pointCenterDistances);

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

void main()
{
    vec2 dirTexSize = textureSize(directions, 0);
    vec2 matTexSize = textureSize(materialTexture, 0);

    vec2 normTexCoords = gl_FragCoord.xy / dirTexSize;

    vec3 frontFacesPos = texture(frontFaces, normTexCoords).xyz;
    vec3 backFacesPos = texture(backFaces, normTexCoords).xyz;

    if(frontFacesPos == backFacesPos) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 directionSample = backFacesPos - frontFacesPos; // Get the direction and length of the ray
    vec3 directionRay = normalize(directionSample);
    float lengthRay = length(directionSample / invDimensions);

    vec3 samplePos = worldPos; // start position of the ray
    vec3 increment = stepSize * normalize(directionRay);
    
    vec4 color = vec4(0.0);
    float[5] materials = float[5](0.0, 0.0, 0.0, 0.0, 0.0);
    float[5] pointCenterDistances = float[5](0.0, 0.0, 0.0, 0.0, 0.0);

    float currentMaterial = 0;
    float previousMaterial = 0;

    vec3 previousPos = samplePos;

    for (int i = 0; i < 2; i++) {
        samplePos += increment;
        vec3 volPos = samplePos / dimensions;
        float material = texture(volumeData, volPos).r; //material ID using NN

        materials[i + 2] = material;
        pointCenterDistances[i + 2] = length(floor(samplePos) + 0.5f - samplePos);
    }
    // Walk from front to back
    for (float t = 0.0; t <= lengthRay; t += stepSize)
    {
        vec3 volPos = samplePos / dimensions;
        // update arrays with the new material
        materials[4] = texture(volumeData, volPos).r; 
        pointCenterDistances[4] = length(floor(samplePos) + 0.5f - samplePos);

        float currentMaterial = getMaterialID(volPos, materials, pointCenterDistances); 
        vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) / matTexSize);

        // If we have a surface, add shading to it by finding its normal
        if(previousMaterial != currentMaterial){
            // Use a bisection method to find the accurate surface position
            int iterations = 10;
            vec3 surfacePos = findSurfacePos(previousPos, increment, previousMaterial, iterations, materials, pointCenterDistances);

            // check if a clipping plane was hit
            vec3 previousPos = samplePos.xyz - (increment + 0.001f);
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
                vec3 bottomLeftOffsetPos = previousPos- increment + bottomLeftDirection * offsetLength;
                vec3 bottomRightOffsetPos = previousPos - increment + bottomRightDirection * offsetLength;

                vec3 upPos = findSurfacePos(upOffsetPos , increment * 4, previousMaterial, iterations + 2, materials, pointCenterDistances);
                vec3 bottomLeftPos = findSurfacePos(bottomLeftOffsetPos, increment * 4, previousMaterial, iterations + 2, materials, pointCenterDistances);
                vec3 bottomRightPos = findSurfacePos(bottomRightOffsetPos, increment * 4, previousMaterial, iterations + 2, materials, pointCenterDistances);

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
        
        previousMaterial = currentMaterial;
        samplePos += increment;
        for(int i = 0; i < 4; i++){
            materials[i] = materials[i + 1];
            pointCenterDistances[i] = pointCenterDistances[i + 1];
        }
    }
    FragColor = color;
}
