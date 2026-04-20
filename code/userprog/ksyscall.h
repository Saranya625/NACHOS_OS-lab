/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"
#include "synchconsole.h"
#include "ksyscallhelper.h"
#include <stdlib.h>
#define INT32_MIN 0
void SysHalt() { kernel->interrupt->Halt(); }

int SysAdd(int op1, int op2) { return op1 + op2; }

int SysAbs(int op1) {if(op1>0) return op1; else return -op1;}

int SysRead(char* buffer, int charCount, int fileId);
int SysWrite(char* buffer, int charCount, int fileId);

char ReadStdInChar() {
    char c = 0;
    int bytes = SysRead(&c, 1, 0);
    if (bytes <= 0) return EOF;
    return c;
}

void WriteStdOutChar(char c) { SysWrite(&c, 1, 1); }

int SysReadNum() {
    memset(_numberBuffer, 0, sizeof(_numberBuffer));
    char c = ReadStdInChar();

    if (c == EOF) {
        DEBUG(dbgSys, "Unexpected end of file - number expected");
        return 0;
    }

    if (isBlank(c)) {
        DEBUG(dbgSys, "Unexpected white-space - number expected");
        return 0;
    }

    int n = 0;
    while (!(isBlank(c) || c == EOF)) {
        _numberBuffer[n++] = c;
        if (n > MAX_NUM_LENGTH) {
            DEBUG(dbgSys, "Number is too long");
            return 0;
        }
        c = ReadStdInChar();
    }

    int len = strlen(_numberBuffer);
    // Read nothing -> return 0
    if (len == 0) return 0;

    // Check comment below to understand this line of code
    if (strcmp(_numberBuffer, "-2147483648") == 0) return INT32_MIN;

    bool nega = (_numberBuffer[0] == '-');
    int zeros = 0;
    bool is_leading = true;
    int num = 0;
    for (int i = nega; i < len; ++i) {
        char c = _numberBuffer[i];
        if (c == '0' && is_leading)
            ++zeros;
        else
            is_leading = false;
        if (c < '0' || c > '9') {
            DEBUG(dbgSys, "Expected number but " << _numberBuffer << " found");
            return 0;
        }
        num = num * 10 + (c - '0');
    }

    // 00            01 or -0
    if (zeros > 1 || (zeros && (num || nega))) {
        DEBUG(dbgSys, "Expected number but " << _numberBuffer << " found");
        return 0;
    }

    if (nega)
        /**
         * This is why we need to handle -2147483648 individually:
         * 2147483648 is larger than the range of int32
         */
        num = -num;

    // It's safe to return directly if the number is small
    if (len <= MAX_NUM_LENGTH - 2) return num;

    /**
     * We need to make sure that number is equal to the number in the buffer.
     *
     * Ask: Why do we need that?
     * Answer: Because it's impossible to tell whether the number is bigger
     * than INT32_MAX or smaller than INT32_MIN if it has the same length.
     *
     * For example: 3 000 000 000.
     *
     * In that case, that number will cause an overflow. However, C++
     * doens't raise interger overflow, so we need to make sure that the input
     * string and the output number is equal.
     *
     */
    if (compareNumAndString(num, _numberBuffer))
        return num;
    else
        DEBUG(dbgSys,
              "Expected int32 number but " << _numberBuffer << " found");

    return 0;
}

void SysPrintNum(int num) {
    if (num == 0) return WriteStdOutChar('0');

    if (num == INT32_MIN) {
        WriteStdOutChar('-');
        for (int i = 0; i < 10; ++i) WriteStdOutChar("2147483648"[i]);
        return;
    }

    if (num < 0) {
        WriteStdOutChar('-');
        num = -num;
    }
    int n = 0;
    while (num) {
        _numberBuffer[n++] = num % 10;
        num /= 10;
    }
    for (int i = n - 1; i >= 0; --i) WriteStdOutChar(_numberBuffer[i] + '0');
}

char SysReadChar() { return ReadStdInChar(); }

void SysPrintChar(char character) { WriteStdOutChar(character); }

