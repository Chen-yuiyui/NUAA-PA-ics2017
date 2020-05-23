#include "cpu/exec.h"
#include "memory/mmu.h"
#include "common.h"
void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  rtl_push(&cpu.eflags.val);
  cpu.eflags.IF = 0;
  rtl_push(&cpu.cs);
  rtl_push(&ret_addr);
  t0=cpu.idtr.base;
  t1=t0+ NO*8;
  t0=vaddr_read(t1,2);
  t2=vaddr_read(t1+4,4);
  t1=t0+(t2&0xffff0000);
  decoding.is_jmp=1;
  decoding.jmp_eip = t1;
}

void dev_raise_intr() {
	cpu.INTR = true;
}
