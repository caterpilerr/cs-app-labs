void phase_3(char *str) 
{
    int a, b;
    char *format ="%d %d";
    int numbersCount = sscanf(str, format, &a, &b);
    if (numbersCount < 2)
    {
        explode_bomb();
    }

    if ((unsigned int)a > 7)
    {
        explode_bomb();
    }

    int k = 0;
    switch (a)
    {
        case 0:
            // k = 207
            k = 0xcf;
            break;
        case 1:
            // k = 311
            k = 0x137;
            break;
        case 2:
            // k = 707
            k = 0x2c3; 
            break;
        case 3:
            // k = 256
            k = 0x100;
            break;
        case 4:
            // k = 389
            k = 0x185;
            break;
        case 5:
            // k = 206
            k = 0xce;
            break;
        case 6:
            // k = 767
            k = 0x2ff;
            break;
        default:
            explode_bomb();
            break;
    }

    if (k != b)
    {
        explode_bomb();
    }
}