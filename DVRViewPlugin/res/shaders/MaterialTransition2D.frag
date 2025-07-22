#version 330
out vec4 FragColor;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air), the tfTexture should have the same
uniform sampler2D tfTexture;
uniform sampler3D volumeData; // contains the 2D positions of the DR

uniform vec3 dimensions; 
uniform vec3 invDimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invFaceTexSize; // Pre-divided FaceTexSize (1.0 / FaceTexSize)
uniform vec2 invTfTexSize;  // Pre-divided tfTexSize (1.0 / tfTexSize)
uniform vec2 invMatTexSize; // Pre-divided matTexSize (1.0 / matTexSize)

uniform float stepSize;
uniform vec3 camPos; 
uniform vec3 lightPos;

uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

uniform bool useShading;
uniform bool useClutterRemover;

float getMaterialID(inout float[5] materials, vec3[5] samplePositions) {
    float firstMaterial = materials[0];
    float previousMaterial = materials[1];

    float currentMaterial = materials[2];

    float nextMaterial = materials[3];
    float lastMaterial = materials[4];

    if(useClutterRemover && firstMaterial == previousMaterial && nextMaterial == lastMaterial && currentMaterial != previousMaterial && currentMaterial != nextMaterial){
        vec3 samplePos = round(samplePositions[2] + vec3(0.5f)) - vec3(0.5f); // Sample the nearest voxel center instead
        vec2 sample2DPos = texture(volumeData, samplePos * invDimensions).rg * invTfTexSize;
        currentMaterial = texture(tfTexture, sample2DPos).r + 0.5f;

        // Update the materials array with the new material
        materials[2] = currentMaterial;
    }

    return currentMaterial;
}

vec3 findSurfacePos(vec3 startPos, vec3 direction, int iterations, float[5] materials, vec3[5] samplePositions, float referenceMaterial, bool biasTowardsStartPos) {
    vec3 lowPos = startPos;
    vec3 highPos = startPos + direction;
    float epsilon = 0.01;

    // Sample materials at both ends
    vec2 lowSample2D = texture(volumeData, lowPos * invDimensions).rg * invTfTexSize;
    float lowMat = texture(tfTexture, lowSample2D).r + 0.5f;
    vec2 highSample2D = texture(volumeData, highPos * invDimensions).rg * invTfTexSize;
    float highMat = texture(tfTexture, highSample2D).r + 0.5f;

    // If both ends are the same, try to step further
    int maxStep = 5;
    int stepCount = 0;
    while (abs(highMat - lowMat) < 0.01 && stepCount < maxStep) {
        highPos += direction;
        highSample2D = texture(volumeData, highPos * invDimensions).rg * invTfTexSize;
        highMat = texture(tfTexture, highSample2D).r + 0.5f;
        stepCount++;
    }
    if (abs(highMat - lowMat) < 0.01) return vec3(-1);

    // Bisection
    for (int i = 0; i < iterations; ++i) {
        vec3 midPos; 
        if (biasTowardsStartPos) {
            midPos = mix(lowPos, highPos, 0.2f); // Bias towards the start position
        } else {
            midPos = mix(lowPos, highPos, 0.5f); // No bias
        }
        vec2 midSample2D = texture(volumeData, midPos * invDimensions).rg * invTfTexSize;
        float midMat = texture(tfTexture, midSample2D).r + 0.5f;

        if (abs(midMat - lowMat) > epsilon) {
            highPos = midPos;
            highMat = midMat;
        } else {
            lowPos = midPos;
            lowMat = midMat;
        }
        if (length(highPos - lowPos) < epsilon) break;
    }
    return (lowPos + highPos) * 0.5;
}


// A helperfunction for the applyShading function
vec3 getSurfaceOffsetPos(vec3 offsetPos, vec3 increment, int iterations, float[5] materials, vec3[5] samplePositions) {
    vec3 temppos = samplePositions[2]; // Store the current position in a temporary variable
    float tempmat = materials[2]; // Store the current material in a temporary variable

    vec2 sample2DPos = texture(volumeData, offsetPos * invDimensions).rg * invTfTexSize; 
    samplePositions[2] = offsetPos; 
    materials[2] = texture(tfTexture, sample2DPos).r + 0.5f; // Update the material ID for the current iteration

    float mat = getMaterialID(materials, samplePositions);

    samplePositions[2] = temppos; // Restore the previous position
    materials[2] = tempmat; // Restore the previous material
    float direction = 1;
    if (mat == materials[2])
        direction = -1; // If the material has changed, we need to go in the opposite direction
    return findSurfacePos(offsetPos, increment * direction, iterations, materials, samplePositions, mat, true);
}


