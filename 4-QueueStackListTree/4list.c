#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NodeStruct
{
    int data;
    struct NodeStruct *next;
} Node;

typedef struct
{
    Node *head;
    int elements;
} CircularSingleLinkedList;

Node *createNode(Node *next, int data)
{
    Node *this = malloc(sizeof(Node));
    this->next = next;
    this->data = data;
    return this;
}

void insertAtStart(CircularSingleLinkedList *list, Node *node)
{
    node->next = list->head;
    list->head = node;
    list->elements++;
}

void generateChain(CircularSingleLinkedList *list, int n)
{
    Node *node = NULL;
    Node *last = NULL;
    for (int i = n; i > 0; i--)
    {
        node = createNode(node, i);
        insertAtStart(list, node);
        if (i == n)
            last = node;
    }
    last->next = node;
    list->head = last;
}

void deleteNextNode(CircularSingleLinkedList *list, Node *prev)
{
    Node *this = prev->next;
    prev->next = this->next; // skip to be deleted node
    if (list->head == this)
        list->head = this->next; // update head if deleted node was head
    list->elements--;
    free(this);
}

int findSafePosition(CircularSingleLinkedList *list, int m)
{
    Node *prev = list->head;

    while (list->elements > 1)
    {
        for (int i = 1; i < m; i++)
        {
            prev = prev->next;
        }

        deleteNextNode(list, prev);
    }
    return list->head->data;
}

int josephus(int n, int m)
{
    CircularSingleLinkedList *list = malloc(sizeof(CircularSingleLinkedList));
    generateChain(list, n);
    int safe = findSafePosition(list, m);
    printf("n: %i m: %i safe: %i", n, m, safe);
    return safe;
}

void runTests()
{
    int testData[][3] = {
        {1, 1, 1},
        {2, 1, 2},
        {2, 4, 1},
        {10, 4, 5},
        {40, 3, 28},
        {256, 2, 1},
        {1000, 24, 265},
        {4321, 42, 1469},
        {123456, 987, 94097},
        {999999, 57, 74063}};

    for (int i = 0; i < sizeof(testData) / sizeof(testData[0]); i++)
    {
        int n = testData[i][0];
        int m = testData[i][1];
        int safe = josephus(n, m);
        printf(" valid: %i\n", safe == testData[i][2]);
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "-t") == 0)
    {
        runTests();
        return 0;
    }
    if (argc < 3)
    {
        printf("usage: %s <n> <interval> | -t\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    if (n <= 0 || m <= 0)
    {
        printf("arguments must be integers above 0\n");
        return 1;
    }
    josephus(n, m);
    printf("\n");
}