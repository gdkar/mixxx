#version 130

uniform sampler2D u_texture;

in vec2 v_texcoord;
void main(void)
{
    gl_FragColor = texture2D(u_texture, v_texcoord);//gl_TexCoord[0].xy);
}
