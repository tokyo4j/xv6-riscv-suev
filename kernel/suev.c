#include "suev.h"
#include "defs.h"
#include "memlayout.h"
#include "riscv.h"

extern char etext[], end[], heap[];
extern char _rmp_start[], _rmp_size[];
extern char _vm_start[], _vm_size[];

#define VM_PAGE(asid, gpn) (&_vm_start[((asid)*64 + (gpn)) * 4096])

__attribute__((naked)) static void _vm_run(struct trapframe *tf) {
  __asm__("addi sp, sp, -112\n"
          "sd ra, 0(sp)\n"
          // skip sp
          "sd s0, 8(sp)\n"
          "sd s1, 16(sp)\n"
          "sd s2, 24(sp)\n"
          "sd s3, 32(sp)\n"
          "sd s4, 40(sp)\n"
          "sd s5, 48(sp)\n"
          "sd s6, 56(sp)\n"
          "sd s7, 64(sp)\n"
          "sd s8, 72(sp)\n"
          "sd s9, 80(sp)\n"
          "sd s10, 88(sp)\n"
          "sd s11, 96(sp)\n"
          "csrr t0, stvec\n" // also store kernel's trap vector
          "sd t0, 104(sp)\n"
          // Set trap VM's trap vector to next address of sret so kernel can
          // regard VM's execution as a single instruction.
          "la t0, 1f\n"
          "csrw stvec, t0\n"

          "ld t0, 0(a0)\n"
          "csrw vsatp, t0\n"
          "sd sp, 8(a0)\n" // store kernel's sp in trapframe
          "ld t0, 16(a0)\n"
          "csrw hgatp, t0\n"
          "ld t0, 24(a0)\n"
          "csrw sepc, t0\n"
          "ld ra, 40(a0)\n"
          "ld sp, 48(a0)\n"
          "ld gp, 56(a0)\n"
          "ld tp, 64(a0)\n"
          "ld t0, 72(a0)\n"
          "ld t1, 80(a0)\n"
          "ld t2, 88(a0)\n"
          "ld s0, 96(a0)\n"
          "ld s1, 104(a0)\n"
          // skip a0 for now
          "ld a1, 120(a0)\n"
          "ld a2, 128(a0)\n"
          "ld a3, 136(a0)\n"
          "ld a4, 144(a0)\n"
          "ld a5, 152(a0)\n"
          "ld a6, 160(a0)\n"
          "ld a7, 168(a0)\n"
          "ld s2, 176(a0)\n"
          "ld s3, 184(a0)\n"
          "ld s4, 192(a0)\n"
          "ld s5, 200(a0)\n"
          "ld s6, 208(a0)\n"
          "ld s7, 216(a0)\n"
          "ld s8, 224(a0)\n"
          "ld s9, 232(a0)\n"
          "ld s10, 240(a0)\n"
          "ld s11, 248(a0)\n"
          "ld t3, 256(a0)\n"
          "ld t4, 264(a0)\n"
          "ld t5, 272(a0)\n"
          "ld t6, 280(a0)\n"
          "csrw sscratch, a0\n" // store pointer to trapframe in sscratch
          "ld a0, 112(a0)\n"    // load skipped a0
          "sret\n"

          ".align 4\n"
          "1:\n"
          "csrrw a0, sscratch, a0\n" // store VM's a0 in sscratch and load
                                     // pointer to trapframe
          "sd ra, 40(a0)\n"
          "sd sp, 48(a0)\n"
          "sd gp, 56(a0)\n"
          "sd tp, 64(a0)\n"
          "sd t0, 72(a0)\n"
          "sd t1, 80(a0)\n"
          "sd t2, 88(a0)\n"
          "sd s0, 96(a0)\n"
          "sd s1, 104(a0)\n"
          // skip a0 for now
          "sd a1, 120(a0)\n"
          "sd a2, 128(a0)\n"
          "sd a3, 136(a0)\n"
          "sd a4, 144(a0)\n"
          "sd a5, 152(a0)\n"
          "sd a6, 160(a0)\n"
          "sd a7, 168(a0)\n"
          "sd s2, 176(a0)\n"
          "sd s3, 184(a0)\n"
          "sd s4, 192(a0)\n"
          "sd s5, 200(a0)\n"
          "sd s6, 208(a0)\n"
          "sd s7, 216(a0)\n"
          "sd s8, 224(a0)\n"
          "sd s9, 232(a0)\n"
          "sd s10, 240(a0)\n"
          "sd s11, 248(a0)\n"
          "sd t3, 256(a0)\n"
          "sd t4, 264(a0)\n"
          "sd t5, 272(a0)\n"
          "sd t6, 280(a0)\n"

          "csrr t0, vsatp\n"
          "sd t0, 0(a0)\n"
          "ld sp, 8(a0)\n" // load kernel's sp
          "csrr t0, hgatp\n"
          "sd t0, 16(a0)\n"
          "csrr t0, sepc\n"
          "sd t0, 24(a0)\n"

          "csrr t0, sscratch\n"
          "sd t0, 112(a0)\n" // store skipped VM's a0

          "ld ra, 0(sp)\n"
          // sp is already loaded
          "ld s0, 8(sp)\n"
          "ld s1, 16(sp)\n"
          "ld s2, 24(sp)\n"
          "ld s3, 32(sp)\n"
          "ld s4, 40(sp)\n"
          "ld s5, 48(sp)\n"
          "ld s6, 56(sp)\n"
          "ld s7, 64(sp)\n"
          "ld s8, 72(sp)\n"
          "ld s9, 80(sp)\n"
          "ld s10, 88(sp)\n"
          "ld s11, 96(sp)\n"
          "ld t0, 104(sp)\n" // restore kernel's trap vector
          "csrw stvec, t0\n"

          "addi sp, sp, 112\n"
          "ret\n");
}

