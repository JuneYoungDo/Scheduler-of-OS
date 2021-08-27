#include<stdio.h>
#include<pthread.h>
#include<semaphore.h>
#define true 1
#define false 0
#define MAX 200	
#define timeQuantum 5

// 자료구조 queue 구현 용
typedef struct pair
{
	int id;
	int remainNum;
}pair;
int front = -1;
int rear = -1;
pair queue[MAX];

int IsEmpty();
int IsFull();
void enqueue(int id, int remainTime);
pair dequeue();

//ㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡㅡ

int SJF_last_Count = 0;
int RR_last_Count = 0;
int Priority_last_Count = 0;
int WTR_last_Count = 0;

// queue안의 스케줄링
void* SJF(void* n);
void* RR(void* n);
void* Priority(void* n);
void* WTR(void* n);

// queue간의 스케줄링 우선순위 입력
void* Input(void* n);
int InputNum[4];

// queue간의 스케줄링
void* QueuePriority(void* n);

// 데이터 형식( class number / processID / 우선순위 / BurstTime )
typedef struct data
{
	int ClassNumber;
	int ProcessID;
	int priority;
	int burstTime;
}data;

data List[MAX];
int cnt = 0;

// Shortest Job First
data SJFlist[MAX];
int SJFcount = 0;

// RoundRobin
data RRlist[MAX];
int RRcount = 0;

// Priority
data Prioritylist[MAX];
int Prioritycount = 0;

// Waiting Time Ratio
typedef struct ratio
{
	int id;
	float ratio;
	int replayNum;
}ratio;
data WTRlist[MAX];
int WTRcount = 0;
ratio ratioArr[MAX];

// semaphore
sem_t semaphore_one;
sem_t semaphore_two;
sem_t semaphore_three;
sem_t semaphore_four;
sem_t semaphore_five;

FILE* fp;

int main(int argc, char *argv[])
{
	// 전체 데이터 갯 수
	int DataNum;
	fp = fopen(argv[1], "r");
//	scanf_s("%d", &DataNum);
	fscanf(fp, "%d", &DataNum);

	for (int i = 0; i < DataNum; i++)
	{
		int num;
//		scanf_s("%d", &num);
//		scanf_s("%d %d %d", &List[i].ProcessID, &List[i].priority, &List[i].burstTime);

		fscanf(fp, "%d", &num);
		fscanf(fp, "%d %d %d", &List[i].ProcessID, &List[i].priority, &List[i].burstTime);

		// 각각 해당 queue로 분류
		if (num == 1)
		{
			List[i].ClassNumber = num;
			SJFlist[SJFcount] = List[i];
			SJFcount++;
		}
		else if (num == 2)
		{
			List[i].ClassNumber = num;
			RRlist[RRcount] = List[i];
			RRcount++;
		}
		else if (num == 3)
		{
			List[i].ClassNumber = num;
			Prioritylist[Prioritycount] = List[i];
			Prioritycount++;
		}
		else if (num == 4)
		{
			List[i].ClassNumber = num;
			WTRlist[WTRcount] = List[i];
			WTRcount++;
		}
	}

	// semaphore 초기화
	sem_init(&semaphore_one, 0, 0);
	sem_init(&semaphore_two, 0, 0);
	sem_init(&semaphore_three, 0, 0);
	sem_init(&semaphore_four, 0, 0);
	sem_init(&semaphore_five, 0, 0);

	// thread 생성
	pthread_t scheduling[6];
	pthread_create(&scheduling[0], NULL, SJF, (data*)&SJFlist);
	pthread_create(&scheduling[1], NULL, RR, (data*)&RRlist);
	pthread_create(&scheduling[2], NULL, Priority, (data*)&Prioritylist);
	pthread_create(&scheduling[3], NULL, WTR, (data*)&WTRlist);
	pthread_create(&scheduling[4], NULL, Input, (void*)&InputNum);		// 우선순위 입력
	pthread_create(&scheduling[5], NULL, QueuePriority, (void*)&InputNum);		// 우선순위 기반 q간의 스케줄링

	pthread_join(scheduling[0], NULL);
	pthread_join(scheduling[1], NULL);
	pthread_join(scheduling[2], NULL);
	pthread_join(scheduling[3], NULL);
	pthread_join(scheduling[4], NULL);
	pthread_join(scheduling[5], NULL);

	sem_destroy(&semaphore_one);
	sem_destroy(&semaphore_two);
	sem_destroy(&semaphore_three);
	sem_destroy(&semaphore_four);
	sem_destroy(&semaphore_five);

	return 0;
}

// SJF용 정렬을 위한 구현
void SJFsorting(int count, data* arr)
{
	data* a = arr;
	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < count - 1 - i; j++)
		{
			if (a[j].burstTime > a[j + 1].burstTime)
			{
				data temp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = temp;
			}
		}
	}
}

// Priority용 정렬을 위한 구현
void PrioritySorting(int count, data* arr)
{
	data* a = arr;
	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < count - 1 - i; j++)
		{
			if (a[j].priority > a[j + 1].priority)
			{
				data temp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = temp;
			}
		}
	}
}

