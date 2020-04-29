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
  static const u32 IRQ_REQUEST_BIT = (1 << 10);
  u32 mask;
  u32 mask_prev;

protected:
  IrqReceiver() : mask(0) {}

  // ���շ�ʵ�ַ���, ���� cpu �ⲿ�ж�
  virtual void set_ext_int(CpuCauseInt i) = 0;
  // ���շ�ʵ�ַ���, ��� cpu �ⲿ�ж�
  virtual void clr_ext_int(CpuCauseInt i) = 0;

  // ׼���ý��� irq �źź�, �ɽ��շ�����
  void ready_recv_irq() {
    if (mask == 0) {
      clr_ext_int(CpuCauseInt::hardware);
      return;
    }

    u32 m = mask & IRQ_REQUEST_BIT;
    u32 p = mask_prev & IRQ_REQUEST_BIT;
    if ((mask & IRQ_REQUEST_BIT) && NOT(mask_prev & IRQ_REQUEST_BIT)) {
      set_ext_int(CpuCauseInt::hardware);
    }
    mask_prev = mask;
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
  static const u32 DMA_LEN            = 8;
  static const u32 DMA_MASK           = 0x1F80'108F;
  static const u32 DMA_IRQ_WRITE_MASK = (1 << 24) - 1; // 24:DMAIrq::master_enable

  static const u32 IRQ_ST_WR_MASK     = (1 << 11) - 1; // 11:IrqDevMask count
  static const u32 IRQ_RST_FLG_BIT_MK = (1 << 31);

private:
  MMU& mmu;
  IrqReceiver* ir;

  DMADev* dmadev[DMA_LEN];
  DMAIrq  dma_irq;        
  DMADpcr dma_dpcr;       
  u32     irq_status;     // I_STAT
  u32     irq_mask;       // I_MASK

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
      CASE_IO_MIRROR(0x1F80'10F0):
        dma_dpcr.v = v;
        set_dma_dev_status();
        return;

      CASE_IO_MIRROR(0x1F80'10F4):
        dma_irq.v = v;
        if (v & IRQ_RST_FLG_BIT_MK) { 
          dma_irq.dd_flag = 0;
        }
        return;

      CASE_IO_MIRROR(0x1F80'1070):
        irq_status &= v & IRQ_ST_WR_MASK;
        update_irq_to_reciver();
        return;

      CASE_IO_MIRROR(0x1F80'1074):
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
    warn("WRIT BUS invaild %x: %x\n", addr, v);
  }


  template<class T> T read(psmem addr) {
    switch (addr) {
      CASE_IO_MIRROR(0x1F80'10F0):
        return dma_dpcr.v;

      CASE_IO_MIRROR(0x1F80'10F4):
        return dma_irq.v;

      CASE_IO_MIRROR(0x1F80'1070):
        return irq_status;

      CASE_IO_MIRROR(0x1F80'1074):
        return irq_mask;
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
    warn("READ BUS invaild %x\n", addr);
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