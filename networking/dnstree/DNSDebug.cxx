#include <stdio.h>

void DumpBufferAsHex(const char* buffer, int cb)
{
    int cbLine = 0;
    for (int i = 0; i < cb; i++)
    {
        printf("0x%.2x (%c) ", (unsigned char)buffer[i], buffer[i]);
        cbLine++;
        if (cbLine == 8)
        {
            printf("\n");
            cbLine = 0;
        }
    }
}

