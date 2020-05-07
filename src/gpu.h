#pragma once

#include <list>

#include "util.h"
#include "dma.h"
#include "bus.h"

struct GLFWwindow;

namespace std {
  class thread;
}

namespace ps1e {

class GPU;
class MonoColorPolygonShader;

typedef u32 GLHANDLE;
typedef const char* ShaderSrc;

union GpuStatus {
  u32 v;
  struct {
    u32 tx        : 4; //0-3 ����ҳ X = ty * 64 (0~3)
    u32 ty        : 1; //  4 ����ҳ Y = ty * 256
    u32 abr       : 2; //5-6 ��͸�� 0{0.5xB+0.5xF}, 1{1.0xB+1.0xF}, 2{1.0xB-1.0xF}, 3{1.0xB+0.25xF}
    u32 tp        : 2; //7-8 ����ҳ��ɫģʽ 0{4bit CLUT}, 1{8bit CLUT}, 2:{15bit}
    u32 dtd       : 1; //  9 1:��������Dither, 24��15
    u32 draw      : 1; // 10 1:�����ͼ���������ʾ����, 0:prohibited
    u32 mask      : 1; // 11 1:����ʱ�޸��ɰ�bit (bit15?)
    u32 enb_msk   : 1; // 12 1:�����ɰ�, ֻ�ܻ��Ƶ��ɰ�����
    u32 inter_f   : 1; // 13 always 1 when GP1(08h).5=0
    u32 distorted : 1; // 14 GP1(08h).7
    u32 text_off  : 1; // 15 1=Disable Textures

    u32 width1    : 1; // 16 width1=1{width0{0:384?368}}
    u32 width0    : 2; // 17-18 ��Ļ���, width1=0{0:256, 1:320, 2:512, 3:640}
    u32 height    : 1; // 19 ��Ļ�߶�, 0:240, 1:480
    u32 video     : 1; // 20 1:PAL, 0:NTSC
    u32 isrgb24   : 1; // 21 1:24bit, 0:15bit
    u32 isinter   : 1; // 22 1:������(����ɨ��)
    u32 display   : 1; // 23 0:������ʾ, 1:����
    u32 irq_on    : 1; // 24 1:IRQ
    u32 r         : 1; // 25 dma ״̬�ܿ���, 1:dma ����
    u32 r_cmd     : 1; // 26 1:���Խ����������� gp0
    u32 r_cpu     : 1; // 27 1:���Է��� vram ���ݵ� cpu
    u32 r_dma     : 1; // 28 1:���Խ��� dma ���ݿ�
    u32 dma_md    : 2; // 29-30 DMA 0:��, 1:δ֪, 2:CPU to GP0, 3:GPU-READ to CPU
    u32 lcf       : 1; // 31 ����ʱ 0:����ż����, 1:����������
  };
};


union GpuCommand {
  u32 v;
  struct {
    u32 parm : 24;
    u32 cmd  : 8;
  };
  GpuCommand(u32 _v) : v(_v) {}
};


struct GpuDataRange {
  u16 width;
  u16 height;

  GpuDataRange(u32 w, u32 h);
};


enum class GpuGp1CommandDef {
  rst_gpu    = 0x00, // reset gpu, sets status to $14802000
  rst_buffer = 0x01, // reset command buffer
  rst_irq    = 0x02, // reset IRQ
  display    = 0x03, // Turn on(1)/off display
  dma        = 0x04, // DMA 0:off, 1:unknow, 2:C to G, 3 G toC
  startxy    = 0x05, // ��Ļ���Ͻǵ��ڴ�����
  setwidth   = 0x06,
  setheight  = 0x07,
  setmode    = 0x08,
  info       = 0x10,
};


enum class ShapeDataStage {
  read_command,
  read_data,
};


class IDrawShape {
public:
  virtual ~IDrawShape() {}
  // д����������(������һ�ε���������), �������״�Ѿ���ȡȫ�������򷵻� false
  virtual bool write(const u32 c) = 0;
  // һ������ȫ����ȡ, �򹹽� opengl ���������ڻ���
  virtual void build(GPU&) = 0;
  // ����ͼ��
  virtual void draw(GPU&) = 0;
};


// ���������ö���, ��������������ʹ�� opengl ����.
class OpenGLScope {
private:
  OpenGLScope(OpenGLScope&);
  OpenGLScope& operator=(OpenGLScope&);
  
public:
  OpenGLScope();
  ~OpenGLScope();
};


class OpenGLShader {
private:
  GLHANDLE program;
  GLHANDLE createShader(ShaderSrc src, u32 shader_flag);
  int getUniform(const char* name);

public:
  OpenGLShader(ShaderSrc vertex, ShaderSrc fragment);
  virtual ~OpenGLShader();

  void use();
  void setUint(const char* name, u32 v);
  void setFloat(const char* name, float v);
};


class GPU : public DMADev {
private:
  class GP0 : public DeviceIO {
    GPU &p;
    ShapeDataStage stage;
    IDrawShape *shape;
  public:
    GP0(GPU &_p) : p(_p), stage(ShapeDataStage::read_command), shape(0) {}
    bool parseCommand(const GpuCommand c);
    void write(u32 value);
    u32 read();
  };


  class GP1 : public DeviceIO {
    GPU &p;
  public:
    GP1(GPU &_p) : p(_p) {}
    void write(u32 value);
    u32 read();
  };

private:
  GP0 gp0;
  GP1 gp1;
  u32 cmd_respons;
  GpuStatus status;
  GLFWwindow* glwindow;
  std::thread* work;
  GpuDataRange screen;
  GpuDataRange ps;
  std::list<IDrawShape*> build;
  std::list<IDrawShape*> shapes;

  // ����gpu�̺߳���, ��Ҫ����
  void gpu_thread();
  void initOpenGL();

public:
  GPU(Bus& bus);
  ~GPU();

  // ���Ϳɻ���ͼ��
  void send(IDrawShape* s) {
    build.push_back(s);
  }

  virtual DmaDeviceNum number() {
    return DmaDeviceNum::gpu;
  }

  virtual bool support(dma_chcr_dir dir) {
    //TODO: ����gpu�Ĵ���
    return false;
  }

  // �����Ѿ��������ɫ������
  template<class Shader> OpenGLShader* getProgram() {
    static Shader instance;
    return &instance;
  }

  GpuDataRange* screen_range() {
    return &screen;
  }
};

}