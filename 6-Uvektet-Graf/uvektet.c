#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define infinity 1000000000

// 18058 Moholt
// 37774 Kalvskinnet

typedef struct EdgeStruct
{
    long to;
    struct EdgeStruct next;
} Edge;

typedef struct NodeStruct
{
    long distance;
    long previous;
} Node;

typedef struct QueueItemStruct
{
    long data;
    struct QueueItemStruct *next;
} QueueItem;

typedef struct QueueStruct
{
    QueueItem *head;
    QueueItem *tail;
} Queue;

void queueInsert(Queue *queue, long data)
{
    QueueItem *item = calloc(1, sizeof(QueueItem));
    item->data = data;
    if (queue->tail != NULL)
    {
        queue->tail->next = item;
    }
    queue->tail = item;
    if (queue->head == NULL)
    {
        queue->head = item;
    }
}

long queueGet(Queue *queue)
{
    QueueItem *item = queue->head;
    long data = item->data;
    queue->head = item->next;
    free(item);
    if (queue->head == NULL)
        queue->tail = NULL;
    return data;
}

void edgeListInsert(Edge *edges[], int from, int to)
{
    Edge *edge = calloc(1, sizeof(Edge));
    edge->to = to;

    if (edges[from] != NULL)
    {
        edge->next = edges[from];
    }

    edges[from] = edge;
}

void readGraphFile(FILE *fp, Edge *edges[])
{
    const int buffSize = 1024;
    char buffer[buffSize];

    while (fgets(buffer, buffSize, fp))
    {
        long from = atoi(strtok(buffer, " "));
        long to = atoi(strtok(NULL, " "));
        edgeListInsert(edges, from, to);
    }

    fclose(fp);
}

void displayBFS(Node *nodes[], int n)
{
    printf("%5s  %5s  %5s\n", "Node", "Forgj", "Dist");
    for (long i = 0; i < n; i++)
    {
        if (nodes[i]->previous == infinity)
        {
            printf("%5ld %5s %5s\n", i, "none", "∞");
        }
        else
        {
            printf("%5ld %5ld %5ld\n", i, nodes[i]->previous, nodes[i]->distance);
        }
    }
}

void bfs(char file[], int start)
{
    // read first line in file here to avoid pointer issues
    // and to know the length nodes should be
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }
    const int buffSize = 1024;
    char buffer[buffSize];
    fgets(buffer, buffSize, fp);
    long n = atoi(strtok(buffer, " "));
    printf("N: %ld", n);

    Node **nodes = calloc(n, sizeof(Node));
    Edge edges[] = calloc(n, sizeof(Edge));
    for (long i = 0; i < n; i++)
    {
        // Node *node = malloc(sizeof(Node));
        nodes[i].distance = infinity;
        nodes[i].previous = infinity;
        // nodes[i] = node;
        //edges[i] = NULL;
        // free(node);
    }

    printf("while loop start\n");

    readGraphFile(fp, edges);

    Queue *queue = calloc(1, sizeof(Queue));
    queueInsert(queue, start);

    printf("while loop start\n");

    while (queue->head != NULL)
    {
        long nodeIndex = queueGet(queue);
        Node prevNode = nodes[nodeIndex];
        if (prevNode.previous == infinity)
        {
            prevNode.previous = 0;
            prevNode.distance = 0;
        }

        for (Edge *edge = edges[nodeIndex]; edge; edge = edge->next)
        {
            Node *node = nodes[edge->to];
            if (node->distance == infinity)
            {
                node->distance = prevNode.distance + 1;
                node->previous = nodeIndex;
                queueInsert(queue, edge->to);
            }
        }
    }
    free(queue);

    displayBFS(nodes, n);
}

int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        if (strcmp(argv[1], "bfs") == 0)
        {
            if (argc < 4)
            {
                printf("BFS needs a start index\n");
                return 1;
            }

            bfs(argv[2], atoi(argv[3]));
        }
        else if (strcmp(argv[1], "top") == 0)
        {
            printf("top\n");
            // top(argv[2])
        }
    }
    else
    {
        printf("usage: %s <bfs|top> <file> [bfsStart]\n", argv[0]);
        return 1;
    }
}