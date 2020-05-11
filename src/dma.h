#pragma once 

#include "util.h"

namespace ps1e {

// dma0 : MDEC in
// dma1 : MDEC out
// dma2 : GPU
// dma3 : CD-ROM
// dma4 : SPU
// dma5 : PIO
// dma6 : GPU OTC

class MMU;
class Bus;


enum class DmaDeviceNum : u32 {
  MDECin  = 0,
  MDECout = 1,
  gpu     = 2,
  cdrom   = 3,
  spu     = 4,
  pio     = 5,
  otc     = 6,
};


enum class ChcrMode : u32 {
  Manual     = 0, // 单块传输
  Stream     = 1, // 传输连续的数据流
  LinkedList = 2, // 传输链表, GPU only
  Reserved   = 3,
};


//  0 1 1 1 0 0 0  1 0 1 1 1 0 1 1  1 0 0 0 0 0 1 1  1 0 0 0 0 0 0 1 1 : Can Write
// 31 - - - - - - 24 - - - - - - - 16 - - - - - - -  8 - - - 4 - - - 0
//        T       B    <cw >   < dw >           <m>  C             S Dir
//        rigger  usy                            ode hoppin g      Tep
//        28{1    24{1 22{3    18{3             10{2 5{1           1{1
union DMAChcr {
  u32 v;
  struct {
    u32 dir       : 1; // 1:从内存到设备
    u32 step      : 1; // 1:每次地址-4, 0:每次地址+4
    u32        _3 : 6; // 
    u32 chopping  : 1; // 
    ChcrMode mode : 2; //
    u32        _4 : 5; // 
    u32 dma_wsize : 3; // 
    u32        _7 : 1; // 
    u32 cpu_wsize : 3; // 
    u32        _8 : 1; // 
    u32 busy_enb  : 1; // 1: 忙碌, DMA 传输数据开始后置1
    u32        _6 : 3; // 
    u32 trigger   : 1; // 1: 触发传输, 传输开始后置0
    u32        _5 : 3; // 
  };
};


union DMAIrq {
  u32 v;
  struct {
    u32 _0 : 15;
    u32 force : 1; // 16 bit

    u32 d0_enable : 1;
    u32 d1_enable : 1;
    u32 d2_enable : 1;
    u32 d3_enable : 1;
    u32 d4_enable : 1;
    u32 d5_enable : 1;
    u32 d6_enable : 1;
    u32 master_enable : 1; // 24bit

    u32 d0_flag : 1;
    u32 d1_flag : 1;
    u32 d2_flag : 1;
    u32 d3_flag : 1;
    u32 d4_flag : 1;
    u32 d5_flag : 1;
    u32 d6_flag : 1;
    u32 master_flag : 1;
  };

  struct{
    u32 _0        : 15;
    u32 _force    : 1;
    u32 dd_enable : 7;
    u32 _mst_enb  : 1;
    u32 dd_flag   : 7;
    u32 _mst_flag : 1;
  };
};


union DMADpcr {
  u32 v;
  struct {
    u32 d0_pri : 3;
    u32 d0_enb : 1;
    u32 d1_pri : 3;
    u32 d1_enb : 1;
    u32 d2_pri : 3;
    u32 d2_enb : 1;
    u32 d3_pri : 3;
    u32 d3_enb : 1;
    u32 d4_pri : 3;
    u32 d4_enb : 1;
    u32 d5_pri : 3;
    u32 d5_enb : 1;
    u32 d6_pri : 3;
    u32 d6_enb : 1;
    u32 offset : 3;
    u32 _0     : 1;
  };
};


enum class dma_chcr_dir {
  DEV_TO_RAM   = 0,
  RAM_FROM_DEV = 0,
  DEV_FROM_RAM = 1,
  RAM_TO_DEV   = 1,
};


class DMADev {
private:
  const DmaDeviceNum devnum;
  u32 _mask;
  u32 priority;
  bool running;

protected:
  u32 base;      // 目标内存基址
  u32 blocks;    // stream 模式数据包数量
  u32 blocksize; // 一个数据包 u32 的数量
  DMAChcr chcr;
  Bus& bus;

  // DMA 传输过程, 由设备调用该方法开始 DMA 传输,
  // 应该在单独的线程中执行
  void transport();

  // 子类实现内存到设备传输
  void dma_ram2dev_block(psmem addr, u32 bytesize, u32 inc);
  // 子类实现设备到内存传输
  void dma_dev2ram_block(psmem addr, u32 bytesize, u32 inc);

public:
  DMADev(Bus& _bus, DmaDeviceNum _devnum) 
  : devnum(_devnum), priority(0), running(false), bus(_bus) {
    _mask = 1 << (static_cast<u32>(number()) * 4);
  }

  virtual ~DMADev() {};

  // 子类重写, 对传输方向支持返回 true
  virtual bool support(dma_chcr_dir dir) = 0;

  // 停止 DMA 传输
  void stop();
  // 使 DMA 启动传输, 因为传输线程独立, 并不保证立即运行
  void start();

  void set_priority(u32 p) {
    priority = p;
  }

  inline u32 mask() {
    return _mask;
  }

  // 返回设备号
  inline DmaDeviceNum number() {
    return devnum;
  }

  void send_base(u32 b) {
    base = b & 0x00FF'FFFF;
  }

  void send_block(u32 b) {
    blocks = (b & 0xffff'0000) >> 16;
    blocksize = b & 0x0'FFFF;
  }

  u32 send_ctrl(u32 b) {
    u32 old = chcr.v;
    chcr.v = b;
    return old;
  }

  u32 read_base() {
    return base;
  }

  u32 read_block() {
    return blocksize | (blocks << 16);
  }

  u32 read_status() {
    return chcr.v;
  }

private:
};

}