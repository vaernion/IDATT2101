#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void stonks_n2(int start, int flucs[], int values[], int n, int *buy, int *sell);
void random_fluctuations(int arr[], int dayCount);
void run_tests();
void run_single_test(int fluctuations[], int n);
void display_results(int values[], int buyDate, int sellDate);

const int STOCK_START = 42;
const bool SHOW_OUTPUT = false;
const bool RUN_BENCHMARKS = true;

int main()
{
    time_t t;
    srand((unsigned)time(&t));

    if (RUN_BENCHMARKS)
    {
        run_tests();
    }
    else
    {
        int flucs[] = {-1, 3, -9, 2, 2, -1, 2, -1, -5};
        int n = sizeof(flucs) / sizeof(int);
        int values[n];
        int buyDate, sellDate;
        stonks_n2(10, flucs, values, n, &buyDate, &sellDate);
        display_results(values, buyDate, sellDate);
    }

    return 0;
}

void run_tests()
{
    int ns[] = {100, 1000, 10000, 100000};

    for (int n = 0; n < sizeof(ns) / sizeof(int); n++)
    {
        printf("---------Start test #%i---------\n", n + 1);

        int fluctuations[ns[n]];
        random_fluctuations(fluctuations, ns[n]);

        float startTime = (float)clock() / CLOCKS_PER_SEC;

        run_single_test(fluctuations, ns[n]);

        float endTime = (float)clock() / CLOCKS_PER_SEC;

        float timeElapsed = endTime - startTime;

        printf("took %f seconds for %i calculations\n", timeElapsed, ns[n]);
    }
}

void run_single_test(int fluctuations[], int n)
{
    int values[n];

    int buyDate = 0, sellDate = 0;

    stonks_n2(STOCK_START, fluctuations, values, n, &buyDate, &sellDate);

    if (SHOW_OUTPUT)
    {
        display_results(values, buyDate, sellDate);
    }
}

void display_results(int values[], int buyDate, int sellDate)
{
    printf("buy on day %i for price %i\n", buyDate + 1, values[buyDate]);
    printf("sell on day %i for price %i\n", sellDate + 1, values[sellDate]);
    printf("you just earned %i 💃\n", values[sellDate] - values[buyDate]);
}

void random_fluctuations(int arr[], int dayCount)
{
    for (int i = 0; i < dayCount; i++)
    {
        arr[i] = rand() % 20 - 10;
    }
}

void stonks_n2(int start, int flucs[], int values[], int n, int *buy, int *sell)
{

    int value = start;
    int largestDifference = 0;

    for (int i = 0; i < n; i++)
    {
        value += flucs[i];

        // prevent negative stock costs
        if (value < 1)
        {
            value = 1;
        }

        values[i] = value;

        int nextValue = value;

        for (int j = i + 1; j < n; j++)
        {
            nextValue += flucs[j];

            // prevent negative stock costs
            if (nextValue < 1)
            {
                nextValue = 1;
            }

            int diff = nextValue - value;
            if (diff > largestDifference)
            {
                largestDifference = diff;
                *buy = i;
                *sell = j;
            }
        }
    }
}
