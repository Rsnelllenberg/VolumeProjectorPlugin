#version 430
out vec4 FragColor;

in vec3 u_color;
in vec3 worldPos;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air)
uniform sampler3D volumeData; // contains the Material IDs of the DR

uniform vec3 dimensions; 
uniform vec3 invDimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invFaceTexSize; // Pre-divided FaceTexSize (1.0 / FaceTexSize)
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

// Define constants for buffers.
const int SLIDING_WINDOW_SIZE = 4;              // Size of the sliding window for accumulating intersection samples. (This should MAX_INTERSECTIONS_PER_VOXEL + 1)
const int MAX_INTERSECTIONS_PER_VOXEL = 3;      // Assumed maximum intersections you expect to get per voxel step.

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
   float segLength = length(rayEnd - rayStart);
    if (t > 0.0001 && t <= segLength) {
        return rayStart + t * rayDir; // Intersection point  
    }

   return vec3(-1); // No intersection  
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

// Evaluates the voxel cell using a marching cubes–like algorithm to generate new intersection samples.
// Returns an array of intersection positions (ordered by distance from the previous ray position)
// along with the associated material IDs.
void getNewIntersections(vec3 samplePos, vec3 previousPos, float previousMaterial,
                           out int intersectionCount,
                           out float newMaterials[MAX_INTERSECTIONS_PER_VOXEL],
                           out vec3 newIntersections[MAX_INTERSECTIONS_PER_VOXEL],
                           out vec3 newNormals[MAX_INTERSECTIONS_PER_VOXEL])
{
    intersectionCount = 0;
    
    // Identify the voxel cell based on the current sample position.
    vec3 cellCenter = floor(samplePos + 0.5);
    vec3 cellOrigin = cellCenter - 0.5;
    float currentMaterial = sampleVolume(samplePos);
    
    // Build the eight corners of the voxel cell.
    vec3 cubeVertices[8] = vec3[8](
        vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0),
        vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 1, 1), vec3(0, 1, 1)
    );
    float cubeValues[8];
    for (int i = 0; i < 8; ++i)
    {
        cubeVertices[i] += cellOrigin;
        cubeValues[i] = sampleVolume(cubeVertices[i]);
    }
    
    // Compute the configuration index for the voxel based on material values.
    int cubeIndex = 0;
    float otherMaterial = currentMaterial;
    for (int i = 0; i < 8; ++i)
    {
        if (cubeValues[i] != previousMaterial)
        {
            cubeIndex |= (1 << i);
            otherMaterial = cubeValues[i];
        }
    }
    
    // Retrieve the intersected edges using a lookup table.
    int edges = edgeTable[cubeIndex];
    if (edges == 0)
    {
        newIntersections[0] = samplePos;
        newMaterials[0] = currentMaterial;
        newNormals[0] = vec3(0.0); // No edge = no normal
        intersectionCount = 1;
        return;
    }
    
    // Calculate the midpoints along edges where intersections occur (allways the midpoint since we do not have isoValues, only material IDs).
    vec3 edgeVertices[12];
    if ((edges & 1) != 0)   edgeVertices[0] = mix(cubeVertices[0], cubeVertices[1], 0.5);
    if ((edges & 2) != 0)   edgeVertices[1] = mix(cubeVertices[1], cubeVertices[2], 0.5);
    if ((edges & 4) != 0)   edgeVertices[2] = mix(cubeVertices[2], cubeVertices[3], 0.5);
    if ((edges & 8) != 0)   edgeVertices[3] = mix(cubeVertices[3], cubeVertices[0], 0.5);
    if ((edges & 16) != 0)  edgeVertices[4] = mix(cubeVertices[4], cubeVertices[5], 0.5);
    if ((edges & 32) != 0)  edgeVertices[5] = mix(cubeVertices[5], cubeVertices[6], 0.5);
    if ((edges & 64) != 0)  edgeVertices[6] = mix(cubeVertices[6], cubeVertices[7], 0.5);
    if ((edges & 128) != 0) edgeVertices[7] = mix(cubeVertices[7], cubeVertices[4], 0.5);
    if ((edges & 256) != 0) edgeVertices[8] = mix(cubeVertices[0], cubeVertices[4], 0.5);
    if ((edges & 512) != 0) edgeVertices[9] = mix(cubeVertices[1], cubeVertices[5], 0.5);
    if ((edges & 1024) != 0) edgeVertices[10] = mix(cubeVertices[2], cubeVertices[6], 0.5);
    if ((edges & 2048) != 0) edgeVertices[11] = mix(cubeVertices[3], cubeVertices[7], 0.5);
    
    // Evaluate intersections for each triangle defined in the lookup table.
    for (int i = 0; triTable[cubeIndex * 16 + i] != -1; i += 3)
    {
        vec3 p1 = edgeVertices[triTable[cubeIndex * 16 + i]];
        vec3 p2 = edgeVertices[triTable[cubeIndex * 16 + i + 1]];
        vec3 p3 = edgeVertices[triTable[cubeIndex * 16 + i + 2]];
        
        vec3 intersectionPoint = calculateIntersection(p1, p2, p3, previousPos, samplePos);
        if (intersectionPoint == vec3(-1))
        {
            continue;
        }
        
        // Calculate the normal vector for the triangle using cross product, and make sure it points towards the camera.
        vec3 normal = normalize(cross(p2 - p1, p3 - p1));
        vec3 reverseNormal = vec3(-normal.x, -normal.y, -normal.z);
        vec3 directionRay = normalize(samplePos - previousPos);
        normal = dot(normal, normalize(directionRay)) < dot(reverseNormal, normalize(directionRay)) ? normal : reverseNormal;

        // Store the found intersection (material assignment is deferred).
        if (intersectionCount < MAX_INTERSECTIONS_PER_VOXEL)
        {
            newIntersections[intersectionCount] = intersectionPoint;
            newNormals[intersectionCount] = normal;
            intersectionCount++;
        }
    }

    // Sort the intersections in ascending order by distance from the previous ray position.
    if (intersectionCount > 1)
    {
        for (int i = 0; i < intersectionCount - 1; i++)
        {
            for (int j = i + 1; j < intersectionCount; j++)
            {
                float dist_i = length(newIntersections[i] - previousPos);
                float dist_j = length(newIntersections[j] - previousPos);
                if (dist_j < dist_i)
                {
                    vec3 tempPos = newIntersections[i];
                    newIntersections[i] = newIntersections[j];
                    newIntersections[j] = tempPos;

                    vec3 tempNormal = newNormals[i];
                    newNormals[i] = newNormals[j];
                    newNormals[j] = tempNormal;
                }
            }
        }
    }

    // After ordering the intersections, assign the material IDs sequentially.
    // The first intersection toggles the incoming previousMaterial, then that result becomes
    // the new base for the next intersection.
    float baseMat = previousMaterial;
    for (int i = 0; i < intersectionCount; i++)
    {
        float newMat = (baseMat != otherMaterial) ? otherMaterial : previousMaterial;
        newMaterials[i] = newMat;
        baseMat = newMat;
    }

    if(intersectionCount > 0){
        return;
    }
    

    newIntersections[0] = samplePos;
    newMaterials[0] = previousMaterial;
    newNormals[0] = vec3(0.0); // No edge = no normal
    intersectionCount = 1;
    return;
}


