#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TABLE_SIZE 10000019

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
    long index = h;

    for (long i = 0; i < TABLE_SIZE; i++)
    {
        if (table[index] == 0)
        {
            table[index] = num;
            return collisions;
        }
        else
        {
            index = modHash(h + i + 1);
            collisions++;
        }
    }

    printf("ERROR: COULD NOT INSERT %ld\n", num);
    return collisions;
}

long probeQuadratic(long table[], long num)
{
    long collisions = 0;
    long h = modHash(num);
    const int k1 = 1;
    const int k2 = 1;

    for (long i = 0; i < TABLE_SIZE; i++)
    {
        long index = modHash(h + (k1 * i) + (k2 * (i * i)));
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

    printf("ERROR: COULD NOT INSERT %ld\n", num);
    return collisions;
}

long modHash2(long num)
{
    long newMod = TABLE_SIZE - 1;
    return num % newMod + 1;
}

long doubleProbe(long table[], long num)
{
    long collisions = 0;
    long h1 = modHash(num);
    long h2;
    long index = h1;

    if (table[index] != 0)
        h2 = modHash2(num);

    for (long i = 0; i < TABLE_SIZE; i++)
    {
        if (table[index] == 0)
        {
            table[index] = num;
            return collisions;
        }
        else
        {
            index = modHash(index + h2);
            collisions++;
        }
    }

    printf("ERROR: COULD NOT INSERT %ld\n", num);
    return collisions;
}

void test(long nums[])
{
    long (*probes[])(long[], long) = {&probeLinear, &probeQuadratic, &doubleProbe};
    char probeNames[][30] = {"linear probe", "quadratic probe", "double hashing"};
    double percents[] = {0.5, 0.8, 0.9, 0.99, 1};

    static long table[TABLE_SIZE] = {0}; // hash table, cleared and reused between tests

    for (int i = 0; i < 3; i++)
    {
        printf("starting test #%i %s\n", i + 1, probeNames[i]);
        for (int j = 0; j < 5; j++)
        {
            long collisions = 0;
            long elementsUsed = TABLE_SIZE * percents[j];

            double startTime = (double)clock() / CLOCKS_PER_SEC;
            for (long k = 0; k < elementsUsed; k++)
            {
                long c = (*probes[i])(table, nums[k]);
                collisions += c;
            }
            double endTime = (double)clock() / CLOCKS_PER_SEC;
            double timeElapsed = endTime - startTime;

            // verify
            long actuallyUsed = 0;
            for (int v = 0; v < TABLE_SIZE; v++)
            {
                if (table[v] != 0)
                    actuallyUsed++;
            }
            if (actuallyUsed != elementsUsed)
                printf("ERROR: %ld NUMBERS MISSING IN HASH TABLE\n", elementsUsed - actuallyUsed);

            printf("used: %.2f elements: %ld collisions: %ld coll_avg.: %.2f time: %.2lfs\n",
                   percents[j], elementsUsed, collisions, (double)collisions / TABLE_SIZE, timeElapsed);

            // reset hash table
            for (int r = 0; r < TABLE_SIZE; r++)
            {
                table[r] = 0;
            }
        }
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
