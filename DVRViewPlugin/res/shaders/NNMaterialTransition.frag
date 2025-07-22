#version 430
out vec4 FragColor;

// Input textures and volume data
uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air)
uniform sampler3D volumeData;      // contains the Material IDs of the DR

// Volume and texture dimensions
uniform vec3 dimensions;
uniform vec3 invDimensions;    // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invFaceTexSize;   // Pre-divided FaceTexSize (1.0 / FaceTexSize)
uniform vec2 invMatTexSize;    // Pre-divided matTexSize (1.0 / matTexSize)

// Rendering and camera parameters
uniform vec3 camPos; 
uniform vec3 lightPos;

// Clipping planes
uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

uniform bool useShading;
uniform bool useClutterRemover;

const float MAX_FLOAT = 3.4028235e34;
const float EPSILON = 0.00001f;

// Sample the volume at a given position and return the material ID
float sampleVolume(vec3 samplePos){
    vec3 voxelPos = floor(samplePos) + 0.5f;
    vec3 volPos = samplePos * invDimensions;
    return texture(volumeData, volPos).r;
}

// Sliding window: update arrays (shift left, insert new at end)
void updateArrays(
    inout float[5] materials,
    inout vec3[5] samplePositions,
    inout vec3[5] normals,
    float newMaterial,
    vec3 newSamplePos,
    vec3 newNormal
) {
    for (int j = 0; j < 4; ++j) {
        materials[j] = materials[j + 1];
        samplePositions[j] = samplePositions[j + 1];
        normals[j] = normals[j + 1];
    }
    materials[4] = newMaterial;
    samplePositions[4] = newSamplePos;
    normals[4] = newNormal;
}


// Get the current material using the sliding window (like MaterialTransition2D.frag)
// Refinement step: copy previous value instead of re-sampling
float getMaterialID(inout float[5] materials, inout vec3[5] samplePositions) {
    float previousMaterial = materials[1];
    float currentMaterial = materials[2];
    float nextMaterial = materials[3];

    // If a transition is detected, copy the previous value instead of re-sampling
    if (currentMaterial != previousMaterial && currentMaterial != nextMaterial
        && useClutterRemover) {
        currentMaterial = previousMaterial;
        materials[2] = currentMaterial;
    }
    return currentMaterial;
}

// DDA: Find the next voxel boundary intersection and return the step length
// Returns the step length and sets hitAxis to 0 (x), 1 (y), or 2 (z)
float findNextVoxelIntersection(
    inout vec3 tNext,
    vec3 tDelta,
    vec3 rayDir,
    float t,
    out int hitAxis,
    inout vec3 voxelSampleIndex
) {
    float stepLength;
    if (tNext.x < tNext.y) {
        if (tNext.x < tNext.z) {
            stepLength = tNext.x - t;
            tNext.x += tDelta.x;
            hitAxis = 0;
        } else {
            stepLength = tNext.z - t;
            tNext.z += tDelta.z;
            hitAxis = 2;
        }
    } else {
        if (tNext.y < tNext.z) {
            stepLength = tNext.y - t;
            tNext.y += tDelta.y;
            hitAxis = 1;
        } else {
            stepLength = tNext.z - t;
            tNext.z += tDelta.z;
            hitAxis = 2;
        }
    }
    // Update voxelSampleIndex along the hit axis
    float dir = sign(rayDir[hitAxis]);
    voxelSampleIndex[hitAxis] += dir;
    return stepLength;
}

// Apply Phong shading at a surface position, using the local normal and lighting
vec4 applyShading(
    vec3 previousPos,
    vec3 directionRay,
    vec4 sampleColor,
    vec3 surfacePos,
    vec3 normal
) {
    vec3 minfaces = 1.0 + sign(u_minClippingPlane - (previousPos * invDimensions));
    vec3 maxfaces = 1.0 + sign((previousPos * invDimensions) - u_maxClippingPlane);
    vec3 surfaceGradient = maxfaces - minfaces;

    if (!all(equal(surfaceGradient, vec3(0).xyz))) {
        normal = normalize(surfaceGradient);
    } else {
        vec3 lightDir = normalize(lightPos - surfacePos);
        vec3 viewDir = normalize(camPos - surfacePos);

        vec3 ambient = 0.1 * sampleColor.rgb;

        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = 0.9 * diff * sampleColor.rgb;

        vec3 reflectDir = reflect(-lightDir, normal);
        float specular = pow(max(dot(viewDir, reflectDir), 0.0), 128.0);

        vec3 phongColor = ambient + diffuse + specular;
        sampleColor.rgb = phongColor;
    }
    return sampleColor;
}

