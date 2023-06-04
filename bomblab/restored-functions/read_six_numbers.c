void read_six_numbers(char *str, int numbers[])
{
    char *format = "%d %d %d %d %d %d";
    int numbersCount = sscanf(str, format,
        &numbers[0],
        &numbers[1],
        &numbers[2],
        &numbers[3],
        &numbers[4],
        &numbers[5]);

    if (numbersCount < 6)
    {
        explode_bomb();
    }
}