vec4 applyShading(vec3 previousPos, vec3 increment, vec3 directionRay, vec4 sampleColor, vec3 surfacePos, int iterations, float[5] materials, vec3[5] samplePositions) {

    // check if a clipping plane was hit
    vec3 minfaces = 1.0 + sign(u_minClippingPlane - (previousPos * invDimensions));
    vec3 maxfaces = 1.0 + sign((previousPos * invDimensions) - u_maxClippingPlane);

    // compute the surface normal (eventually normalize later)
    vec3 surfaceGradient = maxfaces - minfaces;

    vec3 normal;
    int invalidCount = 0;
    // if on clipping plane calculate color and return it
    if (!all(equal(surfaceGradient, vec3(0).xyz))) {
        normal = normalize(surfaceGradient);
    } else {
        float previousMaterial = materials[1]; // Get the previous material ID

        // Find a surface triangle by finding three other closeby points on the surface
        vec3 upDirection = normalize(cross(directionRay, vec3(0.0, 1.0, 0.0)));
        vec3 bottomLeftDirection = normalize(cross(directionRay, vec3(-1.0, -1.0, 0.0)));
        vec3 bottomRightDirection = normalize(cross(directionRay, vec3(1.0, 1.0, 0.0)));

        float offsetLength = clamp(0.2f * stepSize, 0.1f, 0.5f);

        // Define the offset positions
        vec3 upOffsetPos = surfacePos + upDirection * offsetLength;
        vec3 bottomLeftOffsetPos = surfacePos + bottomLeftDirection * offsetLength;
        vec3 bottomRightOffsetPos = surfacePos + bottomRightDirection * offsetLength;

        // Reduce the increment and iterations as we (most likely) start closer to the surface
        increment *= 0.5f; 
        iterations = int(iterations * 0.5f); 

        vec3 upPos = getSurfaceOffsetPos(upOffsetPos, increment, iterations, materials, samplePositions);
        vec3 bottomLeftPos = getSurfaceOffsetPos(bottomLeftOffsetPos, increment, iterations, materials, samplePositions);
        vec3 bottomRightPos = getSurfaceOffsetPos(bottomRightOffsetPos, increment, iterations, materials, samplePositions);

        // Check if any of the positions are vec3(-1)
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
        }
    }
    if (invalidCount < 2) {
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
    return sampleColor;
}

void updateArrays(inout float[5] materials, inout vec3[5] samplePositions, float newMaterial, vec3 newSamplePos) {
    // Shift all elements one position back
    for (int j = 0; j < 4; ++j) {
        materials[j] = materials[j + 1];
        samplePositions[j] = samplePositions[j + 1];
    }
    // Insert the new elements at the last position
    materials[4] = newMaterial;
    samplePositions[4] = newSamplePos;
}

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
    float previousMaterial = 0;

    float[5] materials = float[5](0.0, 0.0, 0.0, 0.0, 0.0);
    vec3[5] samplePositions = vec3[5](samplePos, samplePos, samplePos, samplePos, samplePos);

    float t = 0.0;
    // Walk from front to back
    while (t <= lengthRay + 2 * stepSize)
    {
        vec2 sample2DPos = texture(volumeData, samplePos * invDimensions).rg * invTfTexSize; 
        float newMaterial = texture(tfTexture, sample2DPos).r + 0.5f;

        // Update the arrays
        updateArrays(materials, samplePositions, newMaterial, samplePos);

        if(t > stepSize * 2){ // initialize the first two positions of the array first
            // Get the current material
            float previousMaterial = materials[1];
            float currentMaterial = getMaterialID(materials, samplePositions);

            vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) * invMatTexSize);
         
            // If we have a surface, add shading to it by finding its normal
            if(useShading && previousMaterial != currentMaterial && sampleColor.a > 0.01){
                // Use a bisection method to find the accurate surface position
                int iterations = 10;
                vec3 previousPos = samplePositions[1]; // Position 1 corresponds to the previous position
                vec3 surfacePos = findSurfacePos(previousPos, increment, iterations, materials, samplePositions, currentMaterial, false);
                sampleColor = applyShading(previousPos, increment, directionRay, sampleColor, surfacePos, iterations, materials, samplePositions);
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
        }

        
        t += stepSize;
        if(t < lengthRay){ //Due to the sliding window, we need to continue past the ray length but we simply pad it with the last material
            samplePos += increment;
        }
    }
    FragColor = color;
}
