#include "syscall.h"

#define stdout 1

int main() {
    int x, y;
    x = -123;
    y = Abs(x);
    Halt();
}

