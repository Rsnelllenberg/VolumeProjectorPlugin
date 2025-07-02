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

const float MAX_FLOAT = 3.4028235e34;
const float EPSILON = 0.00001f;

// Sample the volume at a given position and return the material ID
float sampleVolume(vec3 samplePos) {
    vec3 volPos = samplePos * invDimensions;
    return texture(volumeData, volPos).r;
}

// Get the material at a given sample position, with smoothing for transitions
float getNewMaterial(vec3 previousPos, vec3 currentPos) {
    vec3 samplePos = floor((previousPos + currentPos) * 0.5 + 0.5) - 0.5;

    float voxelMaterial = sampleVolume(samplePos);
//    vec3 interVoxelPos = fract(samplePos);
//    vec3 voxelSide = sign(interVoxelPos - 0.5);
//
//    float materialX = sampleVolume(samplePos + vec3(voxelSide.x, 0.0, 0.0));
//    float materialY = sampleVolume(samplePos + vec3(0.0, voxelSide.y, 0.0));
//    float materialZ = sampleVolume(samplePos + vec3(0.0, 0.0, voxelSide.z));
//
//    if (materialX == materialY && materialX == materialZ && materialX != voxelMaterial) { 
//        return materialX;
//    }
    return voxelMaterial;
}

// DDA: Find the next voxel boundary intersection and return the step length
// Returns the step length and sets hitAxis to 0 (x), 1 (y), or 2 (z)
float findNextVoxelIntersection(inout vec3 tNext, vec3 tDelta, vec3 rayDir, float t, out int hitAxis) {
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

    float previousMaterial = sampleVolume(samplePos);
    float currentMaterial = previousMaterial;

    vec3 previousPos = samplePos;
    vec3 currentPos = samplePos;

    // DDA setup
    vec3 tDelta = mix(vec3(MAX_FLOAT), 1.0 / abs(rayDir), notEqual(rayDir, vec3(0.0)));
    vec3 tNext = abs(step(vec3(0.0), rayDir) * (1.0 - fract(samplePos + 0.5)) + step(rayDir, vec3(0.0)) * fract(samplePos + 0.5)) * tDelta; // The ray length for each axis needed to reach a boundery, with a 0.5 offset since we use MC for smoothing where the center of a cube is the intersection of 8 voxels 

    float t = 0.0;

    int currentHitAxis = 0;
    int nextHitAxis = 0;

    while (lengthRay > 0.0) {
        currentHitAxis = nextHitAxis;
        float stepLength = findNextVoxelIntersection(tNext, tDelta, rayDir, t, nextHitAxis);
        if (stepLength <= 0.0 || lengthRay <= 0.0)
            break;

        previousPos = currentPos;
        currentPos = samplePos + rayDir * stepLength;
        previousMaterial = currentMaterial;
        currentMaterial = getNewMaterial(previousPos, currentPos);

        // Only composite if not the very first step
        if (t > 0.0) {
            vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) * invMatTexSize);

            if (useShading && previousMaterial != currentMaterial && sampleColor.a > 0.01) {
                // Compute normal based on hit axis and ray direction
                vec3 normal = vec3(0.0);
                if (currentHitAxis == 0)      normal.x = -sign(rayDir.x);
                else if (currentHitAxis == 1) normal.y = -sign(rayDir.y);
                else if (currentHitAxis == 2) normal.z = -sign(rayDir.z);

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
