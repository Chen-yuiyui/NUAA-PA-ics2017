#include "common.h"
#include "fs.h"
#define DEFAULT_ENTRY ((void*) 0x8048000)
#include  "memory.h"
uintptr_t loader(_Protect *as, const char *filename) {
//  ramdisk_read(DEFAULT_ENTRY,0,get_ramdisk_size());
 int fd = fs_open(filename,0,0);
 size_t nbyte = fs_filesz(fd);
 Log("loaded:[%d]%s size:%d",fd,filename,nbyte);
 void *end = DEFAULT_ENTRY+nbyte;
 void *pa;
 void *va;
 for(va = DEFAULT_ENTRY;va<end;va+=PGSIZE)
 {
	pa = new_page();
//	 Log("Map va to pa: 0x%08x to 0x%08x", va, pa);
	 _map(as,va,pa);
	fs_read(fd,pa,(end-va)<PGSIZE ?(end - va):PGSIZE);
	Log("Map va to pa: 0x%08x to 0x%08x", va, pa);
 }
return (uintptr_t)DEFAULT_ENTRY;
}