int SysRandomNum() { return random(); }

char* SysReadString(int length) {
    char* buffer = new char[length + 1];
    for (int i = 0; i < length; i++) {
        buffer[i] = SysReadChar();
    }
    buffer[length] = '\0';
    return buffer;
}

void SysPrintString(char* buffer, int length) {
    for (int i = 0; i < length; i++) {
        WriteStdOutChar(buffer[i]);
    }
}

bool SysCreateFile(char* fileName) {
    bool success;
    int fileNameLength = strlen(fileName);

    if (fileNameLength == 0) {
        DEBUG(dbgSys, "\nFile name can't be empty");
        success = false;

    } else if (fileName == NULL) {
        DEBUG(dbgSys, "\nNot enough memory in system");
        success = false;

    } else {
        DEBUG(dbgSys, "\nFile's name read successfully");
        if (!kernel->fileSystem->Create(fileName)) {
            DEBUG(dbgSys, "\nError creating file");
            success = false;
        } else {
            success = true;
        }
    }

    return success;
}

int SysOpen(char* fileName, int type) {
    if (type != 0 && type != 1) return -1;

    int id = kernel->fileSystem->Open(fileName, type);
    if (id == -1) return -1;
    DEBUG(dbgSys, "\nOpened file");
    return id;
}

int SysClose(int id) { return kernel->fileSystem->Close(id); }

int SysRead(char* buffer, int charCount, int fileId) {
    if (fileId == 0) {
        int redirected = kernel->fileSystem->ReadStdIn(buffer, charCount);
        if (redirected >= 0) return redirected;
        return kernel->synchConsoleIn->GetString(buffer, charCount);
    }
    return kernel->fileSystem->Read(buffer, charCount, fileId);
}

int SysWrite(char* buffer, int charCount, int fileId) {
    if (fileId == 1) {
        int redirected = kernel->fileSystem->WriteStdOut(buffer, charCount);
        if (redirected >= 0) return redirected;
        return kernel->synchConsoleOut->PutString(buffer, charCount);
    }
    return kernel->fileSystem->Write(buffer, charCount, fileId);
}

int SysSeek(int seekPos, int fileId) {
    if (fileId <= 1) {
        DEBUG(dbgSys, "\nCan't seek in console");
        return -1;
    }
    return kernel->fileSystem->Seek(seekPos, fileId);
}

int SysExec(char* name) {
    // cerr << "call: `" << name  << "`"<< endl;
    OpenFile* oFile = kernel->fileSystem->Open(name);
    if (oFile == NULL) {
        DEBUG(dbgSys, "\nExec:: Can't open this file.");
        return -1;
    }

    delete oFile;

    // Return child process id
    return kernel->pTab->ExecUpdate(name);
}

int SysJoin(int id) { return kernel->pTab->JoinUpdate(id); }

int SysExit(int id) { return kernel->pTab->ExitUpdate(id); }

int SysCreateSemaphore(char* name, int initialValue) {
    int res = kernel->semTab->Create(name, initialValue);

    if (res == -1) {
        DEBUG('a', "\nError creating semaphore");
        delete[] name;
        return -1;
    }

    return 0;
}

int SysWait(char* name) {
    int res = kernel->semTab->Wait(name);

    if (res == -1) {
        DEBUG('a', "\nSemaphore not found");
        delete[] name;
        return -1;
    }

    return 0;
}

int SysSignal(char* name) {
    int res = kernel->semTab->Signal(name);

    if (res == -1) {
        DEBUG('a', "\nSemaphore not found");
        delete[] name;
        return -1;
    }

    return 0;
}

int SysGetPid() { return kernel->currentThread->processID; }

int SysSetStdIn(char* fileName) { return kernel->fileSystem->SetStdIn(fileName); }

int SysSetStdOut(char* fileName) {
    return kernel->fileSystem->SetStdOut(fileName);
}

int SysMalloc(int size) {
    if (kernel->currentThread->space == NULL) return 0;
    return kernel->currentThread->space->Malloc(size);
}

#endif /* ! __USERPROG_KSYSCALL_H__ */
