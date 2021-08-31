#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>

double power(double x, int n);
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
        printf("usage: pwr <num> <factor> | test | bench\n");
        return 1;
    }

    double x = atof(argv[1]);
    int n = atoi(argv[2]);

    double res = power(x, n);

    printf("result is %f\n", res);

    return 0;
}

double power(double x, int n)
{
    if (n == 0)
        return 1;

    return x * power(x, n - 1);
}

void runTest()
{
    double x1 = power(2, 10);
    printf("2^10 == %f == 1024\n", x1);

    double x2 = power(3, 14);
    printf("3^14 == %f == 4782969\n", x2);
}

void runBench()
{
    int args[3][2] = {{20000, 1000}, {20000, 10000}, {20000, 100000}};

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
            power(args[i][0], args[i][1]);
            customEndTime = (double)clock() / CLOCKS_PER_SEC;
            customCount++;
        } while (customEndTime - customStartTime < 1);
        double customTimeElapsed = (customEndTime - customStartTime) / customCount;

        printf("x = %i, n = %i\n", args[i][0], args[i][1]);
        printf("std:    %f microseconds per operation ran %i times\n", stdTimeElapsed * 1000000, stdCount);
        printf("custom: %f microseconds per operation ran %i times\n\n", customTimeElapsed * 1000000, customCount);
    }
}