// WTR용 정렬을 위한 구현
void WTRsorting(int count, ratio* arr)
{
	ratio* a = arr;
	for (int i = 0; i < count; i++)
	{
		for (int j = 0; j < count - 1 - i; j++)
		{
			if (a[j].ratio < a[j + 1].ratio)
			{
				ratio temp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = temp;
			}
		}
	}
}


// queue안의 스케줄링----------------------------
void* SJF(void* n)
{
	sem_wait(&semaphore_one);
	data* a = n;
	SJFsorting(SJFcount, a);
	for (int i = 0; i < SJFcount; i++)
	{
		for (int j = 0; j < a[i].burstTime; j++)
			printf("%d ", a[i].ProcessID);
		printf("\n");
	}
	sem_post(&semaphore_five);
	pthread_exit(NULL);
}

void* RR(void* n)
{
	sem_wait(&semaphore_two);
	data* a = n;
	for (int i = 0; i < RRcount; i++)
	{
		if (a[i].burstTime < timeQuantum)
		{
			for (int j = 0; j < a[i].burstTime; j++)
				printf("%d ", a[i].ProcessID);
			printf("\n");
		}
		else
		{
			enqueue(a[i].ProcessID, a[i].burstTime - timeQuantum);
			for (int j = 0; j < timeQuantum; j++)
				printf("%d ", a[i].ProcessID);
			printf("\n");
		}
	}
	while (!IsEmpty())
	{
		pair a = dequeue();
		int id = a.id;
		int remain = a.remainNum;
		if (remain >= timeQuantum)
		{
			enqueue(id, remain - timeQuantum);
			for (int i = 0; i < timeQuantum; i++)
				printf("%d ", id);
			printf("\n");
		}
		else
		{
			for (int i = 0; i < remain; i++)
				printf("%d ", id);
			printf("\n");
		}
	}
	sem_post(&semaphore_five);
	pthread_exit(NULL);
}

void* Priority(void* n)
{
	sem_wait(&semaphore_three);
	data* a = n;
	PrioritySorting(Prioritycount, a);
	for (int i = 0; i < Prioritycount; i++)
	{
		for (int j = 0; j < a[i].burstTime; j++)
			printf("%d ", a[i].ProcessID);
		printf("\n");
	}
	sem_post(&semaphore_five);
	pthread_exit(NULL);
}

void* WTR(void* n)
{
	sem_wait(&semaphore_four);
	data* a = n;
	ratioArr[0].id = a[0].ProcessID;
	ratioArr[0].ratio = MAX;
	ratioArr[0].replayNum = a[0].burstTime;
	for (int i = 1; i < WTRcount; i++)
	{
		int waittime = 0;
		for (int j = 0; j < i; j++)
		{
			waittime += a[j].burstTime;
		}
		ratioArr[i].id = a[i].ProcessID;
		ratioArr[i].replayNum = a[i].burstTime;
		ratioArr[i].ratio = (waittime + a[i].burstTime) / a[i].burstTime;
	}
	WTRsorting(WTRcount, ratioArr);

	for (int i = 0; i < WTRcount; i++)
	{
		for (int j = 0; j < ratioArr[i].replayNum; j++)
			printf("%d ", ratioArr[i].id);
		printf("\n");
	}
	sem_post(&semaphore_five);
	pthread_exit(NULL);
}

//-----------------------------------------------

// queue간의 스케줄링을 위한 우선순위 입력
void* Input(void* n)
{
	int* a = n;
	printf("Queue 간의 우선순위를 입력하세요. ( 1.SJF 2.RR 3.Priority 4.WTR )\n");
	printf("입력한 순서대로 우선순위를 부여합니다.\n");
	for (int i = 0; i < 3; i++)
	{
//		scanf_s("%d", &a[i]);
		fscanf(fp, "%d", &a[i]);
	}
//	scanf_s("%d", &a[3]);
	fscanf(fp, "%d", &a[3]);
	printf("\n");
	sem_post(&semaphore_five);
	pthread_exit(NULL);
}

// queue간의 스케줄링
void* QueuePriority(void* n)
{
	int* a = n;
	for (int i = 0; i < 4; i++)
	{
		sem_wait(&semaphore_five);
		if (a[i] == 1)
			sem_post(&semaphore_one);
		if (a[i] == 2)
			sem_post(&semaphore_two);
		if (a[i] == 3)
			sem_post(&semaphore_three);
		if (a[i] == 4)
			sem_post(&semaphore_four);
	}
	pthread_exit(NULL);
}

// 자료구조 queue 구현용
int IsEmpty()
{
	if (front == rear)return true;
	else return false;
}
int IsFull()
{
	int tmp = (rear + 1) % MAX;
	if (tmp == front)return true;
	else return false;
}
void enqueue(int id, int remainTime)
{
	if (IsFull());
	else
	{
		rear = (rear + 1) % MAX;
		queue[rear].id = id;
		queue[rear].remainNum = remainTime;
	}
}
pair dequeue()
{
	if (IsEmpty());
	else
	{
		front = (front + 1) % MAX;

		return queue[front];
	}
}