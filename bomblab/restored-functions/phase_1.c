void phase_1(char *input)
{
    char* phase_1_secret = "Border relations with Canada have never been better.";

    if (strings_not_equal(input, phase_1_secret))
    {
        explode_bomb();
    }
}