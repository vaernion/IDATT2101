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
        printf("usage: %s <num> <factor> | test | bench\n", argv[0]);
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
    // 100k recursive calls seem fine, likely thanks to the small size of each frame of power
    // may cause segmentation fault on systems with smaller max stack size
    // 1.001^100000 == 2.5e43, well within double's e308 limit
    double args[3][2] = {{1.001, 1000}, {1.001, 10000}, {1.001, 100000}};
    int rounds = 1000;

    // instead of running for 1 second and calling clock() each iteration
    // we run 1000 rounds and divide to get the average time per round
    // because clock() seemed to be slower than the algorithm, and dominating the time taken
    for (int i = 0; i < 3; i++)
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
            power(args[i][0], (int)args[i][1]);
        }
        double customEndTime = (double)clock() / CLOCKS_PER_SEC;
        double customTimeElapsed = (customEndTime - customStartTime) / (double)rounds;

        printf("x = %f, n = %f\n", args[i][0], args[i][1]);
        printf("std:    %f microseconds per operation\n", stdTimeElapsed * 1000000);
        printf("custom: %f microseconds per operation\n", customTimeElapsed * 1000000);
    }
}