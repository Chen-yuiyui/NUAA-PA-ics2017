#include "nemu.h"
#include "monitor/monitor.h"
#include "monitor/watchpoint.h"
/* The assembly code of instructions executed is only output to the screen
 * when the number of instructions executed is less than this value.
 * This is useful when you use the `si' command.
 * You can modify this value as you want.
 */
#define MAX_INSTR_TO_PRINT 500 
int nemu_state = NEMU_STOP;
WP *scan_watchpoint();
uint32_t expr(char *args,bool success);
void exec_wrapper(bool);
/* Simulate how the CPU works. */
void cpu_exec(uint64_t n) {
  if (nemu_state == NEMU_END) {
    printf("Program execution has ended. To restart the program, exit NEMU and run again.\n");
    return;
  }
  nemu_state = NEMU_RUNNING;
  bool print_flag =n < MAX_INSTR_TO_PRINT;
//  if (n ==-1)
  //  print_flag = 1;
  for (; n > 0; n --) {
    /* Execute one instruction, including instruction fetch,
     * instruction decode, and the actual execution. */
    exec_wrapper(print_flag);

#ifdef DEBUG
    /* TODO: check watchpoints here. */
//	bool success = true;
	WP *wp=scan_watchpoint();
	char str[32]={'\0'};
	if(wp !=NULL)
	{
	strcpy(str,wp->expr);
	char *x,*y;
	x=strtok(str," ");
	y=strtok(NULL," ");
	if(strcmp(x,"$eip")==0&&strcmp(y,"==")==0&&wp->old_val==1&&wp->new_val==0)
	{
	printf("The breakpoint has used!\n");	
	}
	else{
	//	bool  *success = 1;
	//	printf("Hit watchpoint %d at address 0x%x \n",wp->NO,cpu.eip);
		printf("expr   =%s\n",wp->expr);
		printf("old value = 0x%x\n",wp->old_val);
		printf("new value = 0x%x\n",wp->new_val);
		printf("program paused!\n");
		wp->old_val=wp->new_val;
		nemu_state = NEMU_STOP;
	}
	}
		
#endif







#ifdef HAS_IOE
    extern void device_update();
    device_update();
#endif

    if (nemu_state != NEMU_RUNNING) { return; }
  }

  if (nemu_state == NEMU_RUNNING) { nemu_state = NEMU_STOP; }
}
