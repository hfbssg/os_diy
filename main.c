#include <stc8h.h>
#include <intrins.h>
#define MAX_TASKS 2 //简化方面,我们当前操作系统只有2个task
#define MAX_TASK_DEP 32

unsigned char idata task_sp[MAX_TASKS];  // 任务的堆栈指针
unsigned char idata task_stack[MAX_TASKS][MAX_TASK_DEP];// 每个一个task任务的堆栈
unsigned char task_id; //当前任务号， 从0开始。

//任务状态
typedef enum{
	TASK_RUNNING, //运行状态
	TASK_SUSPENDED  //挂起状态
} TaskStatus;


typedef struct{
	  unsigned char id; //任务id
	  TaskStatus status; //任务状态
	  unsigned int delay_count; // 延迟计数器
	  unsigned int delay_duration;//延迟时间
}Task ;

Task idata tasks[MAX_TASKS] = {
	{0, TASK_RUNNING,0,0},
	{1, TASK_RUNNING,0,0},   //两个任务，都是默认运行状态，不延时，
};

void Timer0_init(void); //原型函数 
void sleep(unsigned int , unsigned int );// tid, delay_ms

void sleep(unsigned int task_id , unsigned int delay_ms){
   	tasks[task_id].status = TASK_SUSPENDED;
	  tasks[task_id].delay_count = 0;
	  tasks[task_id].delay_duration = delay_ms;
	
}

void Timer0_init(){
   AUXR |= 0x80;		//定时器时钟1T模式
		TMOD &= 0xF0;		//设置定时器模式
    EA = 1; // 全局中断允许
    ET0 = 1; // 定时器0中断允许
	  TR0 = 1;		//定时器0开始计时
	TL0 = 0x40;		//设置定时初始值
		TH0 = 0xA2;		//设置定时初始值
	
}


void Delay1000ms()		//@24.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 122;
	j = 193;
	k = 128;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


//定义一个任务切换的函数(任务调度器)
void task_switch(){
	
	 task_sp[task_id] = SP;// 把当前系统的堆栈指针存入到某个小朋友的task_sp里面。
	
	 task_id = task_id + 1; //任务加1
	 if(task_id == MAX_TASKS){	 
		 task_id = 0;
	 } 
	 SP = task_sp[task_id];
}



void task0(){
	//第0号任务， 代表第0个小朋友做的事情。
	//static unsigned int a = 3;
	P5M0 = 0x00;
  P5M1 = 0x00;
	
  P53 = 1;
	while(1){
		//a = a + 3;
		//Delay1000ms();
		//检查自己的状态。如果自己是睡眠状态，就应该交给别的task去执行
		if(tasks[0].status == TASK_SUSPENDED){
			task_switch();
			continue;
		}
		
		sleep(0,1000);
		P53 = ~P53;
		task_switch();
	}
}

void task1(){
	//第1号任务， 代表第1个小朋友做的事情。
	//static unsigned int b = 5;
	P4M1 = 0x00;
	P4M0 = 0x00;
	P2M1 = 0x00;
	P2M0 = 0x00;
	P27 = 0;
	
	
	while(1){
				//检查自己的状态。如果自己是睡眠状态，就应该交给别的task去执行
		if(tasks[1].status == TASK_SUSPENDED){
			task_switch();
			continue;
		}
		//b = b + 5;
		//Delay1000ms();
		sleep(1,1000);
    P27 = ~P27;	
		task_switch();
	}
}


void Timer0_ISR(void) interrupt 1 {
	//系统的定时器中断， 每隔1毫秒就执行一下中断函数
	 unsigned char i;
	  
	 for(i =0 ;i<MAX_TASKS;i++){
		  if(tasks[i].status == TASK_SUSPENDED){
				tasks[i].delay_count++;
			}
		  
			if(tasks[i].delay_count >= tasks[i].delay_duration){
				tasks[i].status = TASK_RUNNING; //睡眠结束		 
				tasks[i].delay_count = 0;
			}
		 
	 }
	   
}

void Timer1_Init(void)		//100微秒@24.000MHz
{
	AUXR |= 0x40;			//定时器时钟1T模式
	TMOD &= 0x0F;			//设置定时器模式
	TL1 = 0xA0;				//设置定时初始值
	TH1 = 0xF6;				//设置定时初始值
	TF1 = 0;				//清除TF1标志
	TR1 = 1;				//定时器1开始计时
}

void Timer1_ISR(void) interrupt 3{
	task_switch();// 在timer1中的中断，进行任务切换。
}



//幼儿园老师（操作系统，加载任务的函数）
//fn fn是一个函数的指针，注意数据类型是int 16位的。
//tid task id， 是8位的， 0,1 
//下面函数的作用就是把一个task的函数指针放入对应的堆栈空间里面。
void task_load(unsigned int fn, unsigned char tid){
	task_sp[tid] =  task_stack[tid] + 1; // 把任务的指针往下一个空间挪一格，两个char了
	task_stack[tid][0] = fn& 0xff;
	task_stack[tid][1] = fn>>8;	
}


void main(){
	   Timer0_init();
		 Timer1_Init();
	   task_load(task0,0);// 把task0 装载到内存中。
	   task_load(task1,1);// 把task1 装载到内存中。
	   task_id = 0;   
	   SP = task_sp[0];
}



