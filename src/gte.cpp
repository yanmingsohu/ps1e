﻿#include "gte.h"

namespace ps1e {


GTE::GTE() : 
  ir1(*this,  0, GteReg63Error::Ir1), ir2(*this, 5, GteReg63Error::Ir2), 
  ir3(*this, 15, GteReg63Error::Ir3), ir0(*this, 0, GteReg63Error::Ir0), 
  irgb(*this), orgb(*this),
  sxyp(*this), sz3(*this), rgb2(*this), lzcs(*this),
  mac0(*this, GteReg63Error::Mac0p), mac1(*this, GteReg63Error::Mac1p),
  mac2(*this, GteReg63Error::Mac2p), mac3(*this, GteReg63Error::Mac3p),

  r32(rt.r11, rt.r12), r33(rt.r13, rt.r21), r34(rt.r22, rt.r23),
  r35(rt.r31, rt.r32), r36(rt.r33),

  r40(llm.r11, llm.r12), r41(llm.r13, llm.r21), r42(llm.r22, llm.r23),
  r43(llm.r31, llm.r32), r44(llm.r33),

  r48(lcm.lr1, lcm.lr2), r49(lcm.lg3, lcm.lg1), r50(lcm.lg2, lcm.lg3),
  r51(lcm.lb1, lcm.lb2), r52(lcm.lb3)
{}


GteMac::GteMac(GTE& g, GteReg63Error p) : r(g), positive(p) {
}


GteIR::GteIR(GTE& _r, int bit, GteReg63Error e) : r(_r), Lbit(bit), IRE(e) {
}


u32 GteIR::read() {
  return v;
}


void GteIR::write(u32 _v) {
  v = _v;
  u32 s = v >> 3;
  u32 mask = 0x1f << Lbit;
  r.irgb.v = (r.irgb.v & mask) | (s << Lbit);
}


void GteIR0::write(u32 d) {
  v = d;
}


void GteIrgb::write(u32 d) {
  v = d;
  r.ir1.v = (0x1f & v) << 3;
  r.ir2.v = (0x1f & (v >> 5)) << 3;
  r.ir3.v = (0x1f & (v >> 10)) << 3;
}


u32 GteOrgb::read() {
  return r.irgb.v;
}


void GteFlag::write(u32 d) {
  v = GteReg63WriteMask & d;
}


void GteFlag::set(GteReg63Error f) {
  v |= static_cast<u32>(f);
}


void GteFlag::update() {
  if (v & GteFlagLogSum) {
    v |= (1u << 31);
  } else {
    v &= (1u << 31)-1;
  }
}


void GteFlag::clear() {
  v = 0;
}


void GteVectorXY::write(u32 d) {
  fx = (s32) ((s16)(d & 0xffff));
  fy = (s32) ((s16)((d>>16) & 0xffff));
}


u32 GteVectorXY::read() {
  s16 x = fx;
  s16 y = fy;
  return x & (((s32) y) << 16);
}


GteSxyFifo::GteSxyFifo(GTE& _r) : r(_r) {
}


u32 GteSxyFifo::read() {
  return r.sxy2.read();
}


void GteSxyFifo::write(u32 d) {
  float x = (s32) ((s16)(d & 0xffff));
  float y = (s32) ((s16)((d>>16) & 0xffff));
  push(x, y);
}


void GteSxyFifo::push(float x, float y) {
  r.sxy0.fx = r.sxy1.fx;
  r.sxy0.fy = r.sxy1.fy;
  r.sxy1.fx = r.sxy2.fx;
  r.sxy1.fy = r.sxy2.fy;
  r.sxy2.fx = x;
  r.sxy2.fy = y;
}


GteZFifo::GteZFifo(GTE& _r) : r(_r) {
}


void GteZFifo::push(float d) {
  r.sz0.v = r.sz1.v;
  r.sz1.v = r.sz2.v;
  r.sz2.v = v;
  v = d;
}


void GteRgb::write(u32 d) {
  v = d;
}


u32 GteRgb::read() {
  return v;
}


u32 GteRgb::r() {
  return v & 0xff;
}


u32 GteRgb::g() {
  return (v >> 8) & 0xff;
}


u32 GteRgb::b() {
  return (v >> 16) & 0xff;
}


u32 GteRgb::code() {
  return (v >> 24) & 0xff;
}


GteRgbFifo::GteRgbFifo(GTE& _r) : r(_r) {
}


void GteRgbFifo::write(u32 d) {
  r.rgb0.v = r.rgb1.v;
  r.rgb1.v = v;
  v = d;
}


GteMatrixReg::GteMatrixReg(float& lsb, float& msb) : m(msb), l(lsb) {
}


u32 GteMatrixReg::read() {
  s16 lsb = l;
  s16 msb = m;
  return u32(u16(msb) << 16) | u16(lsb);
}


void GteMatrixReg::write(u32 d) {
  s16 lsb = (s16) (0xffff & d);
  s16 msb = (s16) (0xffff & (d >> 16));
  l = lsb;
  m = msb;
}


GteMatrixReg1::GteMatrixReg1(float& p) : v(p) {
}


u32 GteMatrixReg1::read() {
  return (s16) v;
}


void GteMatrixReg1::write(u32 d) {
  v = (s16) d;
}


GteLeadingZeroes::GteLeadingZeroes(GTE& _r) : r(_r) {
}


u32 GteLeadingZeroes::read() {
  return v;
}


//TODO: 结果在 1..32 范围内?
void GteLeadingZeroes::write(u32 d) {
  v = d;

  if (d & 0x8000'0000) d = ~d;
  if (d == 0) {
    r.lzcr.v = 32;
    return;
  }
  u32 mask = 0x8000'0000;
  for (u32 i = 0; i < 32; ++i) {
    if (mask & d) {
      r.lzcr.v = i;
      return;
    }
    mask = mask >> 1;
  }
}


// fn(RegNumber, RegName, data)
#define GTE_DATA_REG_LIST(fn, d) \
  fn( 0, vxy0, d) \
  fn( 1, vz0,  d) \
  fn( 2, vxy1, d) \
  fn( 3, vz1,  d) \
  fn( 4, vxy2, d) \
  fn( 5, vz2,  d) \
  fn( 6, rgbc, d) \
  fn( 7, otz,  d) \
  fn( 8, ir0,  d) \
  fn( 9, ir1,  d) \
  fn(10, ir2,  d) \
  fn(11, ir3,  d) \
  fn(12, sxy0, d) \
  fn(13, sxy1, d) \
  fn(14, sxy2, d) \
  fn(15, sxyp, d) \
  fn(16, sz0,  d) \
  fn(17, sz1,  d) \
  fn(18, sz2,  d) \
  fn(19, sxyp, d) \
  fn(20, rgb0, d) \
  fn(21, rgb1, d) \
  fn(22, rgb2, d) \
  fn(23, res1, d) \
  fn(24, mac0, d) \
  fn(25, mac1, d) \
  fn(26, mac2, d) \
  fn(27, mac3, d) \
  fn(28, irgb, d) \
  fn(29, orgb, d) \
  fn(30, lzcs, d) \
  fn(31, lzcr, d)

#define GTE_CTRL_REG_LIST(fn, d) \
  fn( 0, r32,  d) \
  fn( 1, r33,  d) \
  fn( 2, r34,  d) \
  fn( 3, r35,  d) \
  fn( 4, r36,  d) \
  fn( 5, trX,  d) \
  fn( 6, trY,  d) \
  fn( 7, trZ,  d) \
  fn( 8, r40,  d) \
  fn( 9, r41,  d) \
  fn(10, r42,  d) \
  fn(11, r43,  d) \
  fn(12, r44,  d) \
  fn(13, rbk,  d) \
  fn(14, gbk,  d) \
  fn(15, bbk,  d) \
  fn(16, r48,  d) \
  fn(17, r49,  d) \
  fn(18, r50,  d) \
  fn(19, r51,  d) \
  fn(20, r52,  d) \
  fn(21, rfc,  d) \
  fn(22, gfc,  d) \
  fn(23, bfc,  d) \
  fn(24, offx, d) \
  fn(25, offy, d) \
  fn(26, H,    d) \
  fn(27, dqa,  d) \
  fn(28, dqb,  d) \
  fn(29, zsf3, d) \
  fn(30, zsf4, d) \
  fn(31, flag, d)

#define GTE_WRITE_CASE(i, r, d)   case i: r.write(d); break;
#define GTE_READ_CASE(i, r, _)    case i: return r.read();


u32 GTE::read_data(const u8 i) {
  switch (i) {
    GTE_DATA_REG_LIST(GTE_READ_CASE, 0)
    default:
      error("Invaild GTE data reg %d\n", i);
      return 0;
  }
}


void GTE::write_data(const u8 i, const u32 d) {
  switch (i) {
    GTE_DATA_REG_LIST(GTE_WRITE_CASE, d)
    default:
      error("Invaild GTE data reg %d\n", i);
  }
}


u32 GTE::read_ctrl(const u8 i) {
  switch (i) {
    GTE_CTRL_REG_LIST(GTE_READ_CASE, 0)
    default:
      error("Invaild GTE ctrl reg %d\n", i);
      return 0;
  }
}


void GTE::write_ctrl(const u8 i, const u32 d) {
  switch (i) {
    GTE_CTRL_REG_LIST(GTE_WRITE_CASE, d)
    default:
      error("Invaild GTE ctrl reg %d\n", i);
  }
}


// fn(CommandNum, Function, GteCommand)
#define GTE_COMMAND_LIST(fn, c) \
  fn(0x01, RTPS,  c) \
  fn(0x06, NCLIP, c) \
  fn(0x0C, OP,    c) \
  fn(0x10, DPCS,  c) \
  fn(0x11, INTPL, c) \
  fn(0x12, MVMVA, c) \
  fn(0x13, NCDS,  c) \
  fn(0x14, CDP,   c) \
  fn(0x16, NCDT,  c) \
  fn(0x1B, NCCS,  c) \
  fn(0x1C, CC,    c) \
  fn(0x1E, NCS,   c) \
  fn(0x20, NCT,   c) \
  fn(0x28, SQR,   c) \
  fn(0x29, DCPL,  c) \
  fn(0x2A, DPCT,  c) \
  fn(0x2D, AVSZ3, c) \
  fn(0x2E, AVSZ4, c) \
  fn(0x30, RTPT,  c) \
  fn(0x3D, GPF,   c) \
  fn(0x3E, GPL,   c) \
  fn(0x3F, NCCT,  c)

#define GTE_CMD_CASE(n, f, c)   case n: /*printf("GTE CMD %s\n", #f);*/ f(c); break;


bool GTE::execute(const GteCommand c) {
  // 如果执行无效指令, flag 也被清除
  flag.clear();
  switch (c.num) {
    GTE_COMMAND_LIST(GTE_CMD_CASE, c);
    default:
      return false;
  }
  flag.update();
  return true;
}


u32 GTE::read_flag() {
  return flag.v;
}


void GTE::write_ir(GteIR& ir, s32 v, u32 lm) {
  if (lm) {
    if (v < 0) {
      flag.set(ir.IRE);
      v = 0;
    } else if (v > GteOF15) {
      flag.set(ir.IRE);
      v = GteOF15-1;
    }
  } else {
    if (v < -GteOF15) {
      flag.set(ir.IRE);
      v = -GteOF15;
    } else if (v > GteOF15) {
      flag.set(ir.IRE);
      v = GteOF15-1;
    }
  }
  ir.write(v);
}


void GTE::write_ir0(s32 v, u32) {
  if (v < 0) {
    flag.set(ir0.IRE);
    v = 0;
  } else if (v > GteOf12) {
    flag.set(ir0.IRE);
    v = GteOf12-1;
  }
  ir0.write(v);
}


void GTE::write_mac(GteMac& mac, float d) {
  if (d > GteOF43) {
    flag.set(mac.positive);
  }
  else if (d < -GteOF43) {
    flag.set(static_cast<GteReg63Error>(static_cast<u32>(mac.positive) + 3));
  }
  mac.v = d;
}


void GTE::write_mac0(float d) {
  if (d > GteOF31) {
    flag.set(mac0.positive);
  }
  else if (d < -GteOF31) {
    flag.set(static_cast<GteReg63Error>(static_cast<u32>(mac0.positive) + 3));
  }
  mac0.v = d;
}


void GTE::write_color_fifo() {
  u32 r = mac1.read() >> 4;
  u32 g = mac2.read() >> 4;
  u32 b = mac3.read() >> 4;
  u32 code = rgbc.code();

  if (r > 0xff) {
    r = 0xff;
    flag.set(GteReg63Error::R);
  }
  if (g > 0xff) {
    g = 0xff;
    flag.set(GteReg63Error::G);
  }
  if (b > 0xff) {
    b = 0xff;
    flag.set(GteReg63Error::B);
  }
  rgb2.write(r | (g << 8) | (b << 16) | (code << 24));
}


void GTE::write_z_fifo(float d) {
  if (d > 0xFFFF) {
    d = 0xffff;
    flag.set(GteReg63Error::Sz3);
  }
  else if (d < 0) {
    d = 0;
    flag.set(GteReg63Error::Sz3);
  }
  sz3.push(d);
}


void GTE::write_otz(float d) {
  if (d > 0xFFFF) {
    d = 0xffff;
    flag.set(GteReg63Error::Sz3);
  }
  else if (d < 0) {
    d = 0;
    flag.set(GteReg63Error::Sz3);
  }
  otz.v = d;
}


void GTE::write_xy_fifo(float x, float y) {
  if (x > 0x3ff) {
    x = 0x3ff;
    flag.set(GteReg63Error::Sx2);
  } else if (x < -0x400) {
    x = -0x400;
    flag.set(GteReg63Error::Sx2);
  }

  if (y > 0x3ff) {
    y = 0x3ff;
    flag.set(GteReg63Error::Sy2);
  } else if (y > -0x400) {
    y = -0x400;
    flag.set(GteReg63Error::Sy2);
  }

  sxyp.push(x, y);
}


void GTE::write_ir_mac123(GteCommand c, float m1, float m2, float m3) {
  if (c.sf) {
    m1 /= 0x1000;
    m2 /= 0x1000;
    m3 /= 0x1000;
  }
  write_mac(mac1, m1);
  write_mac(mac2, m2);
  write_mac(mac3, m3);
  write_ir(ir1, m1, c.lm);
  write_ir(ir2, m2, c.lm);
  write_ir(ir3, m3, c.lm);
}


void GTE::write_ir_mac123(GteCommand c, GteVec3 v) {
  write_ir_mac123(c, v.x, v.y, v.z);
}


void GTE::init_reserved_mm(GteMatrix& mm) {
  mm = {
     -0x60,   0x60, static_cast<float>(ir0.v),
    rt.r13, rt.r13, rt.r13,
    rt.r12, rt.r12, rt.r12,
  };
}


float GTE::overflow_div1() {
  float r = (( (H.v * 0x20000 / sz3.v) +1) /2);
  if (r > 0x1FFFF) {
    r = 0x1FFFF;
    flag.set(GteReg63Error::Div);
  }
  return r;
}


void GTE::RTPS(GteCommand c) {
  PerspectiveTransform(c, sxy0, sz0);
}


void GTE::RTPT(GteCommand c) {
  PerspectiveTransform(c, sxy0, sz0);
  PerspectiveTransform(c, sxy1, sz1);
  PerspectiveTransform(c, sxy2, sz2);
}


void GTE::PerspectiveTransform(GteCommand c, GteVectorXY& xy, GteVectorZ& z) {
  GteVec3 m = mulMatrix(rt, GteVec3(xy, z));
  m.add((trX.v * 0x1000), (trY.v * 0x1000), (trZ.v * 0x1000));
  write_ir_mac123(c, m);
  write_z_fifo(m.z / 0x1000);

  float h = overflow_div1();
  float x = (h * ir1.v + offx.v);
  float y = (h * ir2.v + offy.v);
  write_xy_fifo(x, y);

  write_mac0(h * dqa.v + dqb.v);
  write_ir0(mac0.v);
}


void GTE::NCLIP(GteCommand c) {
  float m = (sxy0.fx * sxy1.fy)+(sxy1.fx * sxy2.fy)+(sxy2.fx * sxy0.fy)
           -(sxy0.fx * sxy2.fy)-(sxy1.fx * sxy0.fy)-(sxy2.fx * sxy1.fy);
  write_mac0(m);
}


void GTE::OP(GteCommand c) {
  float m1 = ir3.v * rt.r22 - ir2.v * rt.r33;
  float m2 = ir1.v * rt.r33 - ir3.v * rt.r11;
  float m3 = ir2.v * rt.r11 - ir1.v * rt.r22;
  write_ir_mac123(c, m1, m2, m3);
}


void GTE::MVMVA(GteCommand c) {
  float t1, t2, t3;
  GteVec3 v;
  GteMatrix mx;

  switch (static_cast<MVMVA_Trans_Vec>(c.mv_tv)) {
    case MVMVA_Trans_Vec::tr:
      t1 = trX.v;
      t2 = trY.v;
      t3 = trZ.v;
      break;
    case MVMVA_Trans_Vec::bk:
      t1 = rbk.v;
      t2 = gbk.v;
      t3 = bbk.v;
      break;
    case MVMVA_Trans_Vec::fc:
      t1 = rfc.v;
      t2 = gfc.v;
      t3 = bfc.v;
      break;
    default:
      t1 = t2 = t3 = 0;
      break;
  }

  switch (static_cast<MVMVA_Mul_Vec>(c.mv_mv)) {
    case MVMVA_Mul_Vec::v0:
      v = GteVec3(vxy0, vz0);
      break;
    case MVMVA_Mul_Vec::v1:
      v = GteVec3(vxy1, vz1);
      break;
    case MVMVA_Mul_Vec::v2:
      v = GteVec3(vxy2, vz2);
      break;
    case MVMVA_Mul_Vec::ir:
      v = GteVec3(ir1.v, ir2.v, ir3.v);
      break;
  }

  switch (static_cast<MVMVA_Mul_Mx>(c.mv_mm)) {
    case MVMVA_Mul_Mx::color:
      mx = lcm;
      break;
    case MVMVA_Mul_Mx::light:
      mx = llm;
      break;
    case MVMVA_Mul_Mx::rotation:
      mx = rt;
      break;
    case MVMVA_Mul_Mx::reserved:
      init_reserved_mm(mx);
      break;
  }

  write_ir_mac123(c, mulMatrix(mx, v));
}


GteVec3 GTE::mulMatrix(GteMatrix& mm, GteVec3 v) {
  GteVec3 r;
  r.x = (mm.r11 * v.x)+(mm.r12 * v.y)+(mm.r13 * v.z);
  r.y = (mm.r21 * v.x)+(mm.r22 * v.y)+(mm.r23 * v.z);
  r.z = (mm.r31 * v.x)+(mm.r32 * v.y)+(mm.r33 * v.z);
  return r;
}


void GTE::normalColor(GteCommand c, GteVec3 v) {
  GteVec3 m = mulMatrix(llm, v);
  write_ir_mac123(c, m.x, m.y, m.z);
  m = mulMatrix(lcm, GteVec3(ir1.v, ir2.v, ir3.v));
  m.add(rbk.v, gbk.v, bbk.v);
  write_ir_mac123(c, m.x, m.y, m.z);
}


void GTE::NCS(GteCommand c) {
  normalColor(c, GteVec3(vxy0, vz0));
  write_color_fifo();
}


void GTE::NCT(GteCommand c) {
  normalColor(c, GteVec3(vxy0, vz0));
  write_color_fifo();
  normalColor(c, GteVec3(vxy1, vz1));
  write_color_fifo();
  normalColor(c, GteVec3(vxy2, vz2));
  write_color_fifo();
}


void GTE::normalColorDepth(GteCommand c) {
  float m1 = (rgbc.r() * ir1.v)+(ir0.v * (rfc.v - rgbc.r() * ir1.v));
  float m2 = (rgbc.g() * ir2.v)+(ir0.v * (gfc.v - rgbc.g() * ir2.v));
  float m3 = (rgbc.b() * ir3.v)+(ir0.v * (bfc.v - rgbc.b() * ir3.v));
  write_ir_mac123(c, m1, m2, m3);
}


void GTE::NCDS(GteCommand c) {
  normalColor(c, GteVec3(vxy0, vz0));
  normalColorDepth(c);
  write_color_fifo();
}


void GTE::NCDT(GteCommand c) {
  normalColor(c, GteVec3(vxy0, vz0));
  normalColorDepth(c);
  write_color_fifo();

  normalColor(c, GteVec3(vxy1, vz1));
  normalColorDepth(c);
  write_color_fifo();

  normalColor(c, GteVec3(vxy2, vz2));
  normalColorDepth(c);
  write_color_fifo();
}


void GTE::normalColorColor(GteCommand c) {
  float m1 = rgbc.r() * ir1.v;
  float m2 = rgbc.g() * ir2.v;
  float m3 = rgbc.b() * ir3.v;
  write_ir_mac123(c, m1, m2, m3);
}


void GTE::NCCS(GteCommand c) {
  normalColor(c, GteVec3(vxy0, vz0));
  normalColorColor(c);
  write_color_fifo();
}


void GTE::NCCT(GteCommand c) {
  normalColor(c, GteVec3(vxy0, vz0));
  normalColorColor(c);
  write_color_fifo();

  normalColor(c, GteVec3(vxy1, vz1));
  normalColorColor(c);
  write_color_fifo();

  normalColor(c, GteVec3(vxy2, vz2));
  normalColorColor(c);
  write_color_fifo();
}


void GTE::SQR(GteCommand c) {
  float m1 = ir1.v * ir1.v;
  float m2 = ir2.v * ir2.v;
  float m3 = ir3.v * ir3.v;
  write_ir_mac123(c, m1, m2, m3);
}


void GTE::CC(GteCommand c) {
  GteVec3 m = mulMatrix(lcm, GteVec3(ir1.v, ir2.v, ir3.v));
  m.add(rbk.v, gbk.v, bbk.v);
  write_ir_mac123(c, m.x, m.y, m.z);
  normalColorColor(c);
  write_color_fifo();
}


void GTE::CDP(GteCommand c) {
  GteVec3 m = mulMatrix(lcm, GteVec3(ir1.v, ir2.v, ir3.v));
  m.add(rbk.v, gbk.v, bbk.v);
  write_ir_mac123(c, m.x, m.y, m.z);
  normalColorDepth(c);
  write_color_fifo();
}


void GTE::DCPL(GteCommand c) {
  normalColorDepth(c);
  write_color_fifo();
}


void GTE::dptchCueColor(GteCommand cmd, GteRgb& cc) {
  float m1 = cc.r() + ir0.v * (rfc.v - cc.r());
  float m2 = cc.g() + ir0.v * (gfc.v - cc.g());
  float m3 = cc.b() + ir0.v * (bfc.v - cc.b());
  write_ir_mac123(cmd, m1, m2, m3);
}


void GTE::DPCT(GteCommand c) {
  dptchCueColor(c, rgb0);
  write_color_fifo();
  dptchCueColor(c, rgb1);
  write_color_fifo();
  dptchCueColor(c, rgb2);
  write_color_fifo();
}


void GTE::DPCS(GteCommand c) {
  dptchCueColor(c, rgb0);
  write_color_fifo();
}


void GTE::INTPL(GteCommand c) {
  float m1 = ir1.v + ir0.v * (rfc.v - ir1.v);
  float m2 = ir2.v + ir0.v * (gfc.v - ir2.v);
  float m3 = ir3.v + ir0.v * (bfc.v - ir3.v);
  write_color_fifo();
}


void GTE::GPF(GteCommand c) {
  float m1 = ir0.v * ir1.v;
  float m2 = ir0.v * ir2.v;
  float m3 = ir0.v * ir3.v;
  write_ir_mac123(c, m1, m2, m3);
  write_color_fifo();
}


void GTE::GPL(GteCommand c) {
  float m1 = mac1.v + ir0.v * ir1.v;
  float m2 = mac2.v + ir0.v * ir2.v;
  float m3 = mac3.v + ir0.v * ir3.v;
  write_ir_mac123(c, m1, m2, m3);
  write_color_fifo();
}


void GTE::AVSZ3(GteCommand c) {
  float m = zsf3.v * (sz1.v + sz2.v + sz3.v);
  write_mac0(m);
  write_otz(mac0.v);
}


void GTE::AVSZ4(GteCommand c) {
  float m = zsf3.v * (sz0.v + sz1.v + sz2.v + sz3.v);
  write_mac0(m);
  write_otz(mac0.v);
}


}