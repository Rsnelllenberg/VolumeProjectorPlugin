#version 430
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air)
uniform sampler3D volumeData; // contains the Material IDs of the DR

uniform vec3 invDimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invDirTexSize; // Pre-divided dirTexSize (1.0 / dirTexSize)
uniform vec2 invMatTexSize; // Pre-divided matTexSize (1.0 / matTexSize)

uniform vec3 camPos; 
uniform vec3 lightPos;

uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

uniform bool useShading;

// Declare edgeTable and triTable as Shader Storage Buffer Objects (SSBOs)
layout(std430, binding = 0) buffer EdgeTableBuffer {
    int edgeTable[256];
};

layout(std430, binding = 1) buffer TriTableBuffer {
    int triTable[4096];
};

float sampleVolume(vec3 samplePos) {
    vec3 volPos = samplePos * invDimensions;
    return texture(volumeData, volPos).r;
}

vec3 calculateIntersection(vec3 p1, vec3 p2, vec3 p3, vec3 rayStart, vec3 rayEnd) {  
   vec3 rayDir = normalize(rayEnd - rayStart);  
   vec3 edge1 = p2 - p1;  
   vec3 edge2 = p3 - p1;  
   vec3 h = cross(rayDir, edge2);  
   float a = dot(edge1, h);  

   // Check if the ray is parallel to the triangle  
   if (abs(a) < 0.0001) {  
       return vec3(-1); // No intersection  
   }  

   float f = 1.0 / a;  
   vec3 s = rayStart - p1;  
   float u = f * dot(s, h);  

   // Check if the intersection is outside the triangle  
   if (u < 0.0 || u > 1.0) {  
       return vec3(-1); // No intersection  
   }  

   vec3 q = cross(s, edge1);  
   float v = f * dot(rayDir, q);  

   // Check if the intersection is outside the triangle  
   if (v < 0.0 || u + v > 1.0) {  
       return vec3(-1); // No intersection  
   }  

   // Calculate the distance along the ray to the intersection point  
   float t = f * dot(edge2, q);  
   if (t > 0.0001) {  
       return rayStart + t * rayDir; // Intersection point  
   }  

   return vec3(-1); // No intersection  
}

// Interpolate vertex position based on the isovalue  
vec3 interpolateVertex(vec3 p1, vec3 p2, float val1, float val2, float isovalue) {  
   float t = (isovalue - val1) / (val2 - val1);  
   return mix(p1, p2, t);  
}  

float getNewMaterial(vec3 samplePos, vec3 previousPos, float previousMaterial) {
    // Determine a cell whose center lines up with samplePos
    vec3 cellCenter = floor(samplePos + 0.5);
    // Then, define the cell's origin so that the cell spans from (origin) to (origin+1)
    vec3 cellOrigin = cellCenter - 0.5;
    float currentMaterial = sampleVolume(samplePos); // Get the material ID of the current voxel

    vec3 cubeVertices[8] = vec3[8](  
        vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0),  
        vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 1, 1), vec3(0, 1, 1)  
    );  
    float cubeValues[8];

    // Update the cube vertices based on the voxel side and sample the volume data at the cube's vertices 
    for (int i = 0; i < 8; ++i) {
        cubeVertices[i] = cubeVertices[i] + cellOrigin;
        cubeValues[i] = sampleVolume(cubeVertices[i]); 
    }

    // Determine the cube index  
    int cubeIndex = 0; 
    float otherMaterial = 0; // Example material ID for the first vertex
    for (int i = 0; i < 8; ++i) {  
        if (cubeValues[i] != currentMaterial) {  
            cubeIndex |= (1 << i);  
            otherMaterial = cubeValues[i]; // What is the material used that is not the current material
        }
    }  

    // Get the edges intersected by the isosurface  
    int edges = edgeTable[cubeIndex];  
    if (edges == 0) {  
        return currentMaterial; // No intersection, return the material ID of the current voxel
    }  

    // We assume the vertex is alwas halfway between the two vertices of the edge  
    vec3 edgeVertices[12];  
    if ((edges & 1) != 0) edgeVertices[0] = mix(cubeVertices[0], cubeVertices[1], 0.5f);  
    if ((edges & 2) != 0) edgeVertices[1] = mix(cubeVertices[1], cubeVertices[2], 0.5f);  
    if ((edges & 4) != 0) edgeVertices[2] = mix(cubeVertices[2], cubeVertices[3], 0.5f);  
    if ((edges & 8) != 0) edgeVertices[3] = mix(cubeVertices[3], cubeVertices[0], 0.5f);  
    if ((edges & 16) != 0) edgeVertices[4] = mix(cubeVertices[4], cubeVertices[5], 0.5f);  
    if ((edges & 32) != 0) edgeVertices[5] = mix(cubeVertices[5], cubeVertices[6], 0.5f);  
    if ((edges & 64) != 0) edgeVertices[6] = mix(cubeVertices[6], cubeVertices[7], 0.5f);  
    if ((edges & 128) != 0) edgeVertices[7] = mix(cubeVertices[7], cubeVertices[4], 0.5f);  
    if ((edges & 256) != 0) edgeVertices[8] = mix(cubeVertices[0], cubeVertices[4], 0.5f);  
    if ((edges & 512) != 0) edgeVertices[9] = mix(cubeVertices[1], cubeVertices[5], 0.5f);  
    if ((edges & 1024) != 0) edgeVertices[10] = mix(cubeVertices[2], cubeVertices[6], 0.5f);  
    if ((edges & 2048) != 0) edgeVertices[11] = mix(cubeVertices[3], cubeVertices[7], 0.5f);

    vec3 voxelPos = floor(samplePos); // Get the voxel position in the volume data
    // Generate triangles based on the triangle table  
    for (int i = 0; triTable[cubeIndex * 16 + i] != -1; i += 3) {  

        vec3 p1 = edgeVertices[triTable[cubeIndex * 16 + i]];  
        vec3 p2 = edgeVertices[triTable[cubeIndex * 16 + i + 1]];  
        vec3 p3 = edgeVertices[triTable[cubeIndex * 16 + i + 2]];

        vec3 intersection = calculateIntersection(p1, p2, p3, previousPos, samplePos);
        if (intersection == vec3(-1)) {
            continue; // No valid intersection in this triangle
        }

        // Determine on which side the sample is relative to the intersection
        float closestVoxelSample = sampleVolume(intersection);
        vec3 normal = normalize(cross(p2 - p1, p3 - p1)); 
        if(dot(normal, samplePos - intersection) < 0.0) {
            return closestVoxelSample; // Return the other material ID
        } else {
            if(closestVoxelSample != currentMaterial) {
                return currentMaterial; // Return the other material ID
            } else {
                return otherMaterial; // Return the current material ID
            }
        }
    }

    return previousMaterial;  // missed the surface, return the previous material ID
}  


