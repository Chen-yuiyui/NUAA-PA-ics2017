#include "nemu.h"
#include "device/mmio.h"
#include"memory/mmu.h"
#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
int mmio_n;
if((mmio_n = is_mmio(addr))!= -1)
	return mmio_read(addr,len,mmio_n);
else
      	return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
 int mmio_n;
 if((mmio_n = is_mmio(addr))!=-1)
	 mmio_write(addr,len,data,mmio_n);
 else
       	memcpy(guest_to_host(addr), &data, len);
}

paddr_t page_translate(vaddr_t addr, bool is_write) {

 if (!cpu.cr0.paging) return addr;
 // Log("page_translate: addr: 0x%x\n", addr);
  paddr_t dir = (addr >> 22) & 0x3ff;
  paddr_t page = (addr >> 12) & 0x3ff;
  paddr_t offset = addr & 0xfff;
  paddr_t PDT_base = cpu.cr3.page_directory_base;
  //Log("page_translate: dir: 0x%x page: 0x%x offset: 0x%x PDT_base: 0x%x\n", dir, page, offset, PDT_base);
  PDE pde;
  pde.val = paddr_read((PDT_base << 12) + (dir << 2), 4);
  if (!pde.present) {
    Log("page_translate: addr: 0x%x\n", addr);
    Log("page_translate: dir: 0x%x page: 0x%x offset: 0x%x PDT_base: 0x%x\n", dir, page, offset, PDT_base);
    assert(pde.present);
  }
  PTE pte;
  // Log("page_translate: page_frame: 0x%x\n", pde.page_frame);
  pte.val = paddr_read((pde.page_frame << 12) + (page << 2), 4);
  if (!pte.present) {
    Log("page_translate: addr: 0x%x\n", addr);
    assert(pte.present);
  }
  paddr_t paddr = (pte.page_frame << 12) | offset;
  //Log("page_translate: paddr: 0x%x\n", paddr);
  return paddr;
  /*uint32_t DIR = vaddr >> 22;
  uint32_t PAGE = vaddr >> 12 & 0x000003FF;
  uint32_t OFFSET = vaddr & 0x00000FFF;
  paddr_t PhysicalAddr = vaddr;


  if (cpu.cr0.val & 0x80000000) {
    uint32_t PageTable = paddr_read(cpu.cr3.val + 4 * DIR, 4) & 0xFFFFF000;
    if(!(paddr_read(cpu.cr3.val + 4 * DIR, 4) & 0x00000001)) {
      Log("FATAL: Virtual Address is 0x%08X", vaddr);
      Log("FATAL: eip = 0x%08X at PD", cpu.eip);
    }
    assert(paddr_read(cpu.cr3.val + 4 * DIR, 4) & 0x00000001); // Present
    paddr_write(cpu.cr3.val + 4 * DIR, 4, (paddr_read(cpu.cr3.val + 4 * DIR, 4) | 0x00000020)); // Set accessed
    uint32_t PageTableEntry = paddr_read(PageTable + 4 * PAGE, 4);
    if(!(PageTableEntry & 0x00000001)) {
      Log("FATAL: Virtual Address is 0x%08X", vaddr);
      Log("FATAL: eip = 0x%08X at PT", cpu.eip);
    }
    assert(PageTableEntry & 0x00000001); // Present
    paddr_write(PageTable + 4 * PAGE, 4, (paddr_read(PageTable + 4 * PAGE, 4) | 0x00000020)); // Set accessed
    if (is_write) 
      paddr_write(PageTable + 4 * PAGE, 4, (paddr_read(PageTable + 4 * PAGE, 4) | 0x00000040)); // Set dirty
    PhysicalAddr = (paddr_read(PageTable + 4 * PAGE, 4) & 0xFFFFF000) + OFFSET;

    //Log("PhysicalAddr = 0x%08X", PhysicalAddr);
  }
  return PhysicalAddr;
	      */
}
#define CROSS_PAGE(addr, len) \
  ((((addr) + (len) - 1) & ~PAGE_MASK) != ((addr) & ~PAGE_MASK))

uint32_t vaddr_read(vaddr_t addr, int len) {
 paddr_t paddr;

  if (CROSS_PAGE(addr, len)) {
    /* data cross the page boundary */
    union {
      uint8_t bytes[4];
      uint32_t dword;
    } data = {0};
    for (int i = 0; i < len; i++) {
      paddr = page_translate(addr + i, false);
      data.bytes[i] = (uint8_t)paddr_read(paddr, 1);
    }
    return data.dword;
  } else {
    paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }
}
 /*uint32_t OFFSET = addr & 0x00000FFF;
  if (cpu.cr0.val & 0x80000000) {
    if (OFFSET + len > 0x00001000) {
      int len1 = 0x00001000 - OFFSET;
      int len2 = len - len1;
      uint32_t part1 = paddr_read(page_translate(addr, false), len1); // low len1 bytes
      uint32_t part2 = paddr_read(page_translate(addr + len1, false), len2); // high len2 bytes
      // stich together
      return part1 + (part2 << (len1 * 8));
    }

    return paddr_read(page_translate(addr, false), len);
  }
  return paddr_read(addr, len);*/
  


void vaddr_write(vaddr_t addr, int len, uint32_t data) {
 /*uint32_t OFFSET = addr & 0x00000FFF;
  if (cpu.cr0.val & 0x80000000) {
    if (OFFSET + len > 0x00001000) {
      int len1 = 0x00001000 - OFFSET;
      int len2 = len - len1;
      paddr_write(page_translate(addr, true), len1, data); // low len1 bytes
      data >>= (len1 * 8);
      paddr_write(page_translate(addr + len1, true), len2, data); // high len2 bytes
    }
    else
      paddr_write(page_translate(addr, true), len, data);
  }
  else
    paddr_write(addr, len, data);*/
     paddr_t paddr;

  if (CROSS_PAGE(addr, len)) {
    /* data cross the page boundary */
    assert(0);
    for (int i = 0; i < len; i++) {
      paddr = page_translate(addr, true);
      paddr_write(paddr, 1, data);
      data >>= 8;
      addr++;
    }
  } else {
    paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
  }
}



