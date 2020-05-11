﻿#include "gpu_shader.h" 
#include "gpu.h"

namespace ps1e {


#define VertexShaderHeader R"shader(
#version 330 core
uniform uint frame_width;
uniform uint frame_height;
uniform int offx;
uniform int offy;
uniform float transparent;

vec4 color_ps2gl(uint pscolor) {
  uint  x = 0xFFu;
  float r = ((pscolor      & x) / 255.0f);
  float g = ((pscolor>> 8) & x) / 255.0f;
  float b = ((pscolor>>16) & x) / 255.0f;
  return vec4(r, g, b, transparent);
}

float get_x(uint ps_pos) {
  uint t = (0xFFFFu & ps_pos);
  uint s = t & 0x400u;
  int a  = int(t) & 0x3FF;
  if (s != 0u) a = ~(a) + 1;
  int x = a + offx;
  return float(x) / frame_width  * 2 - 1;
}

float get_y(uint ps_pos) {
  uint t = (0xFFFFu & (ps_pos>>16u));
  uint s = t & 0x400u;
  int a  = int(t) & 0x3FF;
  if (s != 0u) a = ~(a) + 1;
  int y = a + offy;
  return -(float(y) / frame_height  * 2 - 1);
}

vec2 to_textcoord(uint coord, uint page) {
  uint pagex = (page & 0x0Fu) * 64u;
  uint pagey = (1u & (page >> 4)) * 256u;
  uint x = (coord & 0x0FFu) + pagex;
  uint y = ((coord >> 8) & 0x0FFu) + pagey;
  return vec2(float(x)/frame_width, 1- float(y)/frame_height);
}
)shader"


#define TextureFragShaderHeader R"shader(
#version 330 core
uniform uint page;
uniform uint clut;

vec4 mix_color(vec4 b, vec4 f) {
  switch (int(page >> 5) & 0x03) {
    default:
    case 0: return b * 0.5 + f * 0.5;
    case 1: return b + f;
    case 3: return b - f;
    case 4: return b + f * 0.25;
  }
}

vec4 texture_mode(sampler2D text, vec2 coord) {
  switch (int(page >> 7) & 0x03) {
    case 0: // 4bit

    case 1: // 8bit

    default:
    case 2: // 16bit
      return texture(text, coord);
  }
}
)shader"

// ---------- ---------- Mono color polygon shader

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

// ---------- ---------- Mono color with texture polygon shader

static ShaderSrc texture_mono_color_poly_v = VertexShaderHeader R"shader(
layout (location = 0) in uint pos;
layout (location = 1) in uint coord;
uniform uint ps_color;
uniform uint page;
uniform uint clut;
out vec4 color;
out vec2 oCoord;

void main() {
  gl_Position = vec4(get_x(pos), get_y(pos), 0, 1.0);
  color = color_ps2gl(ps_color);
  oCoord = to_textcoord(coord, page);
}
)shader";


static ShaderSrc texture_color_f = TextureFragShaderHeader R"shader(
out vec4 FragColor;
in vec4 color;
in vec2 oCoord;
uniform sampler2D text;
  
void main() {
  vec4 c = mix_color(texture_mode(text, oCoord), color);
  FragColor = vec4(c.rgb, color.a);
}
)shader";


static ShaderSrc texture_only_f = TextureFragShaderHeader R"shader(
out vec4 FragColor;
in vec4 color;
in vec2 oCoord;
uniform sampler2D text;
  
void main() {
  vec4 c = texture_mode(text, oCoord);
  FragColor = vec4(c.rgb, color.a);
}
)shader";

// ---------- ---------- Shader for virtual ps ram (frame buffer)

static ShaderSrc draw_virtual_screen_vertex = R"shader(
#version 330 core
layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 coord;
out vec2 TexCoord;

void main() {
  gl_Position = vec4(pos.x, pos.y, 0, 1.0);
  TexCoord = coord;
}
)shader";


static ShaderSrc draw_virtual_screen_frag = R"shader(
#version 330 core
out vec4 FragColor;
uniform sampler2D text;
in vec2 TexCoord;
  
void main() {
  FragColor = texture(text, TexCoord);
}
)shader";


ShaderSrc MonoColorPolygonShader::vertex = mono_color_polygon_vertex;
ShaderSrc MonoColorPolygonShader::frag = color_polygon_frag;

ShaderSrc VirtualScreenShader::vertex = draw_virtual_screen_vertex;
ShaderSrc VirtualScreenShader::frag = draw_virtual_screen_frag;

ShaderSrc MonoColorTexturePolyShader::vertex = texture_mono_color_poly_v;
ShaderSrc MonoColorTexturePolyShader::frag_with_color = texture_color_f;
ShaderSrc MonoColorTexturePolyShader::frag_no_color = texture_only_f;

}