static void vm_run(struct trapframe *tf) {
  printf("VM[%d]: starting\n", tf->tp);
reenter:
  _vm_run(tf);
  if (r_scause() == 10) {
    // ecall
    if (tf->a7 == 0) {
      // yield
      printf("VM[%d]: yielded\n", tf->tp);
      tf->epc += 4;
    } else if (tf->a7 == 1) {
      // print
      printf("VM[%d]:   %s\n", tf->tp, (char *)(VM_PAGE(tf->tp, 63)));
      tf->epc += 4;
      goto reenter;
    }
  } else if (!(r_scause() & (1UL << 63))) {
    static const char *const riscv_excp_names[] = {
        "misaligned_fetch",
        "fault_fetch",
        "illegal_instruction",
        "breakpoint",
        "misaligned_load",
        "fault_load",
        "misaligned_store",
        "fault_store",
        "user_ecall",
        "supervisor_ecall",
        "hypervisor_ecall",
        "machine_ecall",
        "exec_page_fault",
        "load_page_fault",
        "reserved",
        "store_page_fault",
        "reserved",
        "reserved",
        "reserved",
        "reserved",
        "guest_exec_page_fault",
        "guest_load_page_fault",
        "reserved",
        "guest_store_page_fault",
    };
    printf("VM[%d]: unexpected exception\n"
           "desc=%s, epc=%p, tval=%p\n",
           tf->tp, riscv_excp_names[r_scause()], tf->epc, r_stval());
    while (1)
      ;
  } else {
    panic("VM[%d]: unexpected interrupt");
  }
}

struct trapframe *make_vm_tf(uint64 asid) {
  pagetable_t npt = kalloc();
  memset(npt, 0, 4096);
  vmcreate(asid);

  // Identity-map kernel text executable and read-only.
  kvmmap(npt, KERNBASE, KERNBASE, (uint64)etext - KERNBASE,
         PTE_R | PTE_X | PTE_U);
  // Identity-map kernel data and the physical RAM we'll make use of.
  kvmmap(npt, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext,
         PTE_R | PTE_W | PTE_U);

