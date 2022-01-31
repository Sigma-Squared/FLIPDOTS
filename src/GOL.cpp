#include <GOL.h>

#define SIZE 7

bool get(byte mat[7], uint8_t i, uint8_t j)
{
    if (i < 0)
        i = (SIZE - 1);
    if (i > (SIZE - 1))
        i = 0;
    if (j < 0)
        j = (SIZE - 1);
    if (j > (SIZE - 1))
        j = 0;
    return mat[j] & (1 << i);
}

void set(byte mat[7], uint8_t i, uint8_t j, bool val)
{
    if (val)
        mat[j] |= (1 << i);
    else
        mat[j] &= ~(1 << i);
}

bool isSame(byte mat1[7], byte mat2[7])
{
    for (int i = 0; i < SIZE; i++)
    {
        if (mat1[i] != mat2[i])
            return false;
    }
    return true;
}

void randomize(byte mat[7])
{
    for (int i = 0; i < SIZE; i++)
    {
        mat[i] = random(0, 0xff);
    }
}

// Conway's Game of Life
void GOL(byte in[7])
{
    byte buf[7] = {0};
    memcpy(buf, in, sizeof(buf));
    for (int j = 0; j < SIZE; j++)
    {
        for (int i = 0; i < SIZE; i++)
        {
            int neighbors = get(in, i - 1, j) + get(in, i + 1, j) + get(in, i, j - 1) + get(in, i, j + 1) + get(in, i - 1, j - 1) + get(in, i + 1, j + 1) + get(in, i - 1, j + 1) + get(in, i + 1, j - 1);
            if (get(in, i, j))
            {
                if (neighbors < 2)
                {
                    set(buf, i, j, false);
                }
                else if (neighbors > 3)
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
    if (isSame(in, buf)) // stagnation, restart with random initial state
    {
        randomize(in);
    }
    else
    {
        memcpy(in, buf, sizeof(buf));
    }
}