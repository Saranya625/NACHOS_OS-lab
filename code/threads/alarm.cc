#include "copyright.h"
#include "alarm.h"
#include "main.h"

Alarm::Alarm(bool doRandom) {
    timer = new Timer(doRandom, this);
    sleepList = new List<SleepThread *>();
}

void Alarm::WaitUntil(int x) {
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);

    int wakeTime = kernel->stats->totalTicks + x;
    sleepList->Append(new SleepThread(kernel->currentThread, wakeTime));
    kernel->currentThread->Sleep(false);

    kernel->interrupt->SetLevel(oldLevel);
}

void Alarm::CallBack() {
    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();

    ListIterator<SleepThread *> iter(sleepList);
    while (!iter.IsDone()) {
        SleepThread *st = iter.Item();
        iter.Next();
        if (kernel->stats->totalTicks >= st->wakeTime) {
            sleepList->Remove(st);
            kernel->scheduler->ReadyToRun(st->thread);
            delete st;
        }
    }

    if (status != IdleMode) {
        interrupt->YieldOnReturn();
    }
}