void updateArrays(inout float[5] materials, inout float[5] voxelRayLengths, float newMaterial, float newRayLength) {
    // Shift all elements one position back
    for (int j = 0; j < 4; ++j) {
        materials[j] = materials[j + 1];
        voxelRayLengths[j] = voxelRayLengths[j + 1];
    }
    // Insert the new elements at the last position
    materials[4] = newMaterial;
    voxelRayLengths[4] = newRayLength;
}

// Calculation based on the Amanatides & Woo "A Fast Voxel Traversal Algorithm For Ray Tracing" paper
// Calculates the next intersection point of the ray with the voxel borders
float findNextVoxelIntersection(inout vec3 tNext, vec3 tDelta, vec3 samplePos, vec3 rayDir, float t) {
    float stepLength;

    if (tNext.x < tNext.y) {
        if (tNext.x < tNext.z) {
            stepLength = tNext.x - t;
            tNext.x += tDelta.x;
        } else {
            stepLength = tNext.z - t;
            tNext.z += tDelta.z;
        }
    } else {
        if (tNext.y < tNext.z) {
            stepLength = tNext.y - t;
            tNext.y += tDelta.y;
        } else {
            stepLength = tNext.z - t;
            tNext.z += tDelta.z;
        }
    }
    return stepLength;
}

void processVoxel(inout vec3 samplePos, inout float lengthRay, inout float currentMaterial, inout float[5] materials, inout float[5] voxelRayLengths, vec3 rayDir, inout vec3 tNext, vec3 tDelta, inout float t) {
    float stepLength = findNextVoxelIntersection(tNext, tDelta, samplePos, rayDir, t);

    // Update the sample position
    vec3 previousPos = samplePos;
    samplePos += rayDir * stepLength;
    lengthRay -= stepLength;
    t += stepLength;

    if (lengthRay <= 0.0) {
        return;
    }

    // Sample the voxel and update materials and voxelRayLengths
    currentMaterial = getNewMaterial(samplePos, previousPos, currentMaterial);

    updateArrays(materials, voxelRayLengths, currentMaterial, stepLength);
}

void performAlphaCompositing(inout vec4 color, float[5] materials, float[5] voxelRayLengths) {
    float previousMaterial = materials[1];
    float currentMaterial = materials[2];

    vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) * invMatTexSize);

    // Perform alpha compositing (front to back)
    vec3 outRGB = color.rgb + (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
    float outAlpha = color.a + (1.0 - color.a) * sampleColor.a;
    color = vec4(outRGB, outAlpha);
}

void main() {
    vec2 normTexCoords = gl_FragCoord.xy * invDirTexSize;

    vec3 frontFacesPos = texture(frontFaces, normTexCoords).xyz;
    vec3 backFacesPos = texture(backFaces, normTexCoords).xyz;

    if(frontFacesPos == backFacesPos) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 directionSample = backFacesPos - frontFacesPos; // Get the direction and length of the ray
    vec3 directionRay = normalize(directionSample);
    float lengthRay = length(directionSample / invDimensions);

    vec3 rayDir = normalize(directionRay);
    vec3 samplePos = (frontFacesPos / invDimensions) + rayDir * 0.001f; // Start position of the ray with a small offset to avoid being exactly on the border of a voxel
    vec4 color = vec4(0.0);

    float currentMaterial = sampleVolume(samplePos); // Get the material ID of the current voxel

    // Initialize the materials and voxelRayLengths arrays
    float[5] materials = float[5](currentMaterial, currentMaterial, currentMaterial, currentMaterial, currentMaterial);
    float[5] voxelRayLengths = float[5](0.0, 0.0, 0.0, 0.0, 0.0);

    // Calculate the intersection times for the x, y, and z borders
    vec3 tDelta = 1.0 / (abs(rayDir) + 0.000001f);
    vec3 tNext = abs(step(vec3(0.0), rayDir) * (1.0 - fract(samplePos)) + step(rayDir, vec3(0.0)) * fract(samplePos)) * tDelta; // the distance to the next voxel border in time
    
    float t = 0.0;
    while (lengthRay > 0.0) {
        processVoxel(samplePos, lengthRay, currentMaterial, materials, voxelRayLengths, rayDir, tNext, tDelta, t);
        if (lengthRay <= 0.0) {
            break;
        }
         //TODO run this for two extra steps
        performAlphaCompositing(color, materials, voxelRayLengths);

        // Early stopping condition
        if (color.a >= 1.0) {
            break;
        }
    }
    FragColor = color;
}
