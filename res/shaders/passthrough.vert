#version 150

#extension GL_ARB_explicit_attrib_location : enable 
#
layout ( location = 0 ) in vec2 a_position;
out vec4 v_position;
out vec2 v_texcoord;

void main(void)
{
    v_texcoord.y = a_position.y;
    v_texcoord.x = (a_position.x + 1.0) * 0.5;
    v_position  = vec4(a_position, 0., 1.);
    gl_Position = v_position;
}
