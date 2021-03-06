﻿#include "../gpu.h"
#include "test.h"
#include <thread>
#include <chrono>

namespace ps1e_t {
using namespace ps1e;
static const u32 gp0 = 0x1F80'1810;
static const u32 gp1 = 0x1F80'1814;


void gpu_basic() {
  tsize(sizeof(GpuStatus), 4, "gpu ctrl reg");
  tsize(sizeof(GpuCommand), 4, "gpu command");
}


union Clut {
  u16 v;
  struct {
    u16 cx:6;
    u16 cy:9;
    u16 _:1;
  };

  Clut(u16 _cx, u16 _cy)
  : cx(_cx), cy(_cy) {}
};


union pos {
  u32 v;
  struct {
    u32 x:16; 
    u32 y:16; 
  };
  struct {
    u32 w:16;
    u32 h:16;
  };

  pos(u16 _x, u16 _y) : x(_x), y(_y) {}
};

union Color {
  u32 v;
  struct {
    u32 r:8; 
    u32 g:8; 
    u32 b:8; 
    u32 _:8;
  };

  Color(u8 cmd, u8 _r, u8 _g, u8 _b) : r(_r), g(_g), b(_b), _(cmd) {}
};

union Color2 {
  u16 v;
  struct {
    u16 r:5;
    u16 g:5;
    u16 b:5;
    u16 a:1;
  };

