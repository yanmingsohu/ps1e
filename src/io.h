#pragma once

#include "util.h"

namespace ps1e {

#define CASE_IO_MIRROR_WRITE(addr, deviomap, io_map_obj, v) \
    CASE_IO_MIRROR(addr): \
    io[ static_cast<size_t>(deviomap) ]->write(v); \
    return;

#define CASE_IO_MIRROR_READ(addr, deviomap, io_map_obj, _) \
    CASE_IO_MIRROR(addr): \
    return io[ static_cast<size_t>(deviomap) ]->read();

#define IO_MIRROR_CASES_IN_SWITCH(rw, io_map_obj, v) \
    rw(0x1F80'1810, DeviceIOMapper::gpu_gp0, io_map_obj, v) \
    rw(0x1F80'1814, DeviceIOMapper::gpu_gp1, io_map_obj, v) \
  

// IO �ӿ�ö��
enum class DeviceIOMapper : size_t {
  none = 0,   // Keep first and value 0
  gpu_gp0,
  gpu_gp1,
  __Length__, // Keep last, Do not Index.
};


// �豸�ϵ�һ�� IO �˿�, Ĭ��ʲô������
class DeviceIO {
public:
  virtual ~DeviceIO() {}
  virtual void write(u32 value) {}
  virtual u32 read() { return 0xFFFF'FFFF; }
};


// �������湦�ܵĽӿ�
class DeviceIOLatch : public DeviceIO {
protected:
  u32 reg;
public:
  virtual void write(u32 value) {
    reg = value;
  }
  virtual u32 read() { 
    return reg;
  }
};

}