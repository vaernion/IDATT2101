#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

long crazy_power(long x, int n);
void runTest();
void runBench();

int main(int argc, char *argv[])
{
    if (!strcmp(argv[1], "test"))
    {
        runTest();
        return 0;
    }
    else if (!strcmp(argv[1], "bench"))
    {
        runBench();
        return 0;
    }
    else if (argc < 3)
    {
        printf("usage: crzy_pwr <num> <factor> | test | bench\n");
        return 1;
    }

    long x = atol(argv[1]);
    int n = atoi(argv[2]);

    long res = crazy_power(x, n);

    printf("result is %li\n", res);

    return 0;
}

long crazy_power(long x, int n)
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
    int args[3][4] = {{20, 1000}, {20, 10000}, {20, 100000}};

    for (int i = 0; i < 3; i++)
    {
        double stdStartTime = (double)clock() / CLOCKS_PER_SEC;
        double stdEndTime;
        int stdCount = 0;
        do
        {
            pow(args[i][0], args[i][1]);
            stdEndTime = (double)clock() / CLOCKS_PER_SEC;
            stdCount++;
        } while (stdEndTime - stdStartTime < 1);
        double stdTimeElapsed = (stdEndTime - stdStartTime) / stdCount;

        double customStartTime = (double)clock() / CLOCKS_PER_SEC;
        double customEndTime = (double)clock() / CLOCKS_PER_SEC;
        int customCount;
        do
        {
            crazy_power(args[i][0], args[i][1]);
            customEndTime = (double)clock() / CLOCKS_PER_SEC;
            customCount++;
        } while (customEndTime - customStartTime < 1);
        double customTimeElapsed = (customEndTime - customStartTime) / customCount;

        printf("x = %i, n = %i\n", args[i][0], args[i][1]);
        printf("std:    %f microseconds per operation ran %i times\n", stdTimeElapsed * 1000000, stdCount);
        printf("custom: %f microseconds per operation ran %i times\n\n", customTimeElapsed * 1000000, customCount);
    }
}