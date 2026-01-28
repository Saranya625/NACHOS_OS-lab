/* add.c
 *	Simple program to test whether the systemcall interface works.
 *
 *	Just do a add syscall that adds two values and returns the result.
 *
 */

#include "syscall.h"

int main() {
    int result;
    int i,j;
    for(i=0;i<1000;i++){
	    for(j=0;j<20000;j++){
		    PrintString("In Child\n");
	    }
    }

    Halt();
    /* not reached */
}
