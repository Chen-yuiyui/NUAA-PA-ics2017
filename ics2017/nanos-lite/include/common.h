#ifndef __COMMON_H__
#define __COMMON_H__

#include <am.h>
#include <klib.h>
#include "debug.h"
void ramdisk_read();
void ramdisk_write();
size_t get_ramdisk_size();
typedef char bool;
#define true 1
#define false 0

#endif
