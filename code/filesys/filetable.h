#ifndef FILETABLE_H
#define FILETABLE_H
#include "openfile.h"
#include "sysdep.h"
#include <string.h>

#define FILE_MAX 10
#define CONSOLE_IN 0
#define CONSOLE_OUT 1
#define MODE_READWRITE 0
#define MODE_READ 1
#define MODE_WRITE 2

class FileTable {
   private:
    OpenFile** openFile;
    int* fileOpenMode;
    bool stdinRedirected;
    bool stdoutRedirected;
    char stdinPath[128];
    char stdoutPath[128];

    int openByMode(char* fileName, int openMode) {
        if (openMode == MODE_READWRITE) return OpenForReadWrite(fileName, FALSE);
        if (openMode == MODE_READ) return OpenForRead(fileName, FALSE);
        if (openMode == MODE_WRITE) return OpenForWrite(fileName);
        return -1;
    }

   public:
    FileTable() {
        openFile = new OpenFile*[FILE_MAX];
        fileOpenMode = new int[FILE_MAX];
        for (int i = 0; i < FILE_MAX; i++) {
            openFile[i] = NULL;
            fileOpenMode[i] = MODE_READWRITE;
        }
        fileOpenMode[CONSOLE_IN] = MODE_READ;
        fileOpenMode[CONSOLE_OUT] = MODE_WRITE;
        stdinRedirected = false;
        stdoutRedirected = false;
        stdinPath[0] = '\0';
        stdoutPath[0] = '\0';
    }

    int Insert(char* fileName, int openMode) {
        int freeIndex = -1;
        int fileDescriptor = -1;
        for (int i = 2; i < FILE_MAX; i++) {
            if (openFile[i] == NULL) {
                freeIndex = i;
                break;
            }
        }

        if (freeIndex == -1) {
            return -1;
        }

        fileDescriptor = openByMode(fileName, openMode);

        if (fileDescriptor == -1) return -1;
        openFile[freeIndex] = new OpenFile(fileDescriptor);
        fileOpenMode[freeIndex] = openMode;

        return freeIndex;
    }

    int Remove(int index) {
        if (index < 2 || index >= FILE_MAX) return -1;
        if (openFile[index]) {
            delete openFile[index];
            openFile[index] = NULL;
            return 0;
        }
        return -1;
    }

    int Read(char* buffer, int charCount, int index) {
        if (index >= FILE_MAX) return -1;
        if (openFile[index] == NULL) return -1;
        int result = openFile[index]->Read(buffer, charCount);
        // if we cannot read enough bytes, we should return -2
        if (result != charCount) return -2;
        return result;
    }

    int Write(char* buffer, int charCount, int index) {
        if (index >= FILE_MAX) return -1;
        if (openFile[index] == NULL || fileOpenMode[index] == MODE_READ)
            return -1;
        return openFile[index]->Write(buffer, charCount);
    }

    int Seek(int pos, int index) {
        if (index <= 1 || index >= FILE_MAX) return -1;
        if (openFile[index] == NULL) return -1;
        // use seek(-1) to move to the end of file
        if (pos == -1) pos = openFile[index]->Length();
        if (pos < 0 || pos > openFile[index]->Length()) return -1;
        return openFile[index]->Seek(pos);
    }

    int SetStdIn(char* fileName) {
        if (openFile[CONSOLE_IN]) {
            delete openFile[CONSOLE_IN];
            openFile[CONSOLE_IN] = NULL;
        }
        stdinRedirected = false;
        stdinPath[0] = '\0';
        fileOpenMode[CONSOLE_IN] = MODE_READ;

        if (fileName == NULL || fileName[0] == '\0') return 0;

        int fileDescriptor = openByMode(fileName, MODE_READ);
        if (fileDescriptor == -1) return -1;
        openFile[CONSOLE_IN] = new OpenFile(fileDescriptor);
        stdinRedirected = true;
        strcpy(stdinPath, fileName);
        return 0;
    }

    int SetStdOut(char* fileName) {
        if (openFile[CONSOLE_OUT]) {
            delete openFile[CONSOLE_OUT];
            openFile[CONSOLE_OUT] = NULL;
        }
        stdoutRedirected = false;
        stdoutPath[0] = '\0';
        fileOpenMode[CONSOLE_OUT] = MODE_WRITE;

        if (fileName == NULL || fileName[0] == '\0') return 0;

        int fileDescriptor = openByMode(fileName, MODE_WRITE);
        if (fileDescriptor == -1) return -1;
        openFile[CONSOLE_OUT] = new OpenFile(fileDescriptor);
        stdoutRedirected = true;
        strcpy(stdoutPath, fileName);
        return 0;
    }

    bool IsStdInRedirected() { return stdinRedirected; }

    bool IsStdOutRedirected() { return stdoutRedirected; }

    int ReadStdIn(char* buffer, int charCount) {
        if (!stdinRedirected || openFile[CONSOLE_IN] == NULL) return -1;
        return openFile[CONSOLE_IN]->Read(buffer, charCount);
    }

    int WriteStdOut(char* buffer, int charCount) {
        if (!stdoutRedirected || openFile[CONSOLE_OUT] == NULL) return -1;
        return openFile[CONSOLE_OUT]->Write(buffer, charCount);
    }

    int InheritStdIO(FileTable* parent) {
        int result = 0;
        if (parent->stdinRedirected) {
            if (SetStdIn(parent->stdinPath) == -1) result = -1;
        } else {
            SetStdIn(NULL);
        }

        if (parent->stdoutRedirected) {
            if (SetStdOut(parent->stdoutPath) == -1) result = -1;
        } else {
            SetStdOut(NULL);
        }
        return result;
    }

    void Reset() {
        for (int i = 0; i < FILE_MAX; i++) {
            if (openFile[i]) {
                delete openFile[i];
                openFile[i] = NULL;
            }
        }
        stdinRedirected = false;
        stdoutRedirected = false;
        stdinPath[0] = '\0';
        stdoutPath[0] = '\0';
        fileOpenMode[CONSOLE_IN] = MODE_READ;
        fileOpenMode[CONSOLE_OUT] = MODE_WRITE;
    }

    ~FileTable() {
        Reset();
        delete[] openFile;
        delete[] fileOpenMode;
    }
};

#endif
