#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

void quicksort(int t[], int v, int h);
int median3sort(int t[], int v, int h);
int splitt(int t[], int v, int h);
void bytt(int t[], int a, int b);
void copyArr(int source[], int copy[], long length);
void fillWithRandom(int t[], long length, long max);
bool sortCheck(int t[], long length);
long long tableChecksum(int t[], long length);
float testSort(int t[], long length, bool useHelper, void (*helper)(), int splitValue);
void quicksortWithHelp(int t[], int v, int h, void (*helper)(), int splitValue);
void bubbleSortHelper(int t[], int v, int h);
void insertionSortHelper(int t[], int v, int h);
int fitSplitValue(int t[], long length, void (*helper)());

const float TEST_ROUNDS = 3; // find average time to sort by doing several rounds on new data
const long MILLION = 1000000;

int main(int argc, char *argv[])
{
    srand(time(0)); // seed rng

    const long dataLength = MILLION;
    int randomData[dataLength]; // never mutated after rng filling, used as source for copies
    fillWithRandom(randomData, dataLength, RAND_MAX);

    if (argc > 2 && strcmp(argv[1], "fit") == 0)
    {
        if (strcmp(argv[2], "b") == 0)
        {

            printf("### fitting bubble sort helper with %li length random data ###\n", dataLength);
            fitSplitValue(randomData, dataLength, &bubbleSortHelper);
            return 0;
        }
        else if (strcmp(argv[2], "i") == 0)
        {
            printf("### fitting insertion sort helper with %li length random data ###\n", dataLength);
            fitSplitValue(randomData, dataLength, &insertionSortHelper);
            return 0;
        }
    }

    printf("### original quicksort ###\n");
    float original = testSort(randomData, dataLength, false, NULL, 2);

    printf("\n### quicksort with bubble sort ###\n");
    float bubbleTime = testSort(randomData, dataLength, true, &bubbleSortHelper, 30);
    if (original && bubbleTime)
        printf("time factor helped/original: %f\n", bubbleTime / original);

    printf("\n### quicksort with insertion sort ###\n");
    float insertTime = testSort(randomData, dataLength, true, &insertionSortHelper, 23);
    if (original && bubbleTime)
        printf("time factor helped/original: %f\n", insertTime / original);
}

float testSort(int t[], long length, bool useHelper, void (*helper)(), int splitValue)
{
    const long long checksum = tableChecksum(t, length);
    long long checksumCopy;
    bool isSorted;

    float startTime = (float)clock() / CLOCKS_PER_SEC;
    for (int i = 0; i < TEST_ROUNDS; i++)
    {
        int tCopy[length];
        copyArr(t, tCopy, length);

        useHelper ? quicksortWithHelp(tCopy, 0, length - 1, helper, splitValue)
                  : quicksort(tCopy, 0, length - 1);

        checksumCopy = tableChecksum(tCopy, length);
        isSorted = sortCheck(tCopy, length);

        if (checksum != checksumCopy)
        {
            printf("Error: The checksums do not match, useHelper: %i\n", useHelper);
            exit(1);
        }
        if (!isSorted)
        {
            printf("Error: The copy is not sorted properly, useHelper: %i\n", useHelper);
            exit(1);
        }
    }
    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = (endTime - startTime) / TEST_ROUNDS;

    // repeat copying and checksumming to find more accurate time for quicksort
    float startTimeCopy = (float)clock() / CLOCKS_PER_SEC;
    for (int i = 0; i < TEST_ROUNDS; i++)
    {
        int tCopy[length];
        copyArr(t, tCopy, length);
        // include checksum for more accurate final time
        // sortCheck() is more inconvient to include
        tableChecksum(tCopy, length);
    }
    float endTimeCopy = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsedCopy = (endTimeCopy - startTimeCopy) / TEST_ROUNDS;

    float actualElapsedTime = timeElapsed - timeElapsedCopy;
    printf("average: %f sec (all: %f copying: %f) -- length: %li, split: %i, useHelper: %i -- original: %lli copy: %lli, isSorted: %i): \n",
           actualElapsedTime, timeElapsed, timeElapsedCopy, length, splitValue, useHelper, checksum, checksumCopy, isSorted);
    return actualElapsedTime;
}

int fitSplitValue(int t[], long length, void (*helper)())
{
    const int MAX_SIZE = 300;
    float bestTime = 9999;
    int bestSize = 0;

    int size = 2;
    while (size < MAX_SIZE)
    {
        float time = testSort(t, length, true, helper, size);
        if (time < bestTime)
        {
            bestTime = time;
            bestSize = size;
        }

        if (size < 30)
        {
            size += 1;
        }
        else if (size < 100)
        {
            size += 2;
        }
        else if (size < 200)
        {
            size += 5;
        }
        else
        {
            size += 10;
        }
    }

    printf("optimal size: %i time: %f with %li elements\n", bestSize, bestTime, length);
    return bestSize;
}

bool sortCheck(int t[], long length)
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

long long tableChecksum(int t[], long length)
{
    long long sum = 0;
    for (int i = 0; i < length; i++)
    {
        sum += t[i];
    }
    return sum;
}

void fillWithRandom(int t[], long length, long max)
{
    for (int i = 0; i < length; i++)
    {
        t[i] = rand() % (max + 1);
    }
}

void copyArr(int source[], int copy[], long length)
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
    if ((h - v) > splitValue)
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

void bubbleSortHelper(int t[], int v, int h)
{
    for (int i = h; i > v; --i)
    {
        for (int j = v; j < i; ++j)
        {
            if (t[j] > t[j + 1])
                bytt(t, j, j + 1);
        }
    }
}

void insertionSortHelper(int t[], int v, int h)
{
    for (int j = v + 1; j <= h; ++j)
    {
        int bytt = t[j];
        int i = j - 1;
        while (i >= 0 && t[i] > bytt)
        {
            t[i + 1] = t[i];
            --i;
        }
        t[i + 1] = bytt;
    }
}