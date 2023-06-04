void phase_4(char *str)
{
    int a, b;
    char *format = "%d %d";
    int numbersCount = sscanf(str, format, &a, &b);
    if (numbersCount != 2)
    {
        explode_bomb();
    }

    if ((unsigned int)a > 0xe)
    {
        explode_bombe();
    }

    int k = func4(a, 0, 0xe);
    if (k != 0)
    {
       explode_bomb(); 
    }

    if (b != 0)
    {
        explode_bomb();
    }
}