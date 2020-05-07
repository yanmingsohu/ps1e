#include "gpu_shader.h"
#include "gpu.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace ps1e {


#define VertexShaderHeader R"shader(
#version 330 core
uniform uint width;
uniform uint height;
uniform float transparent;

vec4 color_ps2gl(uint pscolor) {
  uint  x = 0xFFu;
  float r = ((pscolor      & x) / 255.0f);
  float g = ((pscolor>> 8) & x) / 255.0f;
  float b = ((pscolor>>16) & x) / 255.0f;
  return vec4(r, g, b, transparent);
}

float get_x(uint ps_pos) {
  return float(0xFFFFu & ps_pos) / width  * 2 - 1;
}

float get_y(uint ps_pos) {
  return -(float(0xFFFFu & (ps_pos>>16u)) / height * 2 - 1);
}
)shader"


static ShaderSrc mono_color_polygon_vertex = VertexShaderHeader R"shader(
layout (location = 0) in uint pos;
uniform uint ps_color;
out vec4 color;

void main() {
  gl_Position = vec4(get_x(pos), get_y(pos), 0, 1.0);
  color = color_ps2gl(ps_color);
}
)shader";


static ShaderSrc color_polygon_frag = R"shader(
#version 330 core
out vec4 FragColor;
in vec4 color;
  
void main() {
  FragColor = color;
}
)shader";


ShaderSrc MonoColorPolygonShader::vertex = mono_color_polygon_vertex;
ShaderSrc MonoColorPolygonShader::frag = color_polygon_frag;


}