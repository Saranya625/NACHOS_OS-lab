#include "syscall.h"

int main() {
    int x = ReadNum();
    PrintNum(Abs(x));
    return 0;
}
