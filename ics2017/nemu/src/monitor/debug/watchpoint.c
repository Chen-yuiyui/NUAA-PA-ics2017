#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP *new_wp()
{

	if(free_==NULL)
	{
		printf("there is no places!!\n");
		assert(0);
	}
	WP* result =free_;
	free_=free_->next;
	result->next=NULL;
	return result;
}


void free_wp(WP *wp)
{
	if(head==NULL||wp==NULL)
	return;

	bool Found=false;
	WP *present=head;
	if(head->NO==wp->NO)
	{
		head=head->next;
		present->next = free_;
		free_ = present;
		Found = true;
	}
	else
		{
			while(present->next)
			{
				if(present->next->NO==wp->NO)
				{
					present->next = present->next->next;
					wp->next = free_;
					free_ = wp;
					Found = true;
					break;
				}
			
			}
		if(Found)
		{
			printf("The watchpoint NO.%d( %s) is deleted!\n",wp->NO,wp->expr);
		(wp->expr)[0]='\0';
		wp->old_val = 0;
		}
		else
		{
			printf("The watchpoint does't exist!\n");

		}
		}		
}


int set_watchpoint(char *args)
{
	bool success = true;
	uint32_t val = expr(args,&success);

	if(!success)
	{
		printf("invalid expression!Faild to create watchpoint!\n");
		return -1;
	}
	
	WP *wp = new_wp();
	wp->old_val = val;
	strcpy(wp->expr,args);
	if(head == NULL)
	{
	wp->NO = 1;
	head = wp;}
	else
	{
	WP *newwp;
	newwp = head;
	while (newwp->next!=NULL)
	{
	newwp = newwp->next;	
	}
	wp->NO = newwp->NO+1;
	newwp->next = wp;
	}
	printf("Set watchpoint #%d\n",wp->NO);
	printf("expr     =%s\n",wp->expr);
	printf("old value =0x%x\n",wp->old_val);
	return wp->NO;		
}

bool delete_watchpoint(int num)
{
	if(head==NULL){
		printf("No watchpoint!\n");
	return false;
	}	

	WP *wp;
	if(head->NO == num){
		wp = head;
		head = head->next;
		free_wp(wp);
		printf("Watchpoint %d deleted!\n",num);
	}
	else
	{
		wp=head;
		while(wp->next!=NULL&&wp->next->NO!=num)
		{
			wp=wp->next;
		}
		if(wp==NULL)
		{
			printf("Failed to find the watchpoint %d!\n",num);
		}
		else
		{
			WP *deletedwp;
			deletedwp=wp->next;
			wp->next=deletedwp->next;
			printf("Watchpoint %d deleted!\n",num);
			free_wp(deletedwp);
		}
	}
	return true;
}

void list_watchpoint()
{
	if(head == NULL)
	{
		printf("There is no watchpoint!\n");
		return ;
	}
	WP *wp;
	wp = head;
	printf("NO        Expr    Old Value \n");
	while (wp!=NULL)
	{
		printf("%2d  %10s     0x%08x\n",wp->NO,wp->expr,wp->old_val);
		wp=wp->next;
	}
}

WP *scan_watchpoint()
{
	WP *wp=head;
	if(head == NULL)
	{
	//	printf("There is no watchpoint!\n");
	}
	while(wp!=NULL)
	{	
	bool success = true;
	int val = expr(wp->expr,&success);
	if(val !=wp->old_val)
	{
		printf("EXPR : %s value has changed!\n",wp->expr);
	printf("The old value :0x%08x\t The new value:0x%08x\t\n",wp->old_val,val);
		wp->new_val = val;
		return wp;
	}
	wp=wp->next;
	}
	return NULL;
}


