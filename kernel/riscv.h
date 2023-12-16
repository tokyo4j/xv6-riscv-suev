#pragma once

#ifndef __ASSEMBLER__

#include "types.h"

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3) // machine-mode interrupt enable.

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

// Supervisor Interrupt Enable
#define SIE_SEIE (1L << 9) // external
#define SIE_STIE (1L << 5) // timer
#define SIE_SSIE (1L << 1) // software

// Machine-mode Interrupt Enable
#define MIE_MEIE (1L << 11) // external
#define MIE_MTIE (1L << 7)  // timer
#define MIE_MSIE (1L << 3)  // software

#define HSTATUS_SPV (1L << 7)

#define DEFINE_CSR(csr_name)                                                   \
  static inline __always_inline uint64 r_##csr_name() {                        \
    uint64 x;                                                                  \
    asm volatile("csrr %0," #csr_name : "=r"(x));                              \
    return x;                                                                  \
  }                                                                            \
  static inline __always_inline void w_##csr_name(uint64 x) {                  \
    asm volatile("csrw " #csr_name ", %0" : : "r"(x));                         \
  }

DEFINE_CSR(mhartid)
DEFINE_CSR(mstatus)
DEFINE_CSR(mepc)
DEFINE_CSR(sstatus)
DEFINE_CSR(sip)
DEFINE_CSR(sie)
DEFINE_CSR(mie)
DEFINE_CSR(sepc)
DEFINE_CSR(medeleg)
DEFINE_CSR(mideleg)
DEFINE_CSR(stvec)
DEFINE_CSR(mtvec)
DEFINE_CSR(pmpcfg0)
DEFINE_CSR(pmpaddr0)
DEFINE_CSR(satp)
DEFINE_CSR(mscratch)
DEFINE_CSR(scause)
DEFINE_CSR(stval)
DEFINE_CSR(hstatus)
DEFINE_CSR(hgatp)
DEFINE_CSR(vsatp)

// enable device interrupts
static inline void intr_on() { w_sstatus(r_sstatus() | SSTATUS_SIE); }

// disable device interrupts
static inline void intr_off() { w_sstatus(r_sstatus() & ~SSTATUS_SIE); }

// are device interrupts enabled?
static inline int intr_get() {
  uint64 x = r_sstatus();
  return (x & SSTATUS_SIE) != 0;
}

#define DEFINE_GPR(gpr_name)                                                   \
  static inline __always_inline uint64 r_##gpr_name() {                        \
    uint64 x;                                                                  \
    asm volatile("mv %0," #gpr_name : "=r"(x));                                \
    return x;                                                                  \
  }                                                                            \
  static inline __always_inline void w_##gpr_name(uint64 x) {                  \
    asm volatile("mv " #gpr_name ", %0" : : "r"(x));                           \
  }
DEFINE_GPR(sp)
DEFINE_GPR(tp)
DEFINE_GPR(ra)

// flush the TLB.
static __always_inline void sfence_vma() {
  // the zero, zero means flush all TLB entries.
  asm volatile("sfence.vma zero, zero");
}

typedef uint64 pte_t;
typedef uint64 *pagetable_t; // 512 PTEs

#endif // __ASSEMBLER__

#define PGSIZE 4096 // bytes per page
#define PGSHIFT 12  // bits of offset within a page

#define PGROUNDUP(sz) (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE - 1))

// clang-format off
#define PTE_V         0x0000000000000001UL // valid
#define PTE_R         0x0000000000000002UL
#define PTE_W         0x0000000000000004UL
#define PTE_X         0x0000000000000008UL
#define PTE_U         0x0000000000000010UL // user can access
#define PTE_FLAGS     0x00000000000003ffUL
#define PTE_PPN       0x07fffffffffffc00UL
#define PTE_PRIVATE   0x0800000000000000UL
#define PTE_MERGEABLE 0x1000000000000000UL
#define PTE_RMPE_TYPE 0x1800000000000000UL
// clang-format on

// use riscv's sv39 page table scheme.
#define ATP_SV39 (8L << 60)

#define MAKE_ATP(pagetable) (ATP_SV39 | (((uint64)pagetable) >> 12))
#define ATP2PT(apt) ((pagetable_t)(((uint64)(apt) & ((1UL << 44) - 1)) << 12))

// shift a physical address to the right place for a PTE.
#define PA2PTE(pa) ((((uint64)pa) >> 12) << 10)

#define PTE2PA(pte) (((pte)&PTE_PPN) << 2)

// extract the three 9-bit page table indices from a virtual address.
#define PXMASK 0x1FF // 9 bits
#define PXSHIFT(level) (PGSHIFT + (9 * (level)))
#define PX(level, va) ((((uint64)(va)) >> PXSHIFT(level)) & PXMASK)

// one beyond the highest possible virtual address.
// MAXVA is actually one bit less than the max allowed by
// Sv39, to avoid having to sign-extend virtual addresses
// that have the high bit set.
#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))
