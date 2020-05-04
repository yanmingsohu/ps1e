#pragma once

#include "util.h"

namespace ps1e {

#define CASE_IO_MIRROR(x) CASE_MEM_MIRROR(x)

#define CASE_IO_MIRROR_WRITE(addr, io_enum, io_arr, v) \
    CASE_IO_MIRROR(addr): \
    io_arr[ static_cast<size_t>(DeviceIOMapper::io_enum) ]->write(v); \
    return;

#define CASE_IO_MIRROR_READ(addr, io_enum, io_arr, _) \
    CASE_IO_MIRROR(addr): \
    return io_arr[ static_cast<size_t>(DeviceIOMapper::io_enum) ]->read();

#define IO_ENUM_DEFINE_FIELDS(_, enumfield, __, ___) \
    enumfield,


//
// ���йҽ��������ϵ��豸�� io �����
// TODO: DMA �豸ʶ��� bus �ƶ�������.
//
#define IO_MIRRORS_STATEMENTS(rw, io_arr, v) \
    rw(0x1F80'1810, gpu_gp0, io_arr, v) \
    rw(0x1F80'1814, gpu_gp1, io_arr, v) \
  

// IO �ӿ�ö��
enum class DeviceIOMapper : size_t {
  __zero__ = 0,   // Keep first and value 0
  IO_MIRRORS_STATEMENTS(IO_ENUM_DEFINE_FIELDS, 0, 0)
  __Length__, // Keep last, Do not Index.
};
const size_t io_map_size = static_cast<size_t>(DeviceIOMapper::__Length__);


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