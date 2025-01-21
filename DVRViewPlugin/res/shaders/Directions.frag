#version 330
out vec4 FragColor;

in vec3 u_color;

uniform sampler2D frontfaces;
uniform vec3 dimensions;

void main()
{
    vec2 texSize = textureSize(frontfaces, 0);

    vec2 normTexCoords = gl_FragCoord.xy / texSize;    

	vec3 frontface = vec3(texture(frontfaces, normTexCoords)) * dimensions;
    vec3 backface = u_color * dimensions; 

    vec3 colorDifference = backface - frontface;
    FragColor = vec4(normalize(colorDifference), length(colorDifference));
    //FragColor = backface;
}