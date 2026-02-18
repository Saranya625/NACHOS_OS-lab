#include "syscall.h"

#define stdout 1

int main() {
    int x, y;
    x = -123;
    PrintNum(x);
    y = Abs(x);
    PrintNum(y);
    Halt();
}

