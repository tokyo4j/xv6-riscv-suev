K=kernel
U=user

MEMFS=y

OBJS = \
  $K/entry.o \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/kalloc.o \
  $K/spinlock.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/bio.o \
  $K/fs.o \
  $K/log.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/kernelvec.o \
  $K/plic.o \
  $K/virtio_disk.o \
  $K/suev.o

#QEMU = gdbserver :2345 ../qemu/build/qemu-system-riscv64
QEMU = ../qemu/build/qemu-system-riscv64

CC = riscv64-linux-gnu-gcc
AS = riscv64-linux-gnu-as
LD = riscv64-linux-gnu-ld
OBJCOPY = riscv64-linux-gnu-objcopy
OBJDUMP = riscv64-linux-gnu-objdump

CFLAGS = -O0 -ggdb -fno-omit-frame-pointer -Wall
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -MD
CFLAGS += -mcmodel=medany
CFLAGS += -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS += -I.
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
CFLAGS += -fno-pie -no-pie
ifeq ($(MEMFS), y)
CFLAGS += -DMEMFS=y
endif

LDFLAGS = -z max-page-size=4096

ifneq ($(MEMFS), y)
$K/kernel: $(OBJS) $K/kernel.ld $K/vm/kernel.img.o
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS)
else
$K/kernel: $(OBJS) $K/kernel.ld $K/vm/kernel.img.o fs.img.o
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $K/vm/kernel.img.o fs.img.o
endif

fs.img.o: fs.img
	$(LD) -r -b binary -o $@ $<

$K/vm/kernel.out: $K/vm/kernel.o $K/vm/kernel.ld
	$(LD) $(LDFLAGS) -T $K/vm/kernel.ld -o $@ $<
$K/vm/kernel.img: $K/vm/kernel.out
	$(OBJCOPY) -S -O binary $< $@
$K/vm/kernel.img.o: $K/vm/kernel.img
	$(OBJCOPY) -I binary -O elf64-littleriscv --set-section-alignment .data=4096 $< $@

$U/initcode: $U/initcode.S
	$(CC) $(CFLAGS) -march=rv64g -nostdinc -I. -Ikernel -c $U/initcode.S -o $U/initcode.o
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o $U/initcode.out $U/initcode.o
	$(OBJCOPY) -S -O binary $U/initcode.out $U/initcode

ULIB = $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^

$U/usys.S : $U/usys.pl
	perl $U/usys.pl > $U/usys.S

$U/usys.o : $U/usys.S
	$(CC) $(CFLAGS) -c -o $U/usys.o $U/usys.S

$U/_forktest: $U/forktest.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $U/_forktest $U/forktest.o $U/ulib.o $U/usys.o

mkfs/mkfs: mkfs/mkfs.c $K/fs.h $K/param.h
	gcc -Werror -Wall -I. -o mkfs/mkfs mkfs/mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	$U/_cat\
	$U/_echo\
	$U/_forktest\
	$U/_grep\
	$U/_init\
	$U/_kill\
	$U/_ln\
	$U/_ls\
	$U/_mkdir\
	$U/_rm\
	$U/_sh\
	$U/_stressfs\
	$U/_usertests\
	$U/_grind\
	$U/_wc\
	$U/_zombie\

fs.img: mkfs/mkfs README $(UPROGS)
	mkfs/mkfs fs.img README $(UPROGS)

-include kernel/*.d user/*.d

clean:
	find -iname '*.img' -o \
				-iname '*.o' -o \
				-iname '*.out' -o \
				-iname '*.d' | xargs rm -f
	rm -f $U/initcode $K/kernel $K/vm/kernel fs.img \
				mkfs/mkfs .gdbinit \
        $U/usys.S $(UPROGS)

QEMUGDB = -s -S
ifndef CPUS
CPUS := 1
endif

QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 256M -smp $(CPUS) -nographic
ifneq ($(MEMFS), y)
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=fs.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
endif

#QEMUOPTS := $(filter-out -nographic, $(QEMUOPTS))
#QEMUOPTS += -monitor stdio

qemu: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS)

qemu-gdb: $K/kernel fs.img
	$(QEMU) $(QEMUOPTS) -S -s