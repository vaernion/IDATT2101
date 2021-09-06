#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

double crazy_power(double x, int n);
void runTest();
void runBench();

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "test") == 0)
    {
        runTest();
        return 0;
    }
    else if (argc > 1 && strcmp(argv[1], "bench") == 0)
    {
        runBench();
        return 0;
    }
    else if (argc < 3)
    {
        printf("usage: %s <num> <factor> | test | bench\n", argv[0]);
        return 1;
    }

    long x = atol(argv[1]);
    int n = atoi(argv[2]);

    long res = crazy_power(x, n);

    printf("result is %li\n", res);

    return 0;
}

double crazy_power(double x, int n)
{
    if (n == 0)
        return 1;

    if (n % 2 != 0)
        return x * crazy_power(x * x, (n - 1) / 2);

    return crazy_power(x * x, n / 2);
}

void runTest()
{
    double x1 = crazy_power(2, 10);
    printf("2^10 == %f == 1024\n", x1);

    double x2 = crazy_power(3, 14);
    printf("3^14 == %f == 4782969\n", x2);
}

void runBench()
{
    double args[4][2] = {{1.001, 100}, {1.001, 1000}, {1.001, 10000}, {1.001, 100000}};
    int rounds = 1000;

    for (int i = 0; i < 4; i++)
    {
        double stdStartTime = (double)clock() / CLOCKS_PER_SEC;
        for (int j = 0; j < rounds; j++)
        {
            pow(args[i][0], args[i][1]);
        }
        double stdEndTime = (double)clock() / CLOCKS_PER_SEC;
        double stdTimeElapsed = (stdEndTime - stdStartTime) / (double)rounds;

        double customStartTime = (double)clock() / CLOCKS_PER_SEC;
        for (int j = 0; j < rounds; j++)
        {
            crazy_power(args[i][0], (int)args[i][1]);
        }
        double customEndTime = (double)clock() / CLOCKS_PER_SEC;
        double customTimeElapsed = (customEndTime - customStartTime) / (double)rounds;

        printf("x = %f, n = %f\n", args[i][0], args[i][1]);
        printf("std:    %f microseconds per operation\n", stdTimeElapsed * 1000000);
        printf("custom: %f microseconds per operation\n", customTimeElapsed * 1000000);
    }
}