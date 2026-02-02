#include "thread.h"
#include "scheduler.h"
#include "thread_test.h"
#include "main.h"
#include <stdint.h>
void PriorityThread(void* arg) {
	int p = (int)(intptr_t)arg;
    for (int i = 0; i < 5; i++) {
        printf("Thread priority %d running\n", p);
        kernel->currentThread->Yield();
    }
}

void ThreadTestNew() {
    Thread *low = new Thread("low");
    Thread *mid = new Thread("mid");
    Thread *high = new Thread("high");
	Thread *nextH = new Thread("nextH");

    low->SetPriority(1);
    mid->SetPriority(5);
	high->SetPriority(10);
	nextH->SetPriority(100);
	printf("Here");

    low->Fork(PriorityThread, (void*)(intptr_t)1);
    mid->Fork(PriorityThread, (void *)(intptr_t)5);
    high->Fork(PriorityThread, (void *)(intptr_t)10);
	nextH->Fork(PriorityThread, (void*)(intptr_t)100);
}
