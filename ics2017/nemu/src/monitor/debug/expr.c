#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.  
* Type 'man regex' for more information about POSIX regex functions.  
*/
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256, 
	EQ , NEQ , AND , OR , MINUS , POINTOR , NUMBER , HEX , REGISTER , MARK, GE, LE
	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
	int priority;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */
	{"\\b[0-9]+\\b",NUMBER,0},			// number
	{"\\b0[xX][0-9a-fA-F]+\\b",HEX,0},//hex
	{"\\$(eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|ebp|EBP|esp|ESP|esi|ESI|edi|EDI|eip|EIP)",REGISTER,0},		// register
	{"\\$(([ABCD][HLX])|([abcd][hlx]))",REGISTER,0},		// register
	{"\\b[a-zA-Z_0-9]+" , MARK, 0},		// mark
	{"!=",NEQ,3},						// not equal	
	{"!",'!',6},						// not
	{"\\*",'*',5},						// mul
	{"/",'/',5},						// div
	{"\\t+",NOTYPE,0},					// tabs
	{" +",NOTYPE,0},					// spaces
	{"\\+",'+',4},						// plus
	{"-",'-',4},						// sub
	{"==", EQ,3},						// equal
	{"&&",AND,2},						// and
	{">", '>', 3},      				// greater
	{"<", '<', 3}, 						// lower
	{">=", GE, 3},						// greater or equal
	{"<=", LE, 3},						// lower or equal
	{"\\|\\|",OR,1},					// or
	{"\\(",'(',7},                      // left bracket   
	{"\\)",')',7},                      // right bracket 
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for (i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if (ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

/* str成员就是用来做这件事情的
 * 需要注意的是, str成员的长度是有限的, 当你发现缓冲区将要溢出的时候, 要进行相应的处理
 */
typedef struct token {
	int type;
	char str[32];
	int priority;
} Token;

#define MAX_TOEKN_NUM 1000
Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch; //存放匹配开始位置和结束位置
	nr_token = 0;
	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) { //必须从开始完成匹配
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				char *tmp = e + position + 1;
				int substr_len = pmatch.rm_eo; 
					position += substr_len;
				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
						i, rules[i].regex, position, substr_len, substr_len, substr_start);// eo位置是不匹配字符
				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array ``tokens''. For certain 
				 * types of tokens, some extra actions should be performed.
				 */
				switch(rules[i].token_type) {
					case NOTYPE: break;
					case REGISTER: //寄存器把那个特殊的头部去掉
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].priority = rules[i].priority; 
						strncpy (tokens[nr_token].str,tmp,substr_len-1);
						tokens[nr_token].str[substr_len-1]='\0';
						nr_token ++;
						break; 
					default:
						tokens[nr_token].type = rules[i].token_type;
						tokens[nr_token].priority = rules[i].priority;
						strncpy (tokens[nr_token].str,substr_start,substr_len);
						tokens[nr_token].str[substr_len]='\0';
						nr_token ++;
						break;
				}
			
				break;
 			}
 		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
 	}
	return true; 
}

bool check_parentheses (int p,int q)
{
	int i;
	if (tokens[p].type == '(' && tokens[q].type ==')')
	{
		int lc = 0, rc = 0;
		for (i = p + 1; i < q; i ++)
		{
			if (tokens[i].type == '(')lc ++;
			if (tokens[i].type == ')')rc ++;
			if (rc > lc) return false;	//右括号数目不能超过左边括号数目
		}
		if (lc == rc) return true;
	}
	return false;
}

/*
 *寻找 token[l] 到 token[r] 间的主运算符
 *出现在一对括号中的token不是主运算符. 注意到这里不会出现有括号包围整个表达式的情况, 因为这种情况已经在check_parentheses()相应的if块中被处理了.
 *主运算符的优先级在表达式中是最低的. 这是因为主运算符是最后一步才进行的运算符.
 *当有多个运算符的优先级都是最低时, 根据结合性, 最后被结合的运算符才是主运算符. 一个例子是1 + 2 + 3, 它的主运算符应该是右边的+.
 */

