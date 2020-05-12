﻿#include "gpu.h"
#include "gpu_shader.h"
#include <functional>
#include <stdexcept>

namespace ps1e {

// 实际上 mirror_case 的定义与对应的 case 在渲染上有出入
// 可以用恶魔城进行测试, 进一步确定渲染方式.
#define mirror_case(x)  case x


class VerticesBase {
protected:
  int step;
  const int element;

public:
  VerticesBase(int ele, int st = 0) : step(st), element(ele) {}
  virtual ~VerticesBase() {}

  int elementCount() {
    return element;
  }
};


template<int ElementCount>
class PolygonU32Vertices : public VerticesBase {
private:
  u32 vertices[ElementCount];

public:
  u32 color;

  PolygonU32Vertices() : VerticesBase(ElementCount, -1) {}

  void setAttr(GLVerticesBuffer& vbo) {
    GLBufferData vbdata(vbo, vertices, sizeof(vertices));
    vbdata.uintAttr(0, 1, 1);
  }

  bool write(const u32 c) {
    switch (step) {
      case -1:
        color = c;
        break;

      default: {
        vertices[step] = c;
        break;
      }
    }
    return ++step < ElementCount;
  }
};


template<int ElementCount>
class PolyTextureVertices : public VerticesBase {
private:
  u32 vertices[ElementCount *2];

public:
  u32 color;
  u32 clut;
  u32 page;

  PolyTextureVertices() : VerticesBase(ElementCount, -1) {}

  void setAttr(GLVerticesBuffer& vbo) {
    GLBufferData vbdata(vbo, vertices, sizeof(vertices));
    vbdata.uintAttr(0, 1, 2, 0);
    vbdata.uintAttr(1, 1, 2, 1);
  }

