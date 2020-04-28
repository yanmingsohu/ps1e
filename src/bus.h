#pragma once

#include "util.h"
#include "dma.h"
#include "mem.h"
#include "cpu.h"

namespace ps1e {


enum class IrqDevMask : u32 {
  vblank  = 1,
  gpu     = 1 << 1,
  cdrom   = 1 << 2,
  dma     = 1 << 3,
  dotclk  = 1 << 4,
  hblank  = 1 << 5,
  sysclk  = 1 << 6,
  pad_mm  = 1 << 7,
  sio     = 1 << 8,
  spu     = 1 << 9,
  pio     = 1 << 10,
};


class IrqReceiver {
private:
  u32 mask;

protected:
  IrqReceiver() : mask(0) {}

  // ���շ�ʵ�ַ���, ���� cpu �ⲿ�ж�
  virtual void set_ext_int(CpuCauseInt i) = 0;
  // ���շ�ʵ�ַ���, ��� cpu �ⲿ�ж�
  virtual void clr_ext_int(CpuCauseInt i) = 0;

  // ׼���ý��� irq �źź�, �ɽ��շ�����
  void ready_recv_irq() {
    if (mask) {
      set_ext_int(CpuCauseInt::hardware);
    } else {
      clr_ext_int(CpuCauseInt::hardware);
    }
  }

public:
  virtual ~IrqReceiver() {}

  // ���ͷ�����
  void send_irq(u32 m) {
    mask = m;
  }
};


class Bus {
public:
  static const u32 DMA_CTRL_ADDR      = 0x1F80'10F0;
  static const u32 DMA_IRQ_ADDR       = 0x1F80'10F4;
  static const u32 IRQ_STATUS_ADDR    = 0x1F80'1070;
  static const u32 IRQ_MASK_ADDR      = 0x1F80'1074;
  static const u32 DMA_LEN            = 8;
  static const u32 DMA_MASK           = 0x1F80'108F;
  static const u32 DMA_IRQ_WRITE_MASK = (1 << 24) - 1; // 24
  static const u32 IRQ_ST_WR_MASK     = (1 << 11) - 1; // 11:IrqDevMask count

private:
  MMU& mmu;
  IrqReceiver* ir;

  DMADev* dmadev[DMA_LEN];
  DMAIrq  dma_irq;        // DMA_IRQ_ADDR
  DMADpcr dma_dpcr;       // DMA_CTRL_ADDR
  u32     irq_status;     // IRQ_STATUS_ADDR
  u32     irq_mask;       // IRQ_MASK_ADDR

  // �����豸�ж�
  void send_irq(IrqDevMask m);

  // irq_status/irq_mask �Ĵ���״̬�ı������÷���, 
  // ģ��Ӳ����������, ����״̬���͸� IrqReceiver.
  void update_irq_to_reciver();

public:
  Bus(MMU& _mmu, IrqReceiver* _ir = 0) 
  : mmu(_mmu), ir(_ir), dmadev{0}, dma_dpcr{0},
    irq_status(0), irq_mask(0) {}
  ~Bus() {}

  // ��װ DMA �豸
  bool set_dma_dev(DMADev* dd);

  // dma ��������󱻵���, �����ж�
  void send_dma_irq(DMADev*);

  // �� IRQ ������, ͨ���� CPU
  void bind_irq_receiver(IrqReceiver* _ir) {
    ir = _ir;
  }

  void write32(psmem addr, u32 val);
  void write16(psmem addr, u16 val);
  void write8(psmem addr, u8 val);
  u32 read32(psmem addr);
  u16 read16(psmem addr);
  u8 read8(psmem addr);


  template<class T> void write(psmem addr, T v) {
    switch (addr) {
      case DMA_CTRL_ADDR:
        dma_dpcr.v = v;
        set_dma_dev_status();
        return;

      case DMA_IRQ_ADDR:
        dma_irq.v = v; // & DMA_IRQ_WRITE_MASK; ��Ҫ��� flag
        if (v & (1 << 31)) {
          dma_irq.dd_flag = 0;
        }
        return;

      case IRQ_STATUS_ADDR:
        irq_status = v & IRQ_ST_WR_MASK;
        update_irq_to_reciver();
        return;

      case IRQ_MASK_ADDR:
        irq_mask = v;
        update_irq_to_reciver();
        return;
    }

    if (isDMA(addr)) {
      u32 devnum = (addr & 0x70) >> 4;
      DMADev* dd = dmadev[devnum];
      if (!dd) {
        warn("DMA Device Not exist %x: %x", devnum, v);
        return;
      }
      switch (addr & 0xF) {
        case 0x0:
          dd->send_base(v);
          break;
        case 0x4:
          dd->send_block(v);
          break;
        case 0x8:
          if (dd->send_ctrl(v) != v) {
            change_running_state(dd);
          }
          break;
        default:
          warn("Unknow DMA address %x: %x", devnum, v);
      }
      return;
    }

    T* tp = (T*)mmu.memPoint(addr);
    if (tp) {
      *tp = v;
      return;
    }
    warn("WRIT BUS %x %x\n", addr, v);
  }


  template<class T> T read(psmem addr) {
    switch (addr) {
      case DMA_CTRL_ADDR:   return dma_dpcr.v;
      case DMA_IRQ_ADDR:    return dma_irq.v;
      case IRQ_STATUS_ADDR: return irq_status;
      case IRQ_MASK_ADDR:   return irq_mask;
    }

    if (isDMA(addr)) {
      DMADev* dd = dmadev[(addr & 0x70) >> 4];
      if (!dd) {
        warn("DMA Device Not exist %x", addr);
        return 0;
      }
      switch (addr & 0xF) {
        case 0x0:
          return (T)dd->read_base();
        case 0x4:
          return (T)dd->read_block();
        case 0x8:
          return (T)dd->read_status();
        default:
          warn("Unknow DMA address %x", addr);
      }
      return 0;
    }

    T* tp = (T*)mmu.memPoint(addr);
    if (tp) {
      return *tp;
    }
    warn("READ BUS %x\n", addr);
    return 0;
  }

private:
  // dma_dpcr ͬ���� dma �豸��
  void set_dma_dev_status();
  // ���״̬, ��������������/ֹͣDMA����
  void change_running_state(DMADev* dd);

  u32 has_dma_irq() {
    return dma_irq.master_flag = dma_irq.force || (dma_irq.master_enable 
                              && (dma_irq.dd_enable & dma_irq.dd_flag));
  }

  inline bool isDMA(psmem addr) {
    return ((addr & 0xFFFF'FF80) | DMA_MASK) == 0;
  }
};

}