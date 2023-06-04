void phase_2(char *input)
{
    int numbers[6];
    int numberCount = read_six_numbers(input, numbers);
    if (numberCount < 6)
    {
        explode_bomb();
    }

    if (numbers[0] != 1)
    {
        explode_bomb();
    }

    int i = 1;
    while (i < 6)
    {
        if (numbers[i] != 2 * numbers[i - 1])
        {
            explode_bomb();
        }
    }
}