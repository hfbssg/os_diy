#include <stc8h.h>
#include <intrins.h>
#define MAX_TASKS 2 //�򻯷���,���ǵ�ǰ����ϵͳֻ��2��task
#define MAX_TASK_DEP 32

unsigned char idata task_sp[MAX_TASKS];  // ����Ķ�ջָ��
unsigned char idata task_stack[MAX_TASKS][MAX_TASK_DEP];// ÿ��һ��task����Ķ�ջ
unsigned char task_id; //��ǰ����ţ� ��0��ʼ��

//����״̬
typedef enum{
	TASK_RUNNING, //����״̬
	TASK_SUSPENDED  //����״̬
} TaskStatus;


typedef struct{
	  unsigned char id; //����id
	  TaskStatus status; //����״̬
	  unsigned int delay_count; // �ӳټ�����
	  unsigned int delay_duration;//�ӳ�ʱ��
}Task ;

Task idata tasks[MAX_TASKS] = {
	{0, TASK_RUNNING,0,0},
	{1, TASK_RUNNING,0,0},   //�������񣬶���Ĭ������״̬������ʱ��
};

void Timer0_init(void); //ԭ�ͺ��� 
void sleep(unsigned int , unsigned int );// tid, delay_ms

void sleep(unsigned int task_id , unsigned int delay_ms){
   	tasks[task_id].status = TASK_SUSPENDED;
	  tasks[task_id].delay_count = 0;
	  tasks[task_id].delay_duration = delay_ms;
	
}

void Timer0_init(){
   AUXR |= 0x80;		//��ʱ��ʱ��1Tģʽ
		TMOD &= 0xF0;		//���ö�ʱ��ģʽ
    EA = 1; // ȫ���ж�����
    ET0 = 1; // ��ʱ��0�ж�����
	  TR0 = 1;		//��ʱ��0��ʼ��ʱ
	TL0 = 0x40;		//���ö�ʱ��ʼֵ
		TH0 = 0xA2;		//���ö�ʱ��ʼֵ
	
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


//����һ�������л��ĺ���(���������)
void task_switch(){
	
	 task_sp[task_id] = SP;// �ѵ�ǰϵͳ�Ķ�ջָ����뵽ĳ��С���ѵ�task_sp���档
	
	 task_id = task_id + 1; //�����1
	 if(task_id == MAX_TASKS){	 
		 task_id = 0;
	 } 
	 SP = task_sp[task_id];
}



void task0(){
	//��0������ �����0��С�����������顣
	//static unsigned int a = 3;
	P5M0 = 0x00;
  P5M1 = 0x00;
	
  P53 = 1;
	while(1){
		//a = a + 3;
		//Delay1000ms();
		//����Լ���״̬������Լ���˯��״̬����Ӧ�ý������taskȥִ��
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
	//��1������ �����1��С�����������顣
	//static unsigned int b = 5;
	P4M1 = 0x00;
	P4M0 = 0x00;
	P2M1 = 0x00;
	P2M0 = 0x00;
	P27 = 0;
	
	
	while(1){
				//����Լ���״̬������Լ���˯��״̬����Ӧ�ý������taskȥִ��
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
	//ϵͳ�Ķ�ʱ���жϣ� ÿ��1�����ִ��һ���жϺ���
	 unsigned char i;
	  
	 for(i =0 ;i<MAX_TASKS;i++){
		  if(tasks[i].status == TASK_SUSPENDED){
				tasks[i].delay_count++;
			}
		  
			if(tasks[i].delay_count >= tasks[i].delay_duration){
				tasks[i].status = TASK_RUNNING; //˯�߽���		 
				tasks[i].delay_count = 0;
			}
		 
	 }
	   
}

void Timer1_Init(void)		//100΢��@24.000MHz
{
	AUXR |= 0x40;			//��ʱ��ʱ��1Tģʽ
	TMOD &= 0x0F;			//���ö�ʱ��ģʽ
	TL1 = 0xA0;				//���ö�ʱ��ʼֵ
	TH1 = 0xF6;				//���ö�ʱ��ʼֵ
	TF1 = 0;				//���TF1��־
	TR1 = 1;				//��ʱ��1��ʼ��ʱ
}

void Timer1_ISR(void) interrupt 3{
	task_switch();// ��timer1�е��жϣ����������л���
}



//�׶�԰��ʦ������ϵͳ����������ĺ�����
//fn fn��һ��������ָ�룬ע������������int 16λ�ġ�
//tid task id�� ��8λ�ģ� 0,1 
//���溯�������þ��ǰ�һ��task�ĺ���ָ������Ӧ�Ķ�ջ�ռ����档
void task_load(unsigned int fn, unsigned char tid){
	task_sp[tid] =  task_stack[tid] + 1; // �������ָ������һ���ռ�Ųһ������char��
	task_stack[tid][0] = fn& 0xff;
	task_stack[tid][1] = fn>>8;	
}


void main(){
	   Timer0_init();
		 Timer1_Init();
	   task_load(task0,0);// ��task0 װ�ص��ڴ��С�
	   task_load(task1,1);// ��task1 װ�ص��ڴ��С�
	   task_id = 0;   
	   SP = task_sp[0];
}



