#include "proc.h"

#define MAX_NR_PROC 4
static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
//  _switch(&pcb[i].as);
//  current = &pcb[i];
//  ((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

/*static PCB *current_game = &pcb[0];
void switch_game() {
	 current_game = (current_game == &pcb[0] ? &pcb[2] : &pcb[0]);
}*/

#define PROCWEIGHT 200
int prioritycounter = 0;


_RegSet* schedule(_RegSet *prev) {
  // save the context pointer
  current->tf = prev;
//  int count=200;
  // always select pcb[0] as the new process
// current = (current == current_game ? &pcb[1] : current_game);
  /* if(current ==current_game){
  `while(count)	
	{
	current = current_game;
	count --;
	}
   }
  else
	  current = &pcb[1];
 */
   current = (current == &pcb[0] && prioritycounter > PROCWEIGHT ? &pcb[1] : &pcb[0]);
   if(prioritycounter > PROCWEIGHT)
	    prioritycounter = 0;
   prioritycounter++;
  _switch(&current->as);
 return current->tf;
}
