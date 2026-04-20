#include "syscall.h"

int main() {
    int i;
    int sum;
    int *numbers;
    char *buffer;

    PrintString("malloc test start\n");

    numbers = (int *)malloc(10 * sizeof(int));
    if (numbers == 0) {
        PrintString("malloc for int array failed\n");
        Exit(-1);
    }

    sum = 0;
    for (i = 0; i < 10; i++) {
        numbers[i] = i + 1;
        sum += numbers[i];
    }

    PrintString("sum = ");
    PrintNum(sum);
    PrintChar('\n');

    buffer = (char *)malloc(400);
    if (buffer == 0) {
        PrintString("malloc for buffer failed\n");
        Exit(-1);
    }

    for (i = 0; i < 400; i++) {
        buffer[i] = 'A' + (i % 26);
    }

    PrintString("buffer[0] = ");
    PrintChar(buffer[0]);
    PrintChar('\n');

    PrintString("buffer[129] = ");
    PrintChar(buffer[129]);
    PrintChar('\n');

    PrintString("buffer[399] = ");
    PrintChar(buffer[399]);
    PrintChar('\n');

    PrintString("malloc test end\n");
    return 0;
}
