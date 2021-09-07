#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

void quicksort(int t[], int v, int h);
int median3sort(int t[], int v, int h);
int splitt(int t[], int v, int h);
void bytt(int t[], int a, int b);
void copyArr(int source[], int copy[], int length);
void fillWithRandom(int t[], int length, int max);
bool sortCheck(int t[], int length);
long long tableChecksum(int t[], int length);
float testSort(int t[], int length, bool useHelper, void (*helper)(), int splitValue);
void quicksortWithHelp(int t[], int v, int h, void (*helper)(), int splitValue);
void bubbleHelper(int t[], int v, int h);

const float TEST_ROUNDS = 1;

int main(int argc, char *argv[])
{
    srand(time(0));

    int MILLION = 10000;
    int dataMillion[MILLION];
    fillWithRandom(dataMillion, MILLION, RAND_MAX);
    int dataLength = sizeof(dataMillion) / sizeof(int);
    float original = testSort(dataMillion, dataLength, false, NULL, 1);
    float helped = testSort(dataMillion, dataLength, true, &bubbleHelper, 1);
    if (original && helped)
        printf("speed delta factor helped/original: %f\n", helped / original);
}

float testSort(int t[], int length, bool useHelper, void (*helper)(), int splitValue)
{
    const long long checksum = tableChecksum(t, length);

    float startTime = (float)clock() / CLOCKS_PER_SEC;

    for (int i = 0; i < TEST_ROUNDS; i++)
    {
        int tCopy[length];
        copyArr(t, tCopy, length);

        if (useHelper)
        {
            quicksortWithHelp(tCopy, 0, length - 1, helper, splitValue);
        }
        else
        {
            quicksort(tCopy, 0, length - 1);
        }

        long long checksumCopy = tableChecksum(tCopy, length);
        bool isSorted = sortCheck(tCopy, length);

        if (checksum != checksumCopy)
        {
            printf("Error: The checksums do not match, useHelper: %i\n", useHelper);
            return 0;
        }
        if (!isSorted)
        {
            printf("Error: copy is not sorted properly, useHelper: %i\n", useHelper);
            return 0;
        }
    }

    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = (endTime - startTime) / TEST_ROUNDS;

    float startTimeCopy = (float)clock() / CLOCKS_PER_SEC;

    for (int i = 0; i < TEST_ROUNDS; i++)
    {
        int tCopy[length];
        copyArr(t, tCopy, length);
        long long checksumCopy = tableChecksum(tCopy, length);

        if (checksum != checksumCopy)
        {
            printf("Error: The checksums do not match, useHelper: %i\n", useHelper);
            return 0;
        }
    }

    float endTimeCopy = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsedCopy = (endTimeCopy - startTimeCopy) / TEST_ROUNDS;

    float finalElapsedTime = timeElapsed - timeElapsedCopy;
    printf("time per execution (length: %i, helper: %i, checksum: %lli) :\n all: %f copying: %f actual: %f seconds\n",
           length, useHelper, checksum, timeElapsed, timeElapsedCopy, finalElapsedTime);
    return finalElapsedTime;
}

bool sortCheck(int t[], int length)
{
    for (int i = length - 1; i > 0; i--)
    {
        if (t[i] < t[i - 1])
        {
            return false;
        }
    }
    return true;
}

long long tableChecksum(int t[], int length)
{
    int sum = 0;
    for (int i = 0; i < length; i++)
    {
        sum += t[i];
    }
    return sum;
}

void fillWithRandom(int t[], int length, int max)
{
    for (int i = 0; i < length; i++)
    {
        t[i] = rand() % max + 1;
    }
}

void copyArr(int source[], int copy[], int length)
{
    for (int i = 0; i < length; i++)
    {
        copy[i] = source[i];
    }
}

void quicksort(int t[], int v, int h)
{
    if (h - v > 2)
    {
        int delops = splitt(t, v, h);
        quicksort(t, v, delops - 1);
        quicksort(t, delops + 1, h);
    }
    else
    {
        median3sort(t, v, h);
    }
}

void quicksortWithHelp(int t[], int v, int h, void (*helper)(), int splitValue)
{
    if (h - v > splitValue)
    {
        int delops = splitt(t, v, h);
        quicksortWithHelp(t, v, delops - 1, helper, splitValue);
        quicksortWithHelp(t, delops + 1, h, helper, splitValue);
    }
    else
    {
        helper(t, v, h);
    }
}

int median3sort(int t[], int v, int h)
{
    int m = (v + h) / 2;
    if (t[v] > t[m])
        bytt(t, v, m);

    if (t[m] > t[h])
    {
        bytt(t, m, h);
        if (t[v] > t[m])
            bytt(t, v, m);
    }
    return m;
}

int splitt(int t[], int v, int h)
{
    int iv, ih;
    int m = median3sort(t, v, h);
    int dv = t[m];
    bytt(t, m, h - 1);
    iv = v, ih = h - 1;
    while (1)
    {
        while (t[++iv] < dv)
            ;
        while (t[--ih] > dv)
            ;
        if (iv >= ih)
            break;

        bytt(t, iv, ih);
    }
    bytt(t, iv, h - 1);
    return iv;
}

void bytt(int t[], int a, int b)
{
    int temp = t[a];
    t[a] = t[b];
    t[b] = temp;
}

void bubbleHelper(int t[], int v, int h)
{
    for (int i = h; i > v; --i)
    {
        for (int j = 0; j < i; ++j)
        {
            if (t[j] > t[j + 1])
                bytt(t, j, j + 1);
        }
    }
}