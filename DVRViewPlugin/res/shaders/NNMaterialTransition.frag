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
uniform float stepSize;
uniform vec3 camPos; 
uniform vec3 lightPos;

// Clipping planes
uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

uniform bool useShading;

// Sample the volume at a given position and return the material ID
float sampleVolume(vec3 samplePos) {
    vec3 volPos = samplePos * invDimensions;
    return texture(volumeData, volPos).r;
}

// Get the material at a given sample position, with smoothing for transitions
float getNewMaterial(vec3 samplePos) {
    float voxelMaterial = sampleVolume(samplePos);
    vec3 interVoxelPos = fract(samplePos);
    vec3 voxelSide = sign(interVoxelPos - 0.5);

    float materialX = sampleVolume(samplePos + vec3(voxelSide.x, 0.0, 0.0));
    float materialY = sampleVolume(samplePos + vec3(0.0, voxelSide.y, 0.0));
    float materialZ = sampleVolume(samplePos + vec3(0.0, 0.0, voxelSide.z));

    float materialXY = sampleVolume(samplePos + vec3(voxelSide.x, voxelSide.y, 0.0));
    float materialXZ = sampleVolume(samplePos + vec3(voxelSide.x, 0.0, voxelSide.z));
    float materialYZ = sampleVolume(samplePos + vec3(0.0, voxelSide.y, voxelSide.z));

    // Check for no smoothing needed in each axis direction
    if(materialX == voxelMaterial && (materialXY == voxelMaterial || materialXZ == voxelMaterial)) {
        return voxelMaterial;
    }
    if (materialY == voxelMaterial && (materialXY == voxelMaterial || materialYZ == voxelMaterial)) {
        return voxelMaterial;
    }
    if (materialZ == voxelMaterial && (materialXZ == voxelMaterial || materialYZ == voxelMaterial)) {
        return voxelMaterial;
    }

    // If two neighbors are the same and different from the current, smooth the transition
    if (materialX == materialY && materialX != voxelMaterial) { 
        if (materialXY == materialX){
            if( (voxelSide.x * interVoxelPos.x) + (voxelSide.y * interVoxelPos.y) >= 0.5 + 0.5 * (voxelSide.x + voxelSide.y)) {
                return materialX;
            }
        }
    }
    if (materialX == materialZ && materialX != voxelMaterial) {
        if (materialXZ == materialX){
            if( (voxelSide.x * interVoxelPos.x) + (voxelSide.z * interVoxelPos.z) >= 0.5 + 0.5 * (voxelSide.x + voxelSide.z)) {
                return materialX;
            }
        }
    }
    if (materialY == materialZ && materialY != voxelMaterial) {
        if (materialYZ == materialY){
            if( (voxelSide.y * interVoxelPos.y) + (voxelSide.z * interVoxelPos.z) >= 0.5 + 0.5 * (voxelSide.y + voxelSide.z)) {
                return materialY;
            }
        }
    }

    // Default: return the material ID of the sampled voxel
    return voxelMaterial;
}

