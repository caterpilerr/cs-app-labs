int func4(int num, int a, int b)
{
    int dif = b - a;
    int k = dif + ((unsigned int)dif >> 31);
    k = k >> 1;
    k = k + 1 * a;
    if (k > num)
    {
        return 2 * func4(num, a, k - 1);
    }

    if (k >= num)
    {
        return 0;
    }

    return 1 + 2 * func4(num, k + 1, k);
}