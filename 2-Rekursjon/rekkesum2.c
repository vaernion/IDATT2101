#include <stdio.h>
#include <stdlib.h>

int rekkesum(int n)
{
    if (n == 1)
        return 1;
    return rekkesum(n - 1) + n;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("usage: ./rekkesum <int>\n");
        return 1;
    }
    int n = atoi(argv[1]);
    printf("%i\n", n);
    printf("∑i = %i\n", rekkesum(n));
    printf("i=1\n");
    return 0;
}
