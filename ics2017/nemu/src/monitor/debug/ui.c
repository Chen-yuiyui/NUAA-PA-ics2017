#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
int set_watchpoint(char *args);
bool delete_watchpoint(int NO);
void list_watchpoint();
WP *scan_watchpoint();
void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args)
{
	return -1;
}
static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} 

cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","Single step",cmd_si},
  { "info","output the values of all registers",cmd_info},
  { "x","print the memory",cmd_x},
  { "p","print the result",cmd_p},
  { "w","set the watchpoint",cmd_w},
  { "d","delete the watchpoint",cmd_d}
 	 /* TODO: Add more commands */
	  

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}
static int cmd_si(char *args){
	char *str;
	str=strtok(NULL," ");
	int N=1;
	if(str!=NULL)
	{
	sscanf(str,"%d",&N);
	}
	cpu_exec(N);
	return 0;
}

static int cmd_info(char *args)
{
	char *str;
	str=strtok(NULL," ");
	if(*(str)=='r')
	{
		
		for(int i=0;i<8;i++)
		{	
		printf("%s:\t0x%08x\t ",regsl[i],cpu.gpr[i]._32);
		printf("%d\n",cpu.gpr[i]._32);
		}
		printf("\n");
		for(int i=0;i<8;i++)
		{	
		printf("%s:\t0x%08x\t ",regsw[i],cpu.gpr[i]._16);
		printf("%d\n",cpu.gpr[i]._16);
		}
		printf("\n");
		for(int i=0;i<8;i++)
		{
		printf("%s:\t%02x\t ",regsb[i],cpu.gpr[i]._8[0]);
		printf("%d\n",cpu.gpr[i]._8[0]);	
		}
		printf("\n");
	}
	if(*(str)=='w')
	{
	list_watchpoint();
	}	
	return 0;
}

static int cmd_x(char *args)

{
	char *str1 = strtok(NULL," ");
	char *str2 = strtok(NULL," ");
	int N;
	vaddr_t addr1,addr2;
	sscanf(str1,"%d",&N);
	sscanf(str2,"%x",&addr1);
	sscanf(str2,"%x",&addr2);
	printf("Address	 Dword block  Byte sequence\n");     
	for(int i=0;i<N;i++)
	{	printf("0x%08x ",addr1);
		printf("0x%08x ",vaddr_read(addr1,4));
		for(int j=0;j<4;j++)
		{
		printf("%02x ",vaddr_read(addr2,1));
		addr2=addr2+1;
		}
		printf("\n");
		addr1=addr1+4;
	
	}
	return 0;
	
}

static int cmd_p(char *args)
{
	bool *success = false;
        printf("%x\n",expr(args,success));
	return 1;	
}

static int cmd_w(char *args)
{

	if(args == NULL)
		printf("bad expression!invalid!\n");
	else
		 set_watchpoint(args);
	return 0;
}


static int cmd_d(char *args)
{
	if(args == NULL)
	{
	printf("Invalid expression!!\n");
	}
	else
	{
		int i;
		sscanf(args,"%d",&i);
		delete_watchpoint(i);
	}
	return 0;

}
void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