int find_dominanted_op (int p,int q)
{
	int i;
	int min_priority = 10;
	int oper = p;
	int cnt = 0;
	for (i = p; i <= q;i ++)
	{
		if (tokens[i].type == NUMBER || tokens[i].type == HEX || tokens[i].type == REGISTER || tokens[i].type == MARK)
			continue;
		
		if (tokens[i].type == '(') cnt++;
		if (tokens[i].type == ')') cnt--;
		if (cnt != 0) continue; //左边还有括号,还是处于括号之中
		
		if (tokens[i].priority <= min_priority) {
			min_priority=tokens[i].priority; oper=i;
		}
 	}
	return oper;
}

uint32_t eval(int p,int q) {
	if (p > q) {assert(0);}
	if (p == q) 
	{
		uint32_t num = 0;
		if (tokens[p].type == NUMBER)
			sscanf(tokens[p].str,"%d",&num);
		else if (tokens[p].type == HEX)
			sscanf(tokens[p].str,"%x",&num);
		else if (tokens[p].type == REGISTER)
		{
			if (strlen (tokens[p].str) == 3) {
				int i;
				for (i = R_EAX; i <= R_EDI; i ++)
					if (strcmp (tokens[p].str,regsl[p]) == 0) break;
				if (i > R_EDI)
					if (strcmp (tokens[p].str,"eip") == 0)
						num = cpu.eip;
					else Assert (1,"no this register!\n");
				else num = reg_l(i);
			}
			else if (strlen (tokens[p].str) == 2) {
				if (tokens[p].str[1] == 'x' || tokens[p].str[1] == 'p' || tokens[p].str[1] == 'i') {
					int i;
					for (i = R_AX; i <= R_DI; i ++)
						if (strcmp (tokens[p].str,regsw[i]) == 0)break;
					num = reg_w(i);
				}
				else if (tokens[p].str[1] == 'l' || tokens[p].str[1] == 'h') {
					int i;
					for (i = R_AL; i <= R_BH; i ++)
						if (strcmp (tokens[p].str,regsb[i]) == 0)break;
					num = reg_b(i);
				}
				else assert (1);
			}
		}
		else
		{
			return -1;
		}
		return num;
	}
	else if (check_parentheses (p,q) == true) return eval(p+ 1, q - 1);
 	else {
		int op = find_dominanted_op (p,q);
 		if (p == op || tokens[op].type == POINTOR || tokens[op].type == MINUS || tokens[op].type == '!')
		{
			uint32_t val = eval (p + 1, q);
			switch (tokens[p].type)
 			{
				case POINTOR:  return vaddr_read(val, 4);
				case MINUS:return -val;
				case '!':return !val;
				default: 
					return -1;
			} 
		}
		uint32_t val1 = eval (p, op - 1);
		uint32_t val2 = eval (op + 1, q);
		switch (tokens[op].type)
		{
			case '+':return val1 + val2;
			case '-':return val1 - val2;
			case '*':return val1 * val2;
			case '/':return val1 / val2;
			case EQ:return val1 == val2;
			case NEQ:return val1 != val2;
			case '>':return val1 > val2;
			case '<':return val1 < val2;
			case GE:return val1 >= val2;
			case LE:return val1 <= val2;
			case AND:return val1 && val2;
			case OR:return val1 || val2;
			default:
			
				return -1;
  		}
  	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
  	}
	int i;
	for (i = 0;i < nr_token; i ++) { //识别负数和指针

 		if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != NUMBER && tokens[i - 1].type != HEX && tokens[i - 1].type != REGISTER && tokens[i - 1].type != MARK && tokens[i - 1].type !=')'))) {
			tokens[i].type = POINTOR;
			tokens[i].priority = 6;
		}
		if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != NUMBER && tokens[i - 1].type != HEX && tokens[i - 1].type != REGISTER && tokens[i - 1].type != MARK && tokens[i - 1].type !=')'))) {
			tokens[i].type = MINUS;
			tokens[i].priority = 6;
 		}
  	}
	/* TODO: Insert codes to evaluate the expression. */	
	//printf("success = %d\n", *success);
	return eval(0, nr_token-1);
}