// Appends a new intersection sample (material ID and segment length) to the accumulation buffer.
void appendIntersection(inout float materials[SLIDING_WINDOW_SIZE],
                        inout vec3 samplePositions[SLIDING_WINDOW_SIZE],
                        inout vec3 normals[SLIDING_WINDOW_SIZE],
                        inout int totalAccum,
                        float newMaterial, vec3 newPosition, vec3 normal)
{
    if (totalAccum < SLIDING_WINDOW_SIZE)
    {
        materials[totalAccum] = newMaterial;
        samplePositions[totalAccum] = newPosition;
        normals[totalAccum] = normal;
        totalAccum++;
    }
}

// Advances the ray to the next voxel boundary and collects new intersection samples,
// updating the current material as necessary.
void processVoxel(inout vec3 samplePos,
                  inout float lengthRay,
                  float currentMaterial,
                  // Intersection accumulation buffer.
                  inout float materials[SLIDING_WINDOW_SIZE],
                  inout vec3 samplePositions[SLIDING_WINDOW_SIZE],
                  inout vec3 normals[SLIDING_WINDOW_SIZE],
                  inout int totalAccum,
                  vec3 rayDir,
                  inout vec3 tNext,
                  vec3 tDelta,
                  inout float t)
{
    // Advance the sample position to the next voxel boundary.
    float stepLength = findNextVoxelIntersection(tNext, tDelta, samplePos, rayDir, t);
    vec3 previousPos = samplePos;
    samplePos += rayDir * stepLength;
    lengthRay -= stepLength;
    t += stepLength;
    
    if (lengthRay <= 0.0)
        return;
    
    // Generate new intersection samples for the current voxel segment.
    int interCount;
    float newMaterials[MAX_INTERSECTIONS_PER_VOXEL];
    vec3 newIntersections[MAX_INTERSECTIONS_PER_VOXEL];
    vec3 newNormals[MAX_INTERSECTIONS_PER_VOXEL];
    getNewIntersections(samplePos, previousPos, currentMaterial, interCount, newMaterials, newIntersections, newNormals);
    
    // Append the newly generated intersection samples to the accumulation buffer.
    for (int i = 0; i < interCount; i++)
    {
        previousPos = newIntersections[i];
        appendIntersection(materials, samplePositions, normals, totalAccum, newMaterials[i], newIntersections[i], newNormals[i]);
    }
}

