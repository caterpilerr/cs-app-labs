void phase_5(char *str)
{
    if (string_length(str) != 6)
    {
        explode_bomb();
    }

    int i = 0;
    char builtString[7];
    char *template = "maduiersnfotbylSo you think you can stop the bomb with ctrl+c, do you?";
    while (i != 6)
    {
        int offset = str[i] & 0xff;
        builtString[i] = template[offset];
    }

    builtString[7] = '\0';
    char *original = "flyers";
    if (strings_not_equal(builtString, original))
    {
        explode_bomb();
    }   
}