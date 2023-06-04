int strings_not_equal(char *str1, char *str2)
{
    int str1Length = string_length(str1);
    int str2Length = string_length(str2);
    if (str1Length != str2Length)
    {
        return 1;
    }
    
    char *current1 = str1;
    char *current2 = str2;
    while (*current1 != '\0')
    {
        if (*current1 != *current2)
        {
            return 1;
        }

        current1++;
        current2++;
    }

    return 0;
}