  Color2(u8 _r, u8 _g, u8 _b) : r(_r), g(_g), b(_b) {}
};

union TexpageAttr {
  u32 v;
  struct {
    u32 px          : 4; //0-3
    u32 py          : 1; //4
    u32 semi        : 2; //5-6
    u32 color_mode  : 2; //7-8
    u32 c24         : 1; //9
    u32 draw        : 1; //10
    u32 text_off    : 1; //11
    u32 x_flip      : 1; //12
    u32 y_flip      : 1; //13
    u32 _           :10; //14-23
    u32 cmd         : 8; //24-31
  };
};

union TCoord {
  u32 v;
  struct {
    u32 x : 8;
    u32 y : 8;
    u32 other : 16;
  };
  TCoord(u8 xx, u8 yy, u16 o=0) : x(xx), y(yy), other(o) {}
};


static int random(int max, int min = 0) {
  return rand() % (max - min) + min;
}


static void draw_p3_1(Bus& bus, int x, int y, int count, int cmd) {
  for (int i=0; i<count; ++i) {
    bus.write32(gp0, Color(cmd, random(0xff, 0x10), random(0xff, 0x10), 0x8*i).v);
    bus.write32(gp0, pos(100+x, 10+y).v);
    bus.write32(gp0, pos(200+x, (30+y)+i*30).v);
    bus.write32(gp0, pos(11+x, 100+y).v);
    sleep(50);
  }
}


static void draw_p4_1(Bus& bus, int cmd = 0x28) {
  bus.write32(gp0, Color(cmd, 0,0xa0,0).v);
  bus.write32(gp0, pos(30, 90).v);
  bus.write32(gp0, pos(30, 60).v);
  bus.write32(gp0, pos(60, 60).v);
  bus.write32(gp0, pos(60, 90).v);
}


static void draw_p3_t1(Bus& bus, int cmd = 0x24) {
  TexpageAttr ta{0};
  ta.cmd = 0xE1;
  ta.px = 0;
  ta.py = 0;
  ta.draw = 1;
  bus.write32(gp0, ta.v);

  bus.write32(gp0, Color(cmd, 0,0,0xc0).v);
  bus.write32(gp0, pos(30, 120).v);
  bus.write32(gp0, TCoord(30, 120).v);
  
  bus.write32(gp0, pos(30, 30).v);
  bus.write32(gp0, TCoord(30, 39).v);

  bus.write32(gp0, pos(120, 120).v);
  bus.write32(gp0, TCoord(120, 120).v);
}


static void draw_offset(Bus& bus, int x, int y) {
  // 变更之前必须等待 gpu 绘制完成
  sleep(300);
  bus.write32(gp0, 0xE500'0000 | (x & 0xfff) | ((y & 0xFFF) << 11));
}


static void draw_p3_s1(Bus& bus, int count = 3, int cmd = 0x30) {
  bus.write32(gp0, Color(cmd, 0xff, 0, 0).v);
  bus.write32(gp0, pos(10, 10).v);
  bus.write32(gp0, Color(0, 0, 0xff, 0).v);
  bus.write32(gp0, pos(100, 50).v);
  bus.write32(gp0, Color(0, 0, 0, 0xff).v);
  bus.write32(gp0, pos(50, 100).v);
  if (count == 4) {
    bus.write32(gp0, Color(0, 99, 99, 99).v);
    bus.write32(gp0, pos(150, 100).v);
  }
}


static void draw_p3_s2(Bus& bus, int count = 3, int cmd = 0x34) {
  bus.write32(gp0, Color(cmd, 0xff, 0, 0).v);
  bus.write32(gp0, pos(10, 10).v);
  bus.write32(gp0, TCoord(30, 120).v);

  bus.write32(gp0, Color(0, 0, 0xff, 0).v);
  bus.write32(gp0, pos(100, 50).v);
  bus.write32(gp0, TCoord(30, 39).v);

  bus.write32(gp0, Color(0, 0, 0, 0xff).v);
  bus.write32(gp0, pos(50, 100).v);
  bus.write32(gp0, TCoord(120, 120).v);

  if (count == 4) {
    bus.write32(gp0, Color(0, 99, 99, 99).v);
    bus.write32(gp0, pos(150, 100).v);
    bus.write32(gp0, TCoord(120, 200).v);
  }
}


static void draw_l1(Bus& bus, int cmd = 0x40, int offy = 0) {
  bus.write32(gp0, Color(cmd, 0xff, 0, 0).v);
  bus.write32(gp0, pos(20, 0 + offy).v);
  bus.write32(gp0, pos(300, 0 + offy).v);
}


static void draw_l2(Bus& bus, int cmd = 0x50, int offy = 0) {
  bus.write32(gp0, Color(cmd, 0xff, 0, 0).v);
  bus.write32(gp0, pos(20, 30 + offy).v);
  bus.write32(gp0, Color(0, 0, 0xff, 0).v);
  bus.write32(gp0, pos(300, 30 + offy).v);
}


static void draw_lm1(Bus& bus) {
  bus.write32(gp0, Color(0x48, 0, 0, 0xff).v);
  int x = 20, y = 10, n = 10;
  for (int i=0; i<100; ++i) {
    if (i & 0x1) {
      x += 10;
    } else {
      y += n;
      n = -n;
    }
    bus.write32(gp0, pos(x, y).v);
  }
  bus.write32(gp0, 0x50005000);
}


static void draw_lm2(Bus& bus, int cmd = 0x58) {
  int x = 20, y = 50, n = 10;
  //bus.write32(gp0, Color(cmd, 0, x, 0xff).v);
  //bus.write32(gp0, pos(x, y).v);

  for (int i=0; i<100; ++i) {
    if (i & 0x1) {
      x += 10;
    } else {
      y += n;
      n = -n;
    }
    bus.write32(gp0, Color(cmd, 0, x, 0xff).v);
    bus.write32(gp0, pos(x, y).v);
    cmd = 0;
  }
  bus.write32(gp0, 0x50005000);
}


static void draw_box1(Bus& bus, int x, int y, int cmd=0x60, int w=100, int h=100) {
  bus.write32(gp0, Color(cmd, 0x93, 0x93, x % 255).v);
  bus.write32(gp0, pos(x, y).v);
  if (cmd == 0x60 || cmd == 0x62) {
    bus.write32(gp0, pos(w, h).v);
  }
}


static void draw_box2(Bus& bus, int x, int y, int cmd = 0x64) {
  bus.write32(gp0, Color(cmd, 0x43, 0x93, x % 255).v);
  bus.write32(gp0, pos(x, y).v);
  u16 clut = 0;
  bus.write32(gp0, TCoord(0, 0, clut).v);
  if (cmd >= 0x64 && cmd <= 0x67) {
    bus.write32(gp0, pos(50, 50).v);
  }
}


static void draw_pt(Bus& bus, int count = 100, int cmd = 0x68) {
  for (int i=0; i<count; ++i) {
    draw_box1(bus, random(100, 150), random(100, 150), cmd);
  }
}


static void draw_pt2(Bus& bus, int x, int y, int count = 100, int cmd = 0x6C) {
  for (int i=0; i<count; ++i) {
    bus.write32(gp0, Color(cmd, 0, 0, 200).v);
    bus.write32(gp0, pos(x+i, y).v);
    u16 clut = 0;
    bus.write32(gp0, TCoord(i, i, clut).v);
  }
}


static void set_text_page(Bus& bus, int x, int y) {
  sleep(100);
  TexpageAttr ta{0};
  ta.cmd = 0xE1;
  ta.px = x / 64;
  ta.py = y / 256;
  ta.draw = 1;
  bus.write32(gp0, ta.v);
}


static void gclear(Bus& bus, int x, int y, int w, int h) {
  bus.write32(gp0, Color(0x02, 11, 11, 11).v);
  bus.write32(gp0, pos(x, y).v);
  bus.write32(gp0, pos(w, h).v);
}


static void bios_code(GPU& gpu, Bus& bus) {
  // 这是 bios 绘制 logo 一帧的代码
  u32 list[] = {
    // 白色背景
    0x02b4b4b4, 0x00000000, 0x01e00280, 
    // 阴影四点多边形, 8word
    0x380000b2, 0x00f000c0, 0x00008cb2, 0x00700140, 
    0x00008cb2, 0x01700140, 0x000000b2, 0x00f001c0, 
    // 阴影三点多边形
    0x300000b2, 0x01080198, 0x00008cb2, 0xba1bbaab, 0x00008cb2, 0x0164baab,
    // 阴影三点多边形
    0x300000b2, 0x00d80377, 0x00008cb2, 0xb9eb03d3, 0x00008cb2, 0x013403d3,
  };
  const int len = sizeof(list) / sizeof(u32);
  for (int i=0; i<len; ++i) {
    bus.write32(gp0, list[i]);
    printf("  %x\n", i);
  }
}


static void test_clut(GPU& gpu, Bus& bus) {
  draw_offset(bus, 0, 0);

  // 写调色板
  u16 clutx = 16; // 16的倍数
  u16 cluty = 410;
  bus.write32(gp0, 0xA0000000);
  bus.write32(gp0, pos(clutx, cluty).v);
  bus.write32(gp0, pos(16, 1).v);
  // 写颜色
  for (int i=0; i<2; ++i) {
    //TODO: 需要确认字节顺序: 高低16位是否需要互换
    // 0红 1绿 2蓝 3黄 4青 5白 6灰 7紫
    bus.write32(gp0, (Color2(0x1f, 0, 0).v)     | (Color2(0, 0x1f, 0).v << 16));
    bus.write32(gp0, (Color2(0, 0, 0x1f).v)     | (Color2(0x1f, 0x1f, 0).v << 16));
    bus.write32(gp0, (Color2(0, 0x1f, 0x1f).v)  | (Color2(0x1f, 0x1f, 0x1f).v << 16));
    bus.write32(gp0, (Color2(10, 10, 10).v)     | (Color2(0x1f, 0, 0x1f).v << 16));
  }

  // 写纹理
  u16 text_x = 512; // 64 的倍数
  u16 text_y = 0; // 0 | 256
  bus.write32(gp0, 0xA0000000);
  bus.write32(gp0, pos(text_x, text_y).v);
  bus.write32(gp0, pos(32, 32).v);

  for (int j=0; j<16; ++j) {
    for (int i=0; i<4; ++i) {
      bus.write32(gp0, 0x11112222);
      bus.write32(gp0, 0x33443344);
      bus.write32(gp0, 0x55557777);
      bus.write32(gp0, 0x88889999);
    }

    for (int i=0; i<16; ++i) {
      bus.write32(gp0, 0x00000000);
    }
  }

  TexpageAttr t = {0};
  t.px = text_x / 64;
  t.py = text_y;
  //0=4bit, 1=8bit, 2=15bit,
  t.color_mode = 0;

  int box = 400;
  bus.write32(gp0, Color(0x2d, 0,0,0).v);
  bus.write32(gp0, pos(10, 10).v); // Vertex1
  bus.write32(gp0, TCoord(0, 0, Clut(clutx/16, cluty).v).v); //Texcoord1+Palette (ClutYyXxh)
  bus.write32(gp0, pos(10, box).v); // Vertex2
  bus.write32(gp0, TCoord(0, 32, t.v).v); //Texcoord2+Texpage (PageYyXxh)x
  bus.write32(gp0, pos(box, 10).v); // Vertex3
  bus.write32(gp0, TCoord(32, 0).v);
  bus.write32(gp0, pos(box, box).v); // Vertex4
  bus.write32(gp0, TCoord(32, 32).v);

  for (;;) sleep(1000);//!!!!!!
}


static void test_shape(GPU& gpu, Bus& bus) {
  bus.write32(gp0, Color(0x20, 0xff,0,0).v);
  bus.write32(gp0, pos(80, 10).v);
  bus.write32(gp0, pos(100, 100).v);
  bus.write32(gp0, pos(120, 80).v);
  
  draw_offset(bus, 0, 0);

  draw_l1(bus);
  draw_l1(bus, 0x42, 5);
  draw_lm1(bus);
  draw_l2(bus);
  draw_l2(bus, 0x52, 5);
  draw_lm2(bus);

  draw_box1(bus, 10, 100, 0x62);
  draw_box1(bus, 300, 100);

  draw_p3_1(bus, 100, 100, 5, 0x20);
  draw_p3_1(bus, 10, 10, 5, 0x22);
  draw_p4_1(bus, 0x28);

  draw_offset(bus, 60, 30);
  draw_p4_1(bus, 0x2A);

  draw_offset(bus, 200, 10);
  draw_p3_t1(bus);

  draw_offset(bus, 300, 40);
  draw_p3_t1(bus, 0x25);

  draw_p3_s1(bus);

  draw_offset(bus, 30, 40);
  draw_p3_s1(bus, 3, 0x32);

  draw_offset(bus, 30, 120);
  draw_p3_s1(bus, 4, 0x38);

  draw_offset(bus, 130, 120);
  draw_p3_s1(bus, 4, 0x3A);

  draw_offset(bus, 240, 120);
  draw_p3_s2(bus, 4, 0x3c);

  draw_offset(bus, 40, 200);
  draw_p3_s2(bus, 3, 0x36);

  draw_offset(bus, 0, 0);
  draw_pt(bus, 100);
  draw_offset(bus, 100, 0);
  draw_pt(bus, 100, 0x6A);

  draw_box1(bus, 320, 130, 0x60, 100, 100);

  draw_box1(bus, 400, 100, 0x70);
  draw_box1(bus, 380, 100, 0x72);
  draw_box1(bus, 360, 100, 0x78);
  draw_box1(bus, 340, 100, 0x7A);

  set_text_page(bus, 100, 100);
  draw_box2(bus, 340, 150, 0x64);
  draw_box2(bus, 340, 200, 0x65);

  draw_box2(bus, 390, 150, 0x66);
  draw_box2(bus, 390, 200, 0x67);

  draw_pt2(bus, 340, 260, 100, 0x6C);
  draw_pt2(bus, 340, 262, 100, 0x6D);
  draw_pt2(bus, 340, 264, 100, 0x6E);
  draw_pt2(bus, 340, 266, 100, 0x6F);

  draw_box2(bus, 340, 270, 0x74);
  draw_box2(bus, 350, 270, 0x75);
  draw_box2(bus, 360, 270, 0x76);
  draw_box2(bus, 370, 270, 0x77);

  draw_box2(bus, 340, 280, 0x7C);
  draw_box2(bus, 360, 280, 0x7D);
  draw_box2(bus, 380, 280, 0x7E);
  draw_box2(bus, 400, 280, 0x7F);
}


void test_gpu(GPU& gpu, Bus& bus) {
  gpu_basic();
  bus.write32(gp1, 0x0200'0001); // open display
  
  //bios_code(gpu, bus);
  //test_shape(gpu, bus);
  //test_clut(gpu, bus);

  // end
  draw_offset(bus, 0, 0);
  gclear(bus, 100, 100, 50, 50);
  //sleep(10000);
}

}