// Find the position of a material transition (surface) along a ray using binary search
vec3 findSurfacePos(vec3 startPos, vec3 direction, int iterations, float previousMaterial, float currentMaterial) {
    vec3 lowPos = startPos;
    vec3 highPos = startPos + direction;
    float epsilon = 0.01; // Small value to handle precision issues
    bool foundSurface = false;
    for (int i = 0; i < iterations; ++i) {
        vec3 midPos = (lowPos + highPos) * 0.5;
        float midMaterial = getNewMaterial(midPos);
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

// Apply Phong shading at a surface position, using the local normal and lighting
vec4 applyShading(
    vec3 previousPos,
    vec3 increment,
    vec3 directionRay,
    vec4 sampleColor,
    vec3 surfacePos,
    int iterations,
    float previousMaterial,
    float currentMaterial
) {
    // Check if a clipping plane was hit
    vec3 minfaces = 1.0 + sign(u_minClippingPlane - (previousPos * invDimensions));
    vec3 maxfaces = 1.0 + sign((previousPos * invDimensions) - u_maxClippingPlane);

    // Compute the surface normal (normalize later)
    vec3 surfaceGradient = maxfaces - minfaces;

    vec3 normal;
    int invalidCount = 0;
    // If on clipping plane, calculate color and return it
    if (!all(equal(surfaceGradient, vec3(0).xyz))) {
        normal = normalize(surfaceGradient);
    } else {
        // Find a surface triangle by finding three other closeby points on the surface
        vec3 upDirection = normalize(cross(directionRay, vec3(0.0, 1.0, 0.0)));
        vec3 bottomLeftDirection = normalize(cross(directionRay, vec3(-1.0, -1.0, 0.0)));
        vec3 bottomRightDirection = normalize(cross(directionRay, vec3(1.0, 1.0, 0.0)));

        float offsetLength = 0.2f;

        // Define the offset positions
        vec3 upOffsetPos = previousPos - increment + upDirection * offsetLength;
        vec3 bottomLeftOffsetPos = previousPos - increment + bottomLeftDirection * offsetLength;
        vec3 bottomRightOffsetPos = previousPos - increment + bottomRightDirection * offsetLength;

        // Find the surface positions for normal calculation
        vec3 upPos = findSurfacePos(upOffsetPos, increment * 4, iterations + 2, previousMaterial, currentMaterial);
        vec3 bottomLeftPos = findSurfacePos(bottomLeftOffsetPos, increment * 4, iterations + 2, previousMaterial, currentMaterial);
        vec3 bottomRightPos = findSurfacePos(bottomRightOffsetPos, increment * 4, iterations + 2, previousMaterial, currentMaterial);

        // Check if any of the positions are invalid
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
        float specular = pow(max(dot(viewDir, reflectDir), 0.0), 64.0); // Specular exponent

        vec3 phongColor = ambient + diffuse + specular;
        sampleColor.rgb = phongColor;
    }
    return sampleColor;
}

void main() {
    // Compute normalized texture coordinates for this fragment
    vec2 normTexCoords = gl_FragCoord.xy * invFaceTexSize;

    // Get the entry and exit positions of the ray in volume space
    vec3 frontFacesPos = texture(frontFaces, normTexCoords).xyz * dimensions;
    vec3 backFacesPos = texture(backFaces, normTexCoords).xyz * dimensions;

    // If the ray does not enter the volume, output transparent
    if(frontFacesPos == backFacesPos) {
        FragColor = vec4(0.0);
        return;
    }

    // Compute ray direction and length
    vec3 directionSample = backFacesPos - frontFacesPos;
    vec3 directionRay = normalize(directionSample);
    float lengthRay = length(directionSample);

    // Initialize ray marching variables
    vec3 samplePos = frontFacesPos;
    vec3 increment = stepSize * directionRay;

    vec4 color = vec4(0.0);

    // Track the previous and current material and positions
    float previousMaterial = 0.0;
    float currentMaterial = getNewMaterial(samplePos);

    vec3 previousPos = samplePos;
    vec3 currentPos = samplePos;

    float t = 0.0;
    // Walk from front to back through the volume
    while (t <= lengthRay + 2 * stepSize) {
        previousMaterial = currentMaterial;
        previousPos = currentPos;

        currentPos = samplePos;
        currentMaterial = getNewMaterial(currentPos);

        if (t > stepSize * 2) { // Skip the first two steps to initialize previous/current
            // Look up the color for the current and previous material
            vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) * invMatTexSize);

            // If we have a surface, add shading to it by finding its normal
            if (useShading && previousMaterial != currentMaterial && sampleColor.a > 0.01) {
                int iterations = 10;
                vec3 surfacePos = findSurfacePos(previousPos, increment, iterations, previousMaterial, currentMaterial);
                sampleColor = applyShading(previousPos, increment, directionRay, sampleColor, surfacePos, iterations, previousMaterial, currentMaterial);
            }

            // If the material did not change, scale alpha by the step length
            if (currentMaterial == previousMaterial) {
                sampleColor.a *= length(currentPos - previousPos);
            }

            // Perform alpha compositing (front to back)
            vec3 outRGB = color.rgb + (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
            float outAlpha = color.a + (1.0 - color.a) * sampleColor.a;
            color = vec4(outRGB, outAlpha);

            // Early stopping condition if fully opaque
            if (color.a >= 1.0) {
                break;
            }
        }

        t += stepSize;
        if (t < lengthRay) {
            samplePos += increment;
        }
    }
    FragColor = color;
}
