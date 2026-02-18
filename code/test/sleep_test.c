#include "syscall.h"

int main() {
    PrintString("Before sleep\n");
    Sleep(500);
    PrintString("After sleep\n");
    Halt();
}
