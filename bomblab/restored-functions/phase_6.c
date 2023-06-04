void phase_6(char *str)
{
    int numbers[6];
    read_six_numbers(str, numbers);
    int i;
    for (i = 0; i < 6; i++)
    {
        if ((unsigned int)numbers[i] - 1 > 5)
        {
            explode_bomb();
        }

        int j;
        for (j = i; j < 6; j++)
        {
            if (numbers[j] == numbers[i])
            {
                explode_bomb();
            }
        }
    }

    for (i = 0; i < 6; i++)
    {
        numbers[i] = 7 - numbers[i];
    }

    int **memory[6];
    int v6 = 0x1bb;
    int v5 = 0x1dd;
    int v4 = 0x2b3;
    int v3 = 0x39c;
    int v2 = 0xa8;
    int v1 = 0x142;

    void **p6A[] = {v6, 0x0};
    void **p5A[] = {v5, p6A};
    void **p4A[] = {v4, p5A};
    void **p3A[] = {v3, p4A};
    void **p2A[] = {v2, p3A};
    void **p1A[] = {v1, p2A};

    for (i = 0; i < 6; i = i++)
    {
        int number = numbers[i];
        void **current = p1A;
        while (number > 1)
        {
            current = *(current + 1);
            number--;
        }
        
        memory[i] = current;
    }

    for (i = 0; i < 5; i++)
    {
        *(memory[i] + 1) = memory[i + 1];
    }

    for (i = 0; i < 5; i++)
    {
        if ((int)*memory[i] <= (int)(*(*(memory[i] + 1))))
        {
            explode_bomb();
        }
    }
}