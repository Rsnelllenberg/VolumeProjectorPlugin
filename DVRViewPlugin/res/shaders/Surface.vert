#version 330
layout(location = 0) in vec3 pos;

uniform mat4 u_modelViewProjection;
uniform mat4 u_model; 
uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;
uniform vec3 renderCubeSize;

uniform samplerBuffer renderCubePositions; // as vec3 per cube (x,y,z) where x,y,z are the cube's offset in the cube grid
uniform samplerBuffer renderCubeOccupancy; // formatted vec4 per tf rectangle topLeft(x,y) and bottomRight(x,y)
uniform sampler2D tfRectangleData;         // as a float per texel representing the summed areaTable of the tf rectangle

uniform int renderType;
uniform bool useEmptySpaceSkipping;

out vec3 u_color;
out vec3 worldPos; 



void main() {
    if (any(lessThan(u_minClippingPlane, u_maxClippingPlane))) { // if the min clipping plane is less than the max clipping plane
        vec3 cubeOffset = texelFetch(renderCubePositions, gl_InstanceID).xyz; 
        if(useEmptySpaceSkipping){
            // Compute the world space position 
            vec3 smallestCorner = cubeOffset * renderCubeSize;

            // Check if the instance is outside the bounds
            bvec3 lowerBoundCheck = lessThan(smallestCorner + renderCubeSize, u_minClippingPlane);
            bvec3 upperBoundCheck = lessThan(u_maxClippingPlane, smallestCorner);

            vec4 cubeOccupancyBounds = texelFetch(renderCubeOccupancy, gl_InstanceID);

            // Check the summed areaTable if the cube contains any non transparent values
            float topLeftSum = texture(tfRectangleData, cubeOccupancyBounds.xy).x;
            float bottomLeftSum = texture(tfRectangleData, vec2(cubeOccupancyBounds.x, cubeOccupancyBounds.w)).x;
            float topRightSum = texture(tfRectangleData, vec2(cubeOccupancyBounds.z, cubeOccupancyBounds.y)).x;
            float bottomRightSum = texture(tfRectangleData, cubeOccupancyBounds.zw).x;

            bool containRelevantValue = topLeftSum + bottomRightSum - topRightSum - bottomLeftSum > 0.0;

            if (any(lowerBoundCheck) || any(upperBoundCheck) || !containRelevantValue) {
                gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // Set to a position outside the normalized device coordinates (NDC) [-1, 1] range, effectively discarding the vertex as you can't just discard vertices
                return;
            }
        }

        vec3 actualPos = clamp((pos + cubeOffset) * renderCubeSize, u_minClippingPlane, u_maxClippingPlane);
        worldPos = (u_model * vec4(actualPos, 1.0)).xyz;
        gl_Position = u_modelViewProjection * vec4(actualPos, 1.0);
        u_color = actualPos;
        
    }
}
