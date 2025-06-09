#version 330
layout(location = 0) in vec3 pos;

uniform mat4 u_modelViewProjection;
uniform mat4 u_model; 
uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;
uniform vec3 renderCubeSize;

uniform samplerBuffer renderCubePositions; // as vec3 per cube (x,y,z) where x,y,z are the cube's offset in the cube grid

out vec3 u_color;
out vec3 worldPos; 



void main() {
    if (any(lessThan(u_minClippingPlane, u_maxClippingPlane))) { // if the min clipping plane is less than the max clipping plane
        vec3 cubeOffset = texelFetch(renderCubePositions, gl_InstanceID).xyz; 
        // Compute the world space position 
        vec3 smallestCorner = cubeOffset * renderCubeSize;

        // Check if the instance is outside the bounds
        bvec3 lowerBoundCheck = lessThan(smallestCorner + renderCubeSize, u_minClippingPlane);
        bvec3 upperBoundCheck = lessThan(u_maxClippingPlane, smallestCorner);

        if (any(lowerBoundCheck) || any(upperBoundCheck)) {
            gl_Position = vec4(2.0, 2.0, 2.0, 1.0); // Set to a position outside the normalized device coordinates (NDC) [-1, 1] range, effectively discarding the vertex as you can't just discard vertices
            return;
        }

        vec3 actualPos = clamp((pos + cubeOffset) * renderCubeSize, u_minClippingPlane, u_maxClippingPlane);
        worldPos = (u_model * vec4(actualPos, 1.0)).xyz;
        gl_Position = u_modelViewProjection * vec4(actualPos, 1.0);
        u_color = actualPos;
        
    }
}
