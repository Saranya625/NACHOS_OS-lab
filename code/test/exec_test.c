#include "syscall.h"

#define stdin  0
#define stdout 1

int main() {
    int p1, p2, p3;

    Write("Starting scheduling test...\n", 28, stdout);

    p1 = Exec("num_io");
    p2 = Exec("num_io");
    p3 = Exec("num_io");

    if (p1 < 0 || p2 < 0 || p3 < 0) {
        Write("Exec failed\n", 12, stdout);
        Halt();
    }

    Join(p1);
    Join(p2);
    Join(p3);

    Write("All child processes finished.\n", 31, stdout);
    Halt();
}

