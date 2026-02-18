#include "syscall.h"

int main() {
    PrintString("Before sleep\n");
    Sleep(500000000);
    PrintString("After sleep\n");
    Halt();
}
