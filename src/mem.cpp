﻿#include "mem.h" 
#include <stdio.h>

namespace ps1e {

MMU::MMU(MemJit& memjit) : ram(memjit), bios(memjit), scratchpad(memjit), cc{0} {
}


u8* MMU::memPoint(psmem addr, bool read) {
  switch (addr & 0xff00'0000) {
    case 0x0000'0000:
    case 0x8000'0000:
    case 0xA000'0000:
      /*if (addr & 0x0FE0'0000) {
        warn("MMU: mem out of bounds %x fix: %x\n", addr, addr & (RAM_SIZE-1));
        return 0;
      }*/
      return ram.point(addr & (RAM_SIZE-1));
  }

  if (addr == 0xFFFE'0130) {
    return (u8*)&cc.v;
  }

  switch (addr) {
    CASE_MEM_MIRROR(0x1F80'1000):
      return (u8*)&expansion1_base;

    CASE_MEM_MIRROR(0x1F80'1004):
      return (u8*)&expansion2_base;

    CASE_MEM_MIRROR(0x1F80'1008):
      return (u8*)&expansion1_delay;

    CASE_MEM_MIRROR(0x1F80'100C):
      return (u8*)&expansion3_delay;

    CASE_MEM_MIRROR(0x1F80'1010):
      return (u8*)&bios_size;

    CASE_MEM_MIRROR(0x1F80'1014):
      return (u8*)&spu_delay;

    CASE_MEM_MIRROR(0x1F80'1018):
      return (u8*)&cdrom_delay;

    CASE_MEM_MIRROR(0x1F80'101C):
      return (u8*)&expansion2_delay;

    CASE_MEM_MIRROR(0x1F80'1020):
      return (u8*)&common_delay;

    CASE_MEM_MIRROR(0x1F80'1060):
      return (u8*)&ram_size;
  }

  switch (addr & 0xFFF0'0000) {
    CASE_MEM_MIRROR(0xBFC0'0000):
      if (addr & 0x0038'0000) { 
        warn("MMU: bios out of bounds %x\n", addr);
        return 0;
      }
      if (read) {
        return bios.point(addr & (BIOS_SIZE-1));
      }
      return (u8*)&garbage;

    case 0x1F80'0000:
    case 0x9F80'0000:
      /*if (addr & 0x000F'F000) {
        warn("MMU: scratchpad out of bounds %x\n", addr);
        return 0;
      }*/
      return scratchpad.point(addr & (DCACHE_SZ-1));

    CASE_MEM_MIRROR(0x1FA0'0000):
      warn("Expansion MEM 3 not implements");
      return (u8*)&garbage;
  }

  #ifdef SAFE_MEM
  const u32 e1 = (expansion1_base & 0x1F00'0000);
  const u32 e2 = (expansion2_base & 0x1F00'0000);

  // bios引导时会尝试读写该地址
  if (addr >=e1 && addr < e1+0x7'ffff) {
    warn("Expansion MEM 1 not implements\n");
    return (u8*)&garbage;
  }
  else if (addr >= e2 && addr < e2+128) {
    warn("Expansion MEM 2 not implements\n");
    return (u8*)&garbage;
  }
  #endif

  return 0;
}


u8* MMU::d_cache(psmem addr) {
  return scratchpad.point(addr & (DCACHE_SZ-1));
}


bool MMU::loadBios(char const* filename) {
  u8* buf = bios.point(0);
  size_t rz = readFile(buf, bios.size(), filename);
  if (!rz) {
    printf(RED("cannot load bios %s\n"), filename);
    return false;
  }
  return true;
}


}