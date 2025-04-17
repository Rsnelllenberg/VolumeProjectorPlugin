#version 430
out vec4 FragColor;

uniform sampler2D frontFaces;
uniform sampler2D backFaces;
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air)
uniform sampler3D volumeData; // contains the Material IDs of the DR

uniform vec3 dimensions;
uniform vec3 invDimensions; // Pre-divided dimensions (1.0 / dimensions)
uniform vec2 invFaceTexSize; // Pre-divided FaceTexSize (1.0 / FaceTexSize)
uniform vec2 invMatTexSize; // Pre-divided matTexSize (1.0 / matTexSize)

uniform float stepSize;
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

//float getNewMaterial(vec3 samplePos) {  
//    float voxelMaterial = sampleVolume(samplePos);
//    vec3 interVoxelPos = fract(samplePos);
//    vec3 voxelSide = sign(interVoxelPos - 0.5);
//
//    float materialX = sampleVolume(samplePos + vec3(voxelSide.x, 0.0, 0.0));
//    float materialY = sampleVolume(samplePos + vec3(0.0, voxelSide.y, 0.0));
//    float materialZ = sampleVolume(samplePos + vec3(0.0, 0.0, voxelSide.z));
//
//    float materialXY = sampleVolume(samplePos + vec3(voxelSide.x, voxelSide.y, 0.0));
//    float materialXZ = sampleVolume(samplePos + vec3(voxelSide.x, 0.0, voxelSide.z));
//    float materialYZ = sampleVolume(samplePos + vec3(0.0, voxelSide.y, voxelSide.z));
//
//    if(materialX == voxelMaterial  && materialX == materialXY && materialX == materialY) {
//        return voxelMaterial;  //No smoothing needed
//    }
//
//    if (materialY == voxelMaterial && materialY == materialXY && materialY == materialYZ) {
//        return voxelMaterial;  //No smoothing needed
//    }
//
//    if (materialZ == voxelMaterial && materialZ == materialXZ && materialZ == materialYZ) {
//        return voxelMaterial;  //No smoothing needed
//    }
//
//    if(materialX != voxelMaterial && materialX == materialY && materialX == materialZ) {
//        if((voxelSide.x * interVoxelPos.x) + (voxelSide.y * interVoxelPos.y) + (voxelSide.z * interVoxelPos.z) >= 1.0f + 0.5 * (voxelSide.x + voxelSide.y + voxelSide.z)) {
//            return materialX;
//        } else {
//            return voxelMaterial;
//        }
//    }
//
//
//    // if two are the same smoothing could happen but it is not needed if the voxel already has the same material as them
//    if (materialX == materialY && materialX != voxelMaterial && materialXY == materialX) { 
//        if (voxelMaterial == materialZ){
//            if( (voxelSide.x * interVoxelPos.x) + (voxelSide.y * interVoxelPos.y) >= 0.5 + 0.5 * (voxelSide.x + voxelSide.y)) {
//                return materialX;
//            }
//        }
//    }
//
//    if (materialX == materialZ && materialX != voxelMaterial && materialXZ == materialX) {
//        if (voxelMaterial == materialY){
//            if( (voxelSide.x * interVoxelPos.x) + (voxelSide.z * interVoxelPos.z) >= 0.5 + 0.5 * (voxelSide.x + voxelSide.z)) {
//                return materialX;
//            }
//        }
//    }
//
//    if (materialY == materialZ && materialY != voxelMaterial && materialYZ == materialY) {
//        if (voxelMaterial == materialX){
//            if( (voxelSide.y * interVoxelPos.y) + (voxelSide.z * interVoxelPos.z) >= 0.5 + 0.5 * (voxelSide.y + voxelSide.z)) {
//                return materialY;
//            }
//        }
//    }
//
//
//
//    return voxelMaterial;  //Simply return the material ID of the sampled voxel
//}  

//float getNewMaterial(vec3 samplePos, vec3 previousPos, float previousMaterial) {
////float getNewMaterial(vec3 samplePos) {  
//    float voxelMaterial = sampleVolume(samplePos);
//    vec3 interVoxelPos = fract(samplePos);
//    vec3 voxelSide = sign(interVoxelPos - 0.5);
//
//    float materialX = sampleVolume(samplePos + vec3(voxelSide.x, 0.0, 0.0));
//    float materialY = sampleVolume(samplePos + vec3(0.0, voxelSide.y, 0.0));
//    float materialZ = sampleVolume(samplePos + vec3(0.0, 0.0, voxelSide.z));
//
//    float materialXY = sampleVolume(samplePos + vec3(voxelSide.x, voxelSide.y, 0.0));
//    float materialXZ = sampleVolume(samplePos + vec3(voxelSide.x, 0.0, voxelSide.z));
//    float materialYZ = sampleVolume(samplePos + vec3(0.0, voxelSide.y, voxelSide.z));
//
//    if(materialX == voxelMaterial && (materialXY == voxelMaterial || materialXZ == voxelMaterial)) {
//        return voxelMaterial;  //No smoothing needed
//    }
//
//    if (materialY == voxelMaterial && (materialXY == voxelMaterial || materialYZ == voxelMaterial)) {
//        return voxelMaterial;  //No smoothing needed
//    }
//
//    if (materialZ == voxelMaterial && (materialXZ == voxelMaterial || materialYZ == voxelMaterial)) {
//        return voxelMaterial;  //No smoothing needed
//    }
//
//    // if two are the same smoothing could happen but it is not needed if the voxel already has the same material as them
//    if (materialX == materialY && materialX != voxelMaterial) { 
//        if (materialXY == materialX){
//            if( (voxelSide.x * interVoxelPos.x) + (voxelSide.y * interVoxelPos.y) >= 0.5 + 0.5 * (voxelSide.x + voxelSide.y)) {
//                return materialX;
//            }
//        }
//    }
//
//    if (materialX == materialZ && materialX != voxelMaterial) {
//        if (materialXZ == materialX){
//            if( (voxelSide.x * interVoxelPos.x) + (voxelSide.z * interVoxelPos.z) >= 0.5 + 0.5 * (voxelSide.x + voxelSide.z)) {
//                return materialX;
//            }
//        }
//    }
//
//    if (materialY == materialZ && materialY != voxelMaterial) {
//        if (materialYZ == materialY){
//            if( (voxelSide.y * interVoxelPos.y) + (voxelSide.z * interVoxelPos.z) >= 0.5 + 0.5 * (voxelSide.y + voxelSide.z)) {
//                return materialY;
//            }
//        }
//    }
//
//    return voxelMaterial;  //Simply return the material ID of the sampled voxel
//}  
//
float getMaterialID(float[5] materials) {
    return materials[2]; // Current material is always at index 2
}