void main() {
    vec2 normTexCoords = gl_FragCoord.xy * invFaceTexSize;
    vec3 frontFacesPos = texture(frontFaces, normTexCoords).xyz * dimensions;
    vec3 backFacesPos = texture(backFaces, normTexCoords).xyz * dimensions;

    if(frontFacesPos == backFacesPos) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 directionSample = backFacesPos - frontFacesPos;
    vec3 rayDir = normalize(directionSample);
    float lengthRay = length(directionSample);

    vec3 samplePos = frontFacesPos;
    vec4 color = vec4(0.0);

    // --- Sliding window arrays ---
    float[5] materials = float[5](0.0, 0.0, 0.0, 0.0, 0.0);
    vec3[5] samplePositions = vec3[5](samplePos, samplePos, samplePos, samplePos, samplePos);
    vec3[5] normals = vec3[5](vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0), vec3(0.0));

    // DDA setup
    vec3 tDelta = mix(vec3(MAX_FLOAT), 1.0 / abs(rayDir), notEqual(rayDir, vec3(0.0)));
    vec3 tNext = abs(step(vec3(0.0), rayDir) * (1.0 - fract(samplePos)) + step(rayDir, vec3(0.0)) * fract(samplePos)) * tDelta;
    tNext = mix(vec3(0.0001),tNext, notEqual(tNext, vec3(0.0)));

    float t = 0.0;
    int hitAxis = 0;

    // Track voxel index for correct sampling
    vec3 voxelSampleIndex = floor(samplePos);

   
    // Initialize sliding window with first sample and normal
    float newMaterial = sampleVolume(voxelSampleIndex);
    vec3 newNormal = vec3(0.0); // No normal for the first sample
    for (int i = 0; i < 5; ++i) {
        materials[i] = newMaterial;
        samplePositions[i] = samplePos;
        normals[i] = newNormal;
    }

    while (lengthRay > 0.0) {
        float stepLength = findNextVoxelIntersection(tNext, tDelta, rayDir, t, hitAxis, voxelSampleIndex);
        if (stepLength <= 0.0 || lengthRay <= 0.0)
            break;

        vec3 previousPos = samplePositions[1];
        vec3 currentPos = samplePos + rayDir * stepLength;

        // Compute normal for this step
        vec3 newNormal = vec3(0.0);
        if (hitAxis == 0)      newNormal.x = -sign(rayDir.x);
        else if (hitAxis == 1) newNormal.y = -sign(rayDir.y);
        else if (hitAxis == 2) newNormal.z = -sign(rayDir.z);

        // Sample new material and update sliding window (including normals)
        newMaterial = sampleVolume(voxelSampleIndex + 0.5f); // + 0.5f to center the sample
        updateArrays(materials, samplePositions, normals, newMaterial, currentPos, newNormal);

        float previousMaterial = materials[1];
        float currentMaterial = getMaterialID(materials, samplePositions);

        vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) * invMatTexSize);

        // Only composite if not the very first step
        if (t > 0.0) {
            // Use the normal associated with the current sample (index 2)
            vec3 normal = normals[2];

            if (useShading && previousMaterial != currentMaterial && sampleColor.a > 0.01) {
                sampleColor = applyShading(previousPos, rayDir, sampleColor, currentPos, normal);
            }

            if (currentMaterial == previousMaterial) {
                sampleColor.a *= length(currentPos - previousPos);
            }

            color.rgb += (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
            color.a += (1.0 - color.a) * sampleColor.a;

            if (color.a >= 1.0)
                break;
        }

        t += stepLength;
        lengthRay -= stepLength;
        samplePos = currentPos;
    }

    FragColor = color;
}