//TES transforms new vertices. Runs once for all output vertices

#version 410
layout(isolines) in; // Controls how tesselator creates new geometry

in vec3 teColour[]; // input colours

out vec3 Colour; // colours to fragment shader
uniform int mode = 1;
uniform float p = 4;

void main()
{
  float u = gl_TessCoord.x;

  float b0 = 1.0-u;
  float b1 = u;

  if (mode == 0) {
    gl_Position = b0 * gl_in[0].gl_Position + b1 * gl_in[1].gl_Position;
    Colour 	= b0 * teColour[0] + b1 * teColour[1];
  }
  if (mode == 1) {
    gl_Position = b0 * b0 * gl_in[0].gl_Position + 2 * b1 * b0 * gl_in[1].gl_Position + b1 * b1 * gl_in[2].gl_Position;
    Colour 	= b0 * teColour[0] + b1 * teColour[2];
  }
  if (mode == 2) {
    gl_Position = b0 * b0 * b0 * gl_in[0].gl_Position + 3 * b0 * b0 * b1 * gl_in[1].gl_Position + 3 * b1 * b1 * b0 * gl_in[2].gl_Position + b1 * b1 * b1 * gl_in[3].gl_Position;
    Colour 	= b0 * teColour[0] + b1 * teColour[3];
  }
}