vec3 findSurfacePos(vec3 startPos, vec3 direction, int iterations, float[5] materials) {
    vec3 lowPos = startPos;
    vec3 highPos = startPos + direction;
    float epsilon = 0.01; // Small value to handle precision issues
    bool foundSurface = false;
    float previousMaterial = materials[1]; // Get the previous material ID
    for (int i = 0; i < iterations; ++i) {
        vec3 midPos = (lowPos + highPos) * 0.5;
        float midMaterial = getNewMaterial(midPos, startPos, previousMaterial); // Get the material ID for the current iteration
        materials[2] = midMaterial; // Update the material ID for the current iteration

        midMaterial = getMaterialID(materials); // Get the material ID for the current iteration

        // Check if the material transition is detected
        if (abs(midMaterial - previousMaterial) > epsilon) {
            highPos = midPos;
            foundSurface = true;
        } else {
            lowPos = midPos;
        }

        // Early exit if the positions are very close
        if (length(highPos - lowPos) < epsilon) {
            break;
        }
    }

    if (!foundSurface) {
        return vec3(-1);
    }
    return (lowPos + highPos) * 0.5;
}

vec4 applyShading(vec3 previousPos, vec3 increment, vec3 directionRay, vec4 sampleColor, vec3 surfacePos, int iterations, float[5] materials) {

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

        float offsetLength = 0.2f;

        // Define the offset positions
        vec3 upOffsetPos = previousPos - increment + upDirection * offsetLength;
        vec3 bottomLeftOffsetPos = previousPos - increment + bottomLeftDirection * offsetLength;
        vec3 bottomRightOffsetPos = previousPos - increment + bottomRightDirection * offsetLength;

        vec3 upPos = findSurfacePos(upOffsetPos, increment * 4, iterations + 2, materials);
        vec3 bottomLeftPos = findSurfacePos(bottomLeftOffsetPos, increment * 4, iterations + 2, materials);
        vec3 bottomRightPos = findSurfacePos(bottomRightOffsetPos, increment * 4, iterations + 2, materials);

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

void main() {
    // Check if the fragment is within the clipping planes
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

    vec3 samplePos = frontFacesPos; // Start position of the ray
    vec3 increment = stepSize * normalize(directionRay);

    vec4 color = vec4(0.0);
    float[5] materials = float[5](0.0, 0.0, 0.0, 0.0, 0.0);
    vec3[5] samplePositions = vec3[5](samplePos, samplePos, samplePos, samplePos, samplePos);

    float t = 0.0;
    // Walk from front to back
    while (t <= lengthRay + 2 * stepSize) {
        float newMaterial = getNewMaterial(samplePos, samplePositions[4], materials[4]); // Get the material ID for the current sample position  

        // Update the arrays
        updateArrays(materials, samplePositions, newMaterial, samplePos);

        if (t > stepSize * 2) { // Initialize the first two positions of the array first
            float previousMaterial = materials[1];
            float currentMaterial = getMaterialID(materials);

            vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) * invMatTexSize);

            // If we have a surface, add shading to it by finding its normal
            if (useShading && previousMaterial != currentMaterial && sampleColor.a > 0.01) {
                int iterations = 10;
                vec3 surfacePos = findSurfacePos(samplePositions[1], increment, iterations, materials);
                sampleColor = applyShading(samplePositions[1], increment, directionRay, sampleColor, surfacePos, iterations, materials);
            } else if (previousMaterial == currentMaterial) {
                sampleColor.a *= stepSize; // Compensate for the step size
            }

            // Perform alpha compositing (front to back)
            vec3 outRGB = color.rgb + (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
            float outAlpha = color.a + (1.0 - color.a) * sampleColor.a;
            color = vec4(outRGB, outAlpha);

            // Early stopping condition
            if (color.a >= 1.0) {
                break;
            }
        }

        t += stepSize;
        if (t < lengthRay) { // Continue past the ray length but pad with the last material
            samplePos += increment;
        }
    }
    FragColor = color;
}
