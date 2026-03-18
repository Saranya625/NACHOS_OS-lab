#include "syscall.h"

void trim(char *s) {
    int i = 0;
    int j = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    while (s[i] != '\0') s[j++] = s[i++];
    s[j] = '\0';

    j--;
    while (j >= 0 && (s[j] == ' ' || s[j] == '\t')) s[j--] = '\0';
}

int main() {
    SpaceId newProc;
    OpenFileId input = _ConsoleInput;
    OpenFileId output = _ConsoleOutput;
    char prompt[2], buffer[128];
    int i;

    prompt[0] = '-';
    prompt[1] = '-';

    while (1) {
        Write(prompt, 2, output);

        i = 0;

        do {
            Read(&buffer[i], 1, input);

        } while (buffer[i++] != '\n');

        buffer[--i] = '\0';
        trim(buffer);

        if (buffer[0] != '\0') {
            int pipePos = -1;
            int k = 0;
            char left[128], right[128];
            char pipeFile[] = ".__shell_pipe_tmp";

            while (buffer[k] != '\0') {
                if (buffer[k] == '|') {
                    pipePos = k;
                    break;
                }
                k++;
            }

            if (pipePos == -1) {
                newProc = Exec(buffer);
                if (newProc >= 0) Join(newProc);
            } else {
                for (k = 0; k < pipePos; k++) left[k] = buffer[k];
                left[pipePos] = '\0';

                k = 0;
                while (buffer[pipePos + 1 + k] != '\0') {
                    right[k] = buffer[pipePos + 1 + k];
                    k++;
                }
                right[k] = '\0';

                trim(left);
                trim(right);

                if (left[0] != '\0' && right[0] != '\0') {
                    CreateFile(pipeFile);
                    SetStdOut(pipeFile);
                    newProc = Exec(left);
                    if (newProc >= 0) Join(newProc);
                    SetStdOut("");

                    SetStdIn(pipeFile);
                    newProc = Exec(right);
                    if (newProc >= 0) Join(newProc);
                    SetStdIn("");
                    Remove(pipeFile);
                }
            }
        }
    }
}
