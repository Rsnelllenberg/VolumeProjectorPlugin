#version 430
layout(location = 0) out vec4 FragColor;


// Uniforms for fallback sampling in case of noisy data
uniform sampler2D frontFaces;  // Contains the front face positions (in [0,1], scaled by dataDimensions)
uniform sampler2D backFaces;   // Contains the back face positions (in [0,1], scaled by dataDimensions)
uniform vec3 dataDimensions;   // The volume dataset dimensions

// Sampler uniforms:
uniform isampler2D rayIDTexture;    // for each pixel, a value that is either a valid ray sample ID or -1 if not used it is a 16-bit int texture
uniform sampler2D materialTexture; // the material table, index 0 is no material present (air), the tfTexture should have the same
uniform sampler2D tfTexture;
uniform sampler3D materialVolumeData; // the volume data texture, used to sample the nearest voxel center for the materialID

// Uniforms to convert screen coordinates into normalized texture coordinates.
uniform vec2 invFaceTexSize;    // 1.0 / (face texture width, face texture height)
uniform vec2 invTfTexSize;      // 1.0 / (transfer function texture size)
uniform vec2 invMatTexSize;     // Pre-divided matTexSize (1.0 / matTexSize)
uniform int numRays;            // The number of rays in the batch (this is the same for all rays in the batch)
uniform float stepSize;         // The step size used for the ray marching, this is used to compensate for the alpha blending in the shader

uniform bool useClutterRemover; // If true, the clutter remover is used to smoothen the visualization

// holds the start index for the meanPositions array for each rayID (rayIDs are decided per batch and have no relation to the pixelPos) 
// The values it holds are the samplePosition, since each meanPos is a vec2, the start index needs to be multiplied by 2 in the shader
// It also holds that total number of samples at the end such that the final ray length can also be calculated
layout(std430, binding = 1) buffer SampleMapping {
    int sampleStartIndices[];
};

// This uniform block holds the pre-computed mean 2D positions (one vec2 per sample)
layout(std430, binding = 2) buffer MeanPositions {
    float meanPositions[];
};

float getMaterialID(inout float[5] materials, vec3 samplePos) {
    float firstMaterial = materials[0];
    float previousMaterial = materials[1];

    float currentMaterial = materials[2];

    float nextMaterial = materials[3];
    float lastMaterial = materials[4];

    if(useClutterRemover && firstMaterial == previousMaterial && nextMaterial == lastMaterial && currentMaterial != previousMaterial && currentMaterial != nextMaterial){
        currentMaterial = texture(materialVolumeData, samplePos).r + 0.5f;

        // Update the materials array with the new material
        materials[2] = currentMaterial;
    }

    return currentMaterial;
}

void updateArrays(inout float[5] materials, float newMaterial) {
    // Shift all elements one position back
    for (int j = 0; j < 4; ++j) {
        materials[j] = materials[j + 1];
    }
    // Insert the new elements at the last position
    materials[4] = newMaterial;
}


void main()
{   
    // Get normalized texture coordinates in the "face" texture.
    vec2 normTexCoords = gl_FragCoord.xy * invFaceTexSize;
    
    // Lookup the ray ID for this pixel from the rayID texture.
    // (The rayID texture returns a real number; we round to the nearest int.)
    int rayID = int(texture(rayIDTexture, normTexCoords).r);
    if (rayID < 0) // This pixel is not used in the batch, so discard it.
    {
        discard;
    }

    // Retrieve the sample mapping for this pixel.
    int startIndex = sampleStartIndices[rayID];
    int count = sampleStartIndices[rayID + 1] - startIndex;

    // Composite the color by iterating over the associated 2D sample positions.
    vec4 color = vec4(0.0);
    float previousMaterial = 0;

    float[5] materials = float[5](0.0, 0.0, 0.0, 0.0, 0.0);

    // Sample the front and back face textures.
    // The textures contain values in the [0,1] range scaled by dataDimensions.
    vec3 frontPos = texture(frontFaces, normTexCoords).xyz * dataDimensions;
    vec3 backPos  = texture(backFaces, normTexCoords).xyz * dataDimensions;

    // If the ray is degenerate (e.g. the ray does not hit the volume), skip processing.
    if (all(equal(frontPos, backPos)))
        return;

    // Compute the normalized ray direction and its length.
    vec3 rayDir = normalize(backPos - frontPos);
    vec3 increment = rayDir * stepSize;

    for (int i = 0; i < count; i++)
    {
        int idx = startIndex + i;
        vec2 pos = vec2(meanPositions[idx * 2], meanPositions[idx * 2 + 1]); // Multiply by two to convert pixel id to float pos in the meanPositions array

        float newMaterial = texture(tfTexture, pos * invTfTexSize).r + 0.5f;

        // Update the arrays
        updateArrays(materials, newMaterial);

        if(i > 2){ // initialize the first two positions of the array first
            
             vec3 samplePos = frontPos + i * increment;

            // Get the current material
            float previousMaterial = materials[1];
            float currentMaterial = getMaterialID(materials, samplePos);

            vec4 sampleColor = texture(materialTexture, vec2(currentMaterial, previousMaterial) * invMatTexSize);
            
            if (previousMaterial == currentMaterial) {   
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
    }
    FragColor = color;
}