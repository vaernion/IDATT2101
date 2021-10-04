#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_SIZE 50
#define TABLE_SIZE 128

int nodes = 0;
int collisions = 0;

typedef struct NodeStruct
{
    char data[DATA_SIZE];
    struct NodeStruct *next;
} Node;

Node *createNode(char data[])
{
    Node *node = malloc(sizeof(Node));
    strncpy(node->data, data, sizeof(node->data));
    return node;
}

int hash(char key[])
{
    unsigned k = 0;
    unsigned x = 7;

    // weigh each char by 7^index
    for (int i = 0; i < strlen(key); i++)
    {
        k += (int)key[i] * x;
        x *= 7;
    }
    // TABLE_SIZE should be prime
    return k % TABLE_SIZE;
}

// adds a string as key and value to a hash table
// uses linked lists to handle collisions
void insert(Node *table[], char data[])
{
    int index = hash(data);
    Node *node = createNode(data);
    nodes++;

    if (table[index] != NULL)
    {
        printf("Collision -- n1:%s -- n2:%s\n", data, table[index]->data);
        collisions++;
        node->next = table[index];
    }

    table[index] = node;
}

// looks up a string key in hash table, if found writes to result
// does nothing if it does not exist at hashed index
void lookup(Node *table[], char key[], char result[])
{
    int index = hash(key);
    if (table[index] != NULL)
    {
        // traverse linked list until key is found
        for (Node *node = table[index]; node != NULL; node = node->next)
        {
            if (strncmp(key, node->data, DATA_SIZE) == 0)
            {
                strncpy(result, node->data, DATA_SIZE);
                return;
            }
        }
    }
}

void lookupAndPrint(Node *table[], char key[])
{
    char result[DATA_SIZE] = "";
    lookup(table, key, result);
    if (strncmp(result, "", DATA_SIZE) != 0)
    {
        printf("Found: %s\n", result);
    }
}

// reads a file and adds each line to hash table
void readLinesToHash(Node *table[], char filename[])
{
    const int buffSize = DATA_SIZE;
    char buffer[buffSize];

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    while (fgets(buffer, buffSize, fp))
    {
        buffer[strcspn(buffer, "\n")] = 0; // remove trailing newline
        insert(table, buffer);
    }
}

int main(int argc, char *argv[])
{
    Node *table[TABLE_SIZE] = {NULL};
    char filename[] = "navn.txt";

    if (argc > 1)
        readLinesToHash(table, argv[1]);
    else
        readLinesToHash(table, filename);

    printf("Collisions per person: %f\n", collisions / (float)nodes);
    printf("Collisions: %d -- Load factor: %f\n", collisions, nodes / (float)TABLE_SIZE);

    lookupAndPrint(table, "Grunde Andreas Grepstad Thorheim");
    lookupAndPrint(table, "Kristoffer Øyen");
    lookupAndPrint(table, "Marcus Aurelius"); // does nothing as expected
}