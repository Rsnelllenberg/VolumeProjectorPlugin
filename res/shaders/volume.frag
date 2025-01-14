#version 330
out vec4 FragColor;

in vec3 u_color;

uniform sampler2D givenTexture;

void main()
{
    vec2 texSize = textureSize(givenTexture, 0);

    vec2 normTexCoords = gl_FragCoord.xy / texSize;    

    FragColor = texture(givenTexture, normTexCoords);
}