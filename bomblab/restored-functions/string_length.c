int string_length(char *str) 
{
    int length = 0;
    char *current = str;
    while (*current == '\0')
    {
        current++;
        length = current - str;
    }

    return length;
}