  // Every VM owns 64 pages.
  // 0-31: PRIVATE ([0][1]code [12][13][14]page tables [15]stack)
  // 32-47 MERGEABLE
  // 48-63 SHARED ([63] print buffer)
  kvmmap(npt, 0UL, (uint64)VM_PAGE(asid, 0), 64 * 4096UL,
         PTE_R | PTE_W | PTE_X | PTE_U);
  for (int i = 0; i < 32; i++)
    rmpupdate(
        (uint64)VM_PAGE(asid, i),
        (union rmpe_attr){.type = RMPE_PRIVATE, .asid = asid, .gpn = i}.bits);
  for (int i = 32; i < 48; i++)
    rmpupdate(
        (uint64)VM_PAGE(asid, i),
        (union rmpe_attr){.type = RMPE_MERGEABLE, .asid = asid, .gpn = i}.bits);

  extern char _binary_kernel_vm_kernel_img_start[],
      _binary_kernel_vm_kernel_img_size[];
  if ((uint64)_binary_kernel_vm_kernel_img_size & 4095UL)
    panic("VM binary size is not page-aligned");

  vmupdatedata((uint64)VM_PAGE(asid, 0),
               (uint64)_binary_kernel_vm_kernel_img_start,
               (uint64)_binary_kernel_vm_kernel_img_size);
  vmactivate(asid);

  struct trapframe *tf = kalloc();
  tf->epc = 0UL;
  tf->hgatp = MAKE_ATP(npt) | (asid << 44);
  // Disable paging at VM boot
  tf->vsatp = 0UL;
  // VMs use the page at [15] as stack
  tf->sp = 16 * 4096;
  // Store VM's ASID in tp
  tf->tp = asid;

  return tf;
}

void suev_test() {
  printf("RMP base:%p len:%p\n", _rmp_start, _rmp_size);
  printf("Testing SUEV features...\n");

  asm volatile("csrw 0x6c0, %0" : : "r"(_rmp_start)); // CSR_HRMPBASE=0x6c0
  asm volatile("csrw 0x6c1, %0" : : "r"(_rmp_size));  // CSR_HRMP=0x6c1

  struct trapframe *tf0, *tf1;

  tf0 = make_vm_tf(10);
  tf1 = make_vm_tf(11);

  w_hstatus(r_hstatus() | HSTATUS_SPV);
  w_sstatus(r_sstatus() | SSTATUS_SPP);

  vm_run(tf0);
  vm_run(tf1);

  printf("Merging pages...\n");
  char *leaf = kalloc();
  rmpupdate((uint64)leaf, (union rmpe_attr){.type = RMPE_LEAF}.bits);
  pfix((uint64)VM_PAGE(tf0->tp, 32), (uint64)leaf);
  pmerge((uint64)VM_PAGE(tf0->tp, 32), (uint64)VM_PAGE(tf1->tp, 32));
  pte_t *pte0 = walk(ATP2PT(tf0->hgatp), 4096UL * 32, 0);
  pte_t *pte1 = walk(ATP2PT(tf1->hgatp), 4096UL * 32, 0);
  pte_t old_pte1 = *pte1;
  *pte1 = (*pte0 & PTE_PPN) | (*pte1 & ~PTE_PPN);
  sfence_vma();
  // fill freed page with garbage
  memset((char *)VM_PAGE(tf1->tp, 32), 'x', 4096);
  vm_run(tf0);
  vm_run(tf1);

  printf("Unmerging pages...\n");
  punmerge((uint64)VM_PAGE(tf1->tp, 32), (uint64)VM_PAGE(tf0->tp, 32), tf1->tp);
  *pte1 = old_pte1;
  punfix((uint64)VM_PAGE(tf0->tp, 32), tf0->tp);
  vm_run(tf0);
  vm_run(tf1);

  // Let VM1 try to access VM0's MERGEABLE page
  pte1 = walk(ATP2PT(tf1->hgatp), 4096UL * 64, 0);
  old_pte1 = *pte1;
  *pte1 = PA2PTE(VM_PAGE(tf0->tp, 32)) | PTE_R | PTE_W | PTE_X | PTE_V | PTE_U;
  vm_run(tf0);
  vm_run(tf1);

  // Let VM1 try to access VM0's FIXED MERGEABLE page
  pfix((uint64)VM_PAGE(tf0->tp, 32), (uint64)leaf);
  vm_run(tf0);
  vm_run(tf1);

  // Try to execute VM0 which is in INIT state
  // vmdestroy(tf0->tp);
  vm_run(tf0);
  vm_run(tf1);

  while (1)
    ;
}