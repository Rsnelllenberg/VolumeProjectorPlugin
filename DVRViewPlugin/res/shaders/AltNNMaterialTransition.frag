#version 330
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D directions;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air), the tfTexture should have the same
uniform sampler3D volumeData; // contains the Material IDs of the DR

uniform vec3 dimensions;

uniform vec3 camPos; 
uniform vec3 lightPos;

uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

uniform bool useShading;

// Calculation based on the Amanatides & Woo “A Fast Voxel Traversal Algorithm For Ray Tracing” paper
// Calculates the next intersection point of the ray with the voxel borders
float findNextVoxelIntersection(inout vec3 tNext, vec3 tDelta, vec3 samplePos, vec3 rayDir, vec3 dimensions, float t) {
    float stepLength;
    if (tNext.x < tNext.y) {
        if (tNext.x < tNext.z) {
            stepLength = tNext.x - t;
            if (samplePos.x < 0.0 || samplePos.x >= dimensions.x) return -1.0;
            tNext.x += tDelta.x;
        } else {
            stepLength = tNext.z - t;
            if (samplePos.z < 0.0 || samplePos.z >= dimensions.z) return -1.0;
            tNext.z += tDelta.z;
        }
    } else {
        if (tNext.y < tNext.z) {
            stepLength = tNext.y - t;
            if (samplePos.y < 0.0 || samplePos.y >= dimensions.y) return -1.0;
            tNext.y += tDelta.y;
        } else {
            stepLength = tNext.z - t;
            if (samplePos.z < 0.0 || samplePos.z >= dimensions.z) return -1.0;
            tNext.z += tDelta.z;
        }
    }
    return stepLength;
}

void processVoxel(inout int i, inout vec3 samplePos, inout float lengthRay, inout vec3 voxelCoord, inout float currentMaterial, inout float[5] materials, inout float[5] voxelRayLengths, vec3 rayDir, vec3 dimensions, inout vec3 tNext, vec3 tDelta, inout float t) {
    float stepLength = findNextVoxelIntersection(tNext, tDelta, samplePos, rayDir, dimensions, t);
    if (stepLength < 0.0){
        lengthRay = -1;
        return;
    }

    // Update the sample position
    samplePos += rayDir * stepLength;
    lengthRay -= stepLength;
    t += stepLength;

    // Sample the voxel and update materials and voxelRayLengths
    voxelCoord = samplePos / dimensions; // Add a small offset to avoid sampling on the border
    currentMaterial = texture(volumeData, voxelCoord).r;

    materials[i % 5] = currentMaterial;
    voxelRayLengths[i % 5] = stepLength;
}

void performAlphaCompositing(inout vec4 color, float[5] materials, float[5] voxelRayLengths, int i, int maxI, vec2 matTexSize) {
    float previousMaterial = materials[min(i + 2, maxI) % 5];
    float currentMaterial = materials[min(i + 3, maxI) % 5];
    float nextMaterial = materials[min(i + 4, maxI) % 5];
    float lastMaterial = materials[min(i + 5, maxI) % 5];

    vec4 sampleColor = texture(materialTexture, vec2(previousMaterial, currentMaterial) / matTexSize);
//    if(currentMaterial != previousMaterial && currentMaterial != nextMaterial) {
//        sampleColor.a *= voxelRayLengths[min(i + 3, maxI) % 5];
//    } 

    // Perform alpha compositing (front to back)
    vec3 outRGB = color.rgb + (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
    float outAlpha = color.a + (1.0 - color.a) * sampleColor.a;
    color = vec4(outRGB, outAlpha);
}

void main() {
    vec2 dirTexSize = textureSize(directions, 0);
    vec2 matTexSize = textureSize(materialTexture, 0);

    vec2 normTexCoords = gl_FragCoord.xy / dirTexSize;

    vec4 directionSample = texture(directions, normTexCoords);
    vec3 directionRay = directionSample.xyz;
    float lengthRay = directionSample.a;


    vec3 rayDir = normalize(directionRay);
    vec3 samplePos = worldPos + rayDir * 0.001f; // start position of the ray with avoid being exactly on the border of a voxel
    if(any(lessThan(samplePos, vec3(0.0))) || any(greaterThan(samplePos, dimensions))){
        FragColor = vec4(0.0);
        return;
    }
    vec4 color = vec4(0.0);

    vec3 voxelCoord = samplePos / dimensions; // Add a small offset to avoid sampling on the border

    float currentMaterial = texture(volumeData, voxelCoord).r;
    float previousMaterial = currentMaterial;
    // Calculate the intersection times for the x, y, and z borders
    vec3 tDelta = 1.0 / (abs(rayDir) + 0.000001f); // the ray length to traverse one voxel for each axis
    vec3 tNext = abs(step(vec3(0.0), rayDir) * (1.0 - fract(samplePos)) + step(rayDir, vec3(0.0)) * fract(samplePos)) * tDelta; // the distance to the next voxel border in time

    // Initialize the materials and voxelRayLengths arrays with length 5
    float[5] materials = float[5](currentMaterial, currentMaterial, currentMaterial, currentMaterial, currentMaterial);
    float[5] voxelRayLengths = float[5](0.0, 0.0, 0.0, 0.0, 0.0);

    int i = 0;
    float t = 0.0;
    while (lengthRay > 0) {
        i = i + 1;
        processVoxel(i, samplePos, lengthRay, voxelCoord, currentMaterial, materials, voxelRayLengths, rayDir, dimensions, tNext, tDelta, t);
        if (lengthRay <= 0.0){
            break;
        }

        performAlphaCompositing(color, materials, voxelRayLengths, i, i + 5, matTexSize);

        // Early stopping condition
        if (color.a >= 1.0) {
            break;
        }
//        previousMaterial = currentMaterial;
    }
    int maxI = i + 5;
    for (int j = 0; j < 2; j++) {
        i = i + 1;
        performAlphaCompositing(color, materials, voxelRayLengths, i, maxI, matTexSize);

        // Early stopping condition
        if (color.a >= 1.0) {
            break;
        }
    }

    FragColor = color;
}

