#version 150

out vec2 v_texcoord;

void main(void)
{
    v_texcoord = gl_MultiTexCoord0;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

