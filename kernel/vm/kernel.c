#include "../riscv.h"
#include "../suev.h"

extern char end[];

static void yield() {
  __asm__("li a7, 0\n"
          "ecall" ::
              : "a7");
}

static void print(char *s) {
  // page at [63] is SHARED page and used for buffer
  char *p = (char *)(63 * 4096UL);
  while ((*p++ = *s++))
    ;
  __asm__("li a7, 1\n"
          "ecall" ::
              : "a7");
}

static void memcpy(char *dst, const char *src, int len) {
  while (len--)
    *dst++ = *src++;
}

void main();

__attribute__((naked, section(".entry"))) void _entry() {
  // Validate the page at [15] to use it as stack.
  pvalidate(
      (union rmpe_attr){.validated = 0, .type = RMPE_PRIVATE, .gpn = 15}.bits);
  main();
}

void main() {
  pagetable_t gpt2 = (void *)(4096UL * (14));
  pagetable_t gpt1 = (void *)(4096UL * (13));
  pagetable_t gpt0 = (void *)(4096UL * (12));
  pvalidate((union rmpe_attr){
      .validated = 0, .type = RMPE_PRIVATE, .gpn = (uint64)gpt2 >> 12}
                .bits);
  pvalidate((union rmpe_attr){
      .validated = 0, .type = RMPE_PRIVATE, .gpn = (uint64)gpt1 >> 12}
                .bits);
  pvalidate((union rmpe_attr){
      .validated = 0, .type = RMPE_PRIVATE, .gpn = (uint64)gpt0 >> 12}
                .bits);

  // Indentity-mapping

  gpt2[PX(2, 0UL)] = PA2PTE(gpt1) | PTE_V;
  gpt1[PX(1, 0UL)] = PA2PTE(gpt0) | PTE_V;

  for (uint64 i = 0; i < 32; i++) {
    // Skip validating code, stack, page tables
    if (i >= ((uint64)end >> 12) && (i < 12 || i > 15))
      pvalidate(
          (union rmpe_attr){.validated = 0, .type = RMPE_PRIVATE, .gpn = i}
              .bits);
    gpt0[PX(0, i * 4096)] =
        PA2PTE(i * 4096) | PTE_PRIVATE | PTE_R | PTE_W | PTE_X | PTE_V;
  }
  for (uint64 i = 32; i < 48; i++) {
    pvalidate(
        (union rmpe_attr){.validated = 0, .type = RMPE_MERGEABLE, .gpn = i}
            .bits);
    gpt0[PX(0, i * 4096)] =
        PA2PTE(i * 4096) | PTE_MERGEABLE | PTE_R | PTE_W | PTE_X | PTE_V;
  }
  for (uint64 i = 48; i < 64; i++) {
    gpt0[PX(0, i * 4096)] = PA2PTE(i * 4096) | PTE_R | PTE_W | PTE_X | PTE_V;
  }

  w_satp(MAKE_ATP(gpt2));

  char *mergeable_area = (void *)(4096UL * 32);
  memcpy(mergeable_area, "Hello, World", sizeof("Hello, World"));
  // if (r_tp() == 10UL)
  //   mergeable_area[0] = 'x'; // ERROR on pmerge
  print("Wrote mergeable area:");
  print(mergeable_area);

  char *private_area = (void *)(4096UL * 16);
  memcpy(private_area, "Hello, Mom", sizeof("Hello, Mom"));

  // let hypervisor merge the page
  yield();

  // write-access the merged page
  // *mergeable_area = 'x'; // ERROR

  // read-access the merged page
  print("Reading mergeable area: ");
  print(mergeable_area);

  // let hypervisor unmerge the page
  yield();

  // write-access the unmerged page
  if (r_tp() == 10UL)
    *mergeable_area = 'x';
  else
    *mergeable_area = 'y';

  // read-access the unmerged page
  print("Reading mergeable area: ");
  print(mergeable_area);

  yield();

  gpt0[PX(0, 4096UL * 64)] =
      PA2PTE(4096UL * 64) | PTE_MERGEABLE | PTE_R | PTE_W | PTE_X | PTE_V;
  sfence_vma();
  // Let VM1 try to access VM0's MERGEABLE page
  // if (r_tp() == 11UL) {
  //   print("Accessing VM0's MERGEABLE page...");
  //   print((char *)(4096UL * 64)); // ERROR
  // }

  yield();

  // Let VM1 try to access VM0's FIXED MERGEABLE page
  // if (r_tp() == 11UL) {
  //   print("Accessing VM0's FIXED MERGEABLE page...");
  //   print((char *)(4096UL * 64)); // ERROR
  // }
  gpt0[PX(0, 4096UL * 64)] = 0;
  sfence_vma();

  yield();

  while (1)
    ;
}