// Composites the current color using the accumulated intersection samples in the buffer.
// Only the samples between startIndex (inclusive) and endIndex (exclusive) are processed.
void performAlphaCompositing(inout vec4 color,
                             float materials[SLIDING_WINDOW_SIZE],
                             vec3 samplePositions[SLIDING_WINDOW_SIZE],
                             vec3 normals[SLIDING_WINDOW_SIZE],
                             int startIndex, int endIndex)
{
    if(startIndex == 0){ // This is a faulty case
        return;
    }

    for (int i = startIndex; i < endIndex; i++)
    {
        float prevMat = materials[i - 1];
        float currMat = materials[i];
        vec4 sampleColor = texture(materialTexture, vec2(currMat, prevMat) * invMatTexSize);

        float sampleDistance = length(samplePositions[i] - samplePositions[i - 1]);

        if (sampleColor.a == 0.0)
            continue;
        if (currMat == prevMat)
        {
            sampleColor.a *= sampleDistance;
        } else { 
            // Handle the material encountered before hitting the edge first
            vec4 colorBeforeEdge = texture(materialTexture, vec2(prevMat, prevMat) * invMatTexSize);
            colorBeforeEdge.a *= sampleDistance;

            color.rgb += (1.0 - color.a) * colorBeforeEdge.a * colorBeforeEdge.rgb;
            color.a += (1.0 - color.a) * colorBeforeEdge.a;
        }

        if(useShading && currMat != prevMat){
            // Phong shading calculations
            vec3 normal = normalize(normals[i]);
            vec3 ambient = 0.1 * sampleColor.rgb; // Ambient component

            vec3 lightDir = normalize(lightPos - samplePositions[i]);
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * sampleColor.rgb; // Diffuse component

            vec3 viewDir = normalize(camPos - samplePositions[i]);
            vec3 reflectDir = reflect(-lightDir, normal);
            float specular = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);

            vec3 phongColor = ambient + diffuse + specular;
            sampleColor.rgb = phongColor;
        }

        // Blend the new sample color into the current output using front-to-back alpha compositing.
        color.rgb += (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
        color.a += (1.0 - color.a) * sampleColor.a;
    }
}

// Main ray marching loop:
//  - Initializes the ray and accumulated intersection samples,
//  - Traverses the volume generating intersection samples,
//  - And composites the final color using those samples.
void main()
{
    vec2 normTexCoords = gl_FragCoord.xy * invFaceTexSize;
    vec3 frontFacesPos = texture(frontFaces, normTexCoords).xyz * dimensions;
    vec3 backFacesPos = texture(backFaces, normTexCoords).xyz * dimensions;
    
    if (frontFacesPos == backFacesPos)
    {
        FragColor = vec4(0.0);
        return;
    }
    
    vec3 directionSample = backFacesPos - frontFacesPos;
    vec3 rayDir = normalize(directionSample);
    float lengthRay = length(directionSample);
    
    // Initialize starting point and output color.
    vec3 samplePos = frontFacesPos;
    vec4 color = vec4(0.0);
    float currentMaterial = sampleVolume(samplePos);
    
    // Initialize the sliding windows for holding intersection samples.
    float materials[SLIDING_WINDOW_SIZE];
    vec3 samplePositions[SLIDING_WINDOW_SIZE];
    vec3 normals[SLIDING_WINDOW_SIZE];
    int totalAccum = 0;
    
    // Setup traversal parameters based on the ray direction and start position.
    vec3 tDelta = 1.0 / (abs(rayDir) + 0.000001); // How long it take for the ray to reach a voxel boundery for each axis
    vec3 tNext = abs(step(vec3(0.0), rayDir) * (1.0 - fract(samplePos + 0.5)) + step(rayDir, vec3(0.0)) * fract(samplePos + 0.5)) * tDelta; // The ray length for each axis needed to reach a boundery, with a 0.5 offset since we use MC for smoothing where the center of a cube is the intersection of 8 voxels 
    float t = 0.0;
    
    // Traverse through the volume, accumulating intersection samples and compositing color.
    while (lengthRay > 0.0)
    {
        processVoxel(samplePos, lengthRay, currentMaterial,
                     materials, samplePositions, normals, totalAccum,
                     rayDir, tNext, tDelta, t);
        if (lengthRay <= 0.0)
            break;
        
        // Composite the color using the accumulated intersection samples.
        if (totalAccum > 0)
        {
            performAlphaCompositing(color, materials, samplePositions, normals, 1, totalAccum);
            
            // Shift the accumulation buffer, keeping the last sample as the base for future accumulation.
            if (totalAccum > 0)
            {
                materials[0] = materials[totalAccum - 1];
                samplePositions[0] = samplePositions[totalAccum - 1];
                totalAccum = 1;
            }
        }
        
        // Stop if the ray becomes fully opaque.
        if (color.a >= 1.0)
            break;
    }
    
    FragColor = color;
}
