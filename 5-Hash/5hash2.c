#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// const int TABLE_SIZE = 10000019;
#define TABLE_SIZE 10000019
// #define TABLE_SIZE 101

void genRandom(long nums[])
{
    long num = 0;
    for (long i = 0; i < TABLE_SIZE; i++)
    {
        num += (rand() % 1000) + 1;
        nums[i] = num;
        // printf("%d\n", num);
    }
}

// Fisher-Yates shuffle
void shuffle(long nums[])
{
    for (long i = 1; i < TABLE_SIZE; i++)
    {
        long j = rand() % (i + 1);
        long temp = nums[i];
        nums[i] = nums[j];
        nums[j] = temp;
    }
}

long modHash(long num)
{
    return num % TABLE_SIZE;
}

long probeLinear(long table[], long num)
{
    long collisions = 0;
    long h = modHash(num);

    for (long i = 0; i < TABLE_SIZE; i++)
    {
        long index = modHash(h + i);
        if (table[index] == 0)
        {
            table[index] = num;
            return collisions;
        }
        else
        {
            collisions++;
        }
    }

    // Should never run
    printf("ERROR: COULD NOT INSERT %ld\n", num);
    return collisions;
}

void test(long nums[])
{
    long (*probes[])() = {probeLinear};
    char probeNames[][50] = {"linear probe", "quadratic probe", "double hashing"};
    static long table[TABLE_SIZE] = {0}; // hash table

    double percents[] = {0.5, 0.8, 0.9, 0.99, 1};

    for (int i = 0; i < 1; i++)
    {
        printf("starting test #%i %s\n", i + 1, probeNames[i]);
        for (int j = 0; j < 5; j++)
        {
            float startTime = (float)clock() / CLOCKS_PER_SEC;
            long collisions = 0;
            long elementsUsed = TABLE_SIZE * percents[j];

            for (long k = 0; k < elementsUsed; k++)
            {
                long c = probes[i](table, nums[k]);
                collisions += c;
            }

            float endTime = (float)clock() / CLOCKS_PER_SEC;
            float timeElapsed = (endTime - startTime);
            printf("used: %.2f elements: %ld collisions: %ld coll_avg.: %.2f time: %.2fs\n",
                   percents[j], elementsUsed, collisions, (double)collisions / TABLE_SIZE, timeElapsed);

            // reset hash table
            for (int r = 0; r < TABLE_SIZE; r++)
            {
                table[r] = 0;
            }
        }
        printf("finished test #%i\n", i + 1);
    }
}

int main(int argc, char *argv[])
{
    srand(time(0));               // seed rng
    static long nums[TABLE_SIZE]; // random unique input numbers

    genRandom(nums);
    shuffle(nums);

    test(nums);
}