  bool write(const u32 c) {
    switch (step) {
      case -1:
        color = c;
        break;

      default: {
        switch (step) {
          case 1:
            clut = (c & 0xFFFF'0000)>>16;
            break;
          case 3:
            page = (c & 0xFFFF'0000)>>16;
            break;
        }
        vertices[step] = c;
        break;
      }
    }
    return ++step < (ElementCount *2);
  }
};


template<int ElementCount>
class ShadedPolyVertices : public VerticesBase {
private:
  u32 vertices[ElementCount *2];

public:
  ShadedPolyVertices() : VerticesBase(ElementCount) {}

  void setAttr(GLVerticesBuffer& vbo) {
    GLBufferData vbdata(vbo, vertices, sizeof(vertices));
    vbdata.uintAttr(0, 1, 2, 1);
    vbdata.uintAttr(1, 1, 2, 0);
  }

  bool write(const u32 c) {
    vertices[step] = c;
    return ++step < (ElementCount *2);
  }
};


template<int ElementCount>
class ShadedPolyWithTextureVertices : public VerticesBase {
private:
  u32 vertices[ElementCount *3];

public:
  u32 clut;
  u32 page;

  ShadedPolyWithTextureVertices() : VerticesBase(ElementCount) {}

  void setAttr(GLVerticesBuffer& vbo) {
    GLBufferData vbdata(vbo, vertices, sizeof(vertices));
    vbdata.uintAttr(0, 1, 3, 1);
    vbdata.uintAttr(1, 1, 3, 0);
    vbdata.uintAttr(2, 1, 3, 2);
  }

  bool write(const u32 c) {
    vertices[step] = c;
    switch (step) {
      case 2:
        clut = (c & 0xFFFF'0000)>>16;
        break;

      case 5:
        page = (c & 0xFFFF'0000)>>16;
        break;
    }
    return ++step < (ElementCount *3);
  }
};


class MonoLineFixVertices : public VerticesBase {
private:
  u32 vertices[2];

public:
  u32 color;

  MonoLineFixVertices() : VerticesBase(2, -1) {}

  void setAttr(GLVerticesBuffer& vbo) {
    GLBufferData vbdata(vbo, vertices, sizeof(vertices));
    vbdata.uintAttr(0, 1, 1, 0);
  }

  bool write(const u32 c) {
    switch (step) {
      case -1:
        color = c;
        break;

      default:
        vertices[step] = c;
    }
    return ++step < 2;
  }
};


class MultipleVertices {
public:
  const u32 END = 0x50005000;
  const int InitBufSize = 0x10;

protected:
  u32 *vertices;

private:
  int capacity;
  int count;
  const int mincount;

  void resize(int size) {
    if (capacity < size) {
      u32 *mm = (u32*) realloc(vertices, size * sizeof(u32));
      if (!mm) {
        throw std::runtime_error("Failed allocated memory");
      }
      vertices = mm;
      capacity = size;
    }
  }

public:
  u32 color;

  MultipleVertices(int initCount, int terminatMin) : 
      capacity(0), count(initCount), mincount(terminatMin)
  {
    capacity = InitBufSize;
    vertices = (u32*) malloc(capacity * sizeof(u32));
    if (!vertices) {
      throw std::runtime_error("Failed allocated memory");
    }
  }

  ~MultipleVertices() {
    free(vertices);
    vertices = 0;
    count = 0;
    capacity = 0;
  }

  int elementCount() {
    return count;
  }

  bool write(const u32 c) {
    // 55555555h ? 50005000h ??
    if ((count >= mincount) && (c & 0xF000F000)==END) {
      return false;
    }

    writeVertices(count, c);

    if (++count >= capacity) {
      resize(capacity << 1);
    }
    return true;
  }

  inline size_t vsize() {
    return sizeof(u32) * count;
  }

  virtual void writeVertices(int count, const u32 data) = 0;
};


class MonoLineMulVertices : public MultipleVertices {
public:
  MonoLineMulVertices() : MultipleVertices(-1, 2) {}

  void setAttr(GLVerticesBuffer& vbo) {
    GLBufferData vbdata(vbo, vertices, vsize());
    vbdata.uintAttr(0, 1, 1, 0);
  }

  void writeVertices(int count, const u32 data) {
    switch (count) {
      case -1:
        color = data;
        break;

      default:
        vertices[count] = data;
    }
  }
};


class ShadedLineMulVertices : public MultipleVertices {
public:
  ShadedLineMulVertices() : MultipleVertices(0, 3) {}

  void setAttr(GLVerticesBuffer& vbo) {
    GLBufferData vbdata(vbo, vertices, vsize());
    vbdata.uintAttr(0, 1, 2, 1);
    vbdata.uintAttr(1, 1, 2, 0);
  }

  void writeVertices(int count, const u32 data) {
    vertices[count] = data;
  }

  int elementCount() {
    return MultipleVertices::elementCount() >> 1;
  }
};


template< class Vertices, 
          void (*Draw)(GLVertexArrays&, int),
          class Shader = MonoColorPolygonShader,
          bool active_texture = false
          >
class Polygon : public IDrawShape {
private:
  Vertices vertices;
  float transparent;

public:
  Polygon(float trans) : transparent(trans) {
  }

  ~Polygon() {
  }

  virtual bool write(const u32 c) {
    return vertices.write(c);
  }

  virtual void draw(GPU& gpu, GLVertexArrays& vao) {
    gl_scope(vao);
    vbo.init(vao);
    gl_scope(vbo);
    vertices.setAttr(vbo);

    auto prog = gpu.useProgram<Shader>();
    prog->setShaderUni(vertices, gpu, transparent);

    if (active_texture) gpu.useTexture().bind();
    Draw(vao, vertices.elementCount());
    if (active_texture) gpu.useTexture().unbind();
  }
};


void drawTriangles(GLVertexArrays& vao, int elementCount) {
  vao.drawTriangles(elementCount);
}


void drawFan(GLVertexArrays& vao, int elementCount) {
  vao.drawTriangleFan(elementCount);
}


void drawTriStrip(GLVertexArrays& vao, int elementCount) {
  vao.drawTriangleStrip(elementCount);
}


void drawQuads(GLVertexArrays& vao, int elementCount) {
  vao.drawQuads(elementCount);
}


void drawLines(GLVertexArrays& vao, int elementCount) {
  vao.drawLineStrip(elementCount);
}
 

bool GPU::GP0::parseCommand(const GpuCommand c) {
  switch (c.cmd) {
    case 0x00:
      debug("Gpu Nop?\n");
      return false;

    case 0x1F:
      p.status.irq_on = 1;
      return false;

    case 0xE1:
      p.status.v = SET_BIT(p.status.v, 0b11'1111'1111, c.parm);
      p.status.text_off = (c.parm >> 11) & 1;
      p.status.inter_f  = (c.parm >> 13) & 1;
      p.text_flip.x     = (c.parm >> 12) & 1;
      p.text_flip.y     = (c.parm >> 13) & 1;
      p.dirtyAttr();
      return false;

    case 0xE2:
      p.text_win.v = c.parm;
      p.dirtyAttr();
      return false;

    case 0xE3: 
      p.draw_tp_lf.v = c.parm;
      p.dirtyAttr();
      return false;

    case 0xE4: 
      p.draw_bm_rt.v = c.parm;
      p.dirtyAttr();
      return false;

    case 0xE5:
      p.draw_offset.v = c.parm;
      p.dirtyAttr();
      return false;

    case 0xE6:
      p.status.mask = c.parm & 1;
      p.status.enb_msk = (c.parm >> 1) & 1;
      p.dirtyAttr();
      return false;
      
    case 0x20: mirror_case(0x21):
      shape = new Polygon<PolygonU32Vertices<3>, drawTriangles>(1);
      break;

    case 0x22: mirror_case(0x23):
      shape = new Polygon<PolygonU32Vertices<3>, drawTriangles>(0.5);
      break;

    case 0x28: mirror_case(0x29):
      shape = new Polygon<PolygonU32Vertices<4>, drawQuads>(1);
      break;

    case 0x2A: mirror_case(0x2B):
      shape = new Polygon<PolygonU32Vertices<4>, drawQuads>(0.5);
      break;

    case 0x24:
      shape = new Polygon<PolyTextureVertices<3>, 
                  drawTriangles, MonoColorTexturePolyShader, true>(1);
      break;

    case 0x25:
      shape = new Polygon<PolyTextureVertices<3>, 
                  drawTriangles, TextureOnlyPolyShader, true>(1);
      break;

    case 0x26:
      shape = new Polygon<PolyTextureVertices<3>, 
                  drawTriangles, MonoColorTexturePolyShader, true>(0.5);
      break;

    case 0x27:
      shape = new Polygon<PolyTextureVertices<3>, 
                  drawTriangles, TextureOnlyPolyShader, true>(0.5);
      break;

    case 0x2C:
      shape = new Polygon<PolyTextureVertices<4>, 
                  drawTriangles, MonoColorTexturePolyShader, true>(1);
      break;

    case 0x2D:
      shape = new Polygon<PolyTextureVertices<4>, 
                  drawTriangles, TextureOnlyPolyShader, true>(1);
      break;

    case 0x2E:
      shape = new Polygon<PolyTextureVertices<4>, 
                  drawTriangles, MonoColorTexturePolyShader, true>(0.5);
      break;

    case 0x2F:
      shape = new Polygon<PolyTextureVertices<4>, 
                  drawTriangles, TextureOnlyPolyShader, true>(0.5);
      break;

    case 0x30: mirror_case(0x31):
      shape = new Polygon<ShadedPolyVertices<3>, 
                  drawTriangles, ShadedPolyShader, false>(1);
      break;

    case 0x32: mirror_case(0x33):
      shape = new Polygon<ShadedPolyVertices<3>, 
                  drawTriangles, ShadedPolyShader, false>(0.5);
      break;

    case 0x38: mirror_case(0x39):
      shape = new Polygon<ShadedPolyVertices<4>, 
                  drawQuads, ShadedPolyShader, false>(1);
      break;

    case 0x3A: mirror_case(0x3B):
      shape = new Polygon<ShadedPolyVertices<4>, 
                  drawQuads, ShadedPolyShader, false>(0.5);
      break;

    case 0x34: mirror_case(0x35):
      shape = new Polygon<ShadedPolyWithTextureVertices<3>,
                  drawTriangles, ShadedPolyWithTextShader, true>(1);
      break;

    case 0x36: mirror_case(0x37):
      shape = new Polygon<ShadedPolyWithTextureVertices<3>,
                  drawTriangles, ShadedPolyWithTextShader, true>(0.5);
      break;

    case 0x3C: mirror_case(0x3D):
      shape = new Polygon<ShadedPolyWithTextureVertices<4>,
                  drawQuads, ShadedPolyWithTextShader, true>(1);
      break;

    case 0x3E: mirror_case(0x3F):
      shape = new Polygon<ShadedPolyWithTextureVertices<4>,
                  drawQuads, ShadedPolyWithTextShader, true>(0.5);
      break;

    case 0x40:
      shape = new Polygon<MonoLineFixVertices, 
                  drawLines, MonoColorPolygonShader, false>(1);
      break;

    case 0x42:
      shape = new Polygon<MonoLineFixVertices, 
                  drawLines, MonoColorPolygonShader, false>(0.5);
      break;

    case 0x48:
      shape = new Polygon<MonoLineMulVertices, 
                  drawLines, MonoColorPolygonShader, false>(1);
      break;

    case 0x4A:
      shape = new Polygon<MonoLineMulVertices, 
                  drawLines, MonoColorPolygonShader, false>(0.5);
      break;

    case 0x50: 
      shape = new Polygon<ShadedPolyVertices<2>, 
                  drawLines, ShadedPolyShader, false>(1);
      break;

    case 0x52: 
      shape = new Polygon<ShadedPolyVertices<2>, 
                  drawLines, ShadedPolyShader, false>(0.5);
      break;

    case 0x58:
      shape = new Polygon<ShadedLineMulVertices, 
                  drawLines, ShadedPolyShader, false>(1);
      break;

    default:
      error("Invaild GP0 Command %x %x\n", c.cmd, c.v);
      return false;
  }
  return true;
}


u32 GPU::GP0::read() {
  if (p.read_queue.size()) {
    IGpuReadData* data_que = p.read_queue.front();
    last_read = data_que->read();
    if (!data_que->has()) {
      delete data_que;
      p.read_queue.pop_front();
    }
  }
  return last_read;
}


void GPU::GP0::write(u32 v) {
  //debug("GP0 Write 0x%08x\n", v);
  switch (stage) {
    case ShapeDataStage::read_command:
      if (!parseCommand(v)) {
        break;
      }
      stage = ShapeDataStage::read_data;
      // do not break

    case ShapeDataStage::read_data:
      if (!shape->write(v)) {
        p.send(shape);
        shape = NULL;
        stage = ShapeDataStage::read_command;
      }
      break;
  }
}


void GPU::GP0::reset_fifo() {
  stage = ShapeDataStage::read_command;
  if (shape) {
    delete shape;
    shape = NULL;
  }
}


GPU::GP0::GP0(GPU &_p) : p(_p), stage(ShapeDataStage::read_command), shape(0), last_read(0) {
}


}