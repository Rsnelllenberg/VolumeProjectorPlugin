#version 430
layout(location = 0) out vec4 FragColor;

// Sampler uniforms:
uniform isampler2D rayIDTexture;    // for each pixel, a value that is either a valid ray sample ID or -1 if not used it is a 16-bit int texture
uniform sampler2D tfTexture;        // a 2D lookup texture that maps mean positions (e.g. from a transfer function) to a color.

// Uniforms to convert screen coordinates into normalized texture coordinates.
uniform vec2 invFaceTexSize;    // 1.0 / (face texture width, face texture height)
uniform vec2 invTfTexSize;      // 1.0 / (transfer function texture size)
uniform int numRays;            // The number of rays in the batch (this is the same for all rays in the batch)
uniform float stepSize;         // The step size used transparancy modulation


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
    for (int i = 0; i < count; i++)
    {
        int idx = startIndex + i;
        vec2 pos = vec2(meanPositions[idx * 2], meanPositions[idx * 2 + 1]); // Multiply by two to convert pixel id to float pos in the meanPositions array

        // Use this position to fetch a color from the transfer function.
        vec4 sampleColor = texture(tfTexture, pos * invTfTexSize);
        sampleColor.a *= stepSize; // Compensate for the step size

        // Composite using front-to-back alpha blending.
        color.rgb += (1.0 - color.a) * sampleColor.a * sampleColor.rgb;
        color.a += (1.0 - color.a) * sampleColor.a;

        
        // If the accumulated opacity is complete, we can stop early.
        if (color.a >= 1.0)
            break;
    }
    
    FragColor = color;
}