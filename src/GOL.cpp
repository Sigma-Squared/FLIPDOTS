#include <GOL.h>

#define BOARD_SIZE 7

bool get(byte mat[BOARD_SIZE], int i, int j)
{
    int iw = ((i < 0 ? (BOARD_SIZE - 1) : i) % BOARD_SIZE);
    int jw = ((j < 0 ? (BOARD_SIZE - 1) : j) % BOARD_SIZE);
    return mat[jw % BOARD_SIZE] & (1 << iw);
}

void set(byte mat[BOARD_SIZE], int i, int j, bool val)
{
    if (val)
        mat[j] |= (1 << i);
    else
        mat[j] &= ~(1 << i);
}

// Conway's Game of Life
void GOL(byte in[BOARD_SIZE])
{
    byte buf[BOARD_SIZE] = {0};
    memcpy(buf, in, sizeof(buf));
    for (int j = 0; j < BOARD_SIZE; j++)
    {
        for (int i = 0; i < BOARD_SIZE; i++)
        {
            int neighbors = get(in, i - 1, j) + get(in, i + 1, j) + get(in, i, j - 1) + get(in, i, j + 1) + get(in, i - 1, j - 1) + get(in, i + 1, j + 1) + get(in, i - 1, j + 1) + get(in, i + 1, j - 1);
            if (get(in, i, j))
            {
                if (neighbors == 1 || neighbors > 3)
                {
                    set(buf, i, j, false);
                }
            }
            else
            {
                if (neighbors == 3)
                {
                    set(buf, i, j, true);
                }
            }
        }
    }
    memcpy(in, buf, sizeof(buf));
}