#pragma once

#include "types.h"

#define CSR_HRMPBASE 0x6c0
#define CSR_HRMPLEN 0x6c1

#define RMPE_SHARED 0
#define RMPE_PRIVATE 1
#define RMPE_MERGEABLE 2
#define RMPE_LEAF 3

// for pvalidate, asid must be zero
#define ASID_RANGE 256

/* For RMPE leaf, only gPA, VALIDATED and gen are effective */
union rmpe_attr {
  struct {
    uint64 validated : 1;
    uint64 fixed : 1;
    uint64 type : 2;
    uint64 asid : 8;
    uint64 gpn : 44;
    uint64 : 8;
  };
  uint64 bits;
};

#define RMPE_VALIDATED_MASK 0x0000000000000001UL
#define RMPE_FIXED_MASK 0x0000000000000002UL
#define RMPE_TYPE_MASK 0x0000000000000010UL
#define RMPE_ASID_MASK 0x0000000000000fe0UL
#define RMPE_GPN_MASK 0x00fffffffffff000UL

struct rmpe {
  union rmpe_attr attr;
  uint64 gen;
};

struct guest_ctx {
  uint64 gen;
  enum {
    GUEST_STATE_INIT,
    GUEST_STATE_CREATED,
    GUEST_STATE_ACTIVATED,
  } state;
};

static __always_inline void rmpupdate(uint64 spa, uint64 rmpe_attrs) {
  __asm__("mv a0, %0\n"
          "mv a1, %1\n"
          ".word 0x10000b\n" ::"r"(spa),
          "r"(rmpe_attrs)
          : "a0", "a1");
}

static __always_inline void vmcreate(uint64 asid) {
  __asm__("mv a0, %0\n"
          ".word 0x20000b\n" ::"r"(asid)
          : "a0");
}

static __always_inline void vmactivate(uint64 asid) {
  __asm__("mv a0, %0\n"
          ".word 0x30000b\n" ::"r"(asid)
          : "a0");
}

static __always_inline void vmupdatedata(uint64 dest_paddr, uint64 src_paddr,
                                         uint64 len) {
  __asm__("mv a0, %0\n"
          "mv a1, %1\n"
          "mv a2, %2\n"
          ".word 0x40000b\n" ::"r"(dest_paddr),
          "r"(src_paddr), "r"(len)
          : "a0", "a1", "a2");
}

static __always_inline void vmdestroy(uint64 asid) {
  __asm__("mv a0, %0\n"
          ".word 0x50000b\n" ::"r"(asid)
          : "a0");
}

static __always_inline void pvalidate(uint64 rmpe_attrs) {
  __asm__("mv a0, %0\n"
          ".word 0x60000b\n" ::"r"(rmpe_attrs)
          : "a0");
}

static __always_inline void pfix(uint64 hpa, uint64 leaf_hpa) {
  __asm__("mv a0, %0\n"
          "mv a1, %1\n"
          ".word 0x70000b\n" ::"r"(hpa),
          "r"(leaf_hpa)
          : "a0", "a1");
}

static __always_inline void punfix(uint64 hpa, uint64 asid) {
  __asm__("mv a0, %0\n"
          "mv a1, %1\n"
          ".word 0x80000b\n" ::"r"(hpa),
          "r"(asid)
          : "a0", "a1");
}

static __always_inline void pmerge(uint64 dst_hpa, uint64 src_hpa) {
  __asm__("mv a0, %0\n"
          "mv a1, %1\n"
          ".word 0x90000b\n" ::"r"(dst_hpa),
          "r"(src_hpa)
          : "a0", "a1");
}

static __always_inline void punmerge(uint64 dst_hpa, uint64 src_hpa,
                                     uint64 asid) {
  __asm__("mv a0, %0\n"
          "mv a1, %1\n"
          "mv a2, %2\n"
          ".word 0xa0000b\n" ::"r"(dst_hpa),
          "r"(src_hpa), "r"(asid)
          : "a0", "a1", "a2");
}
