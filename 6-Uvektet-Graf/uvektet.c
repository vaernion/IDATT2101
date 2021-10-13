#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define infinity 1000000000

typedef struct NodeStruct
{
    long nr;
    long distance;
    struct NodeStruct *previous;
    struct EdgeStruct *edgeHead;
    void *data;
} Node;

typedef struct EdgeStruct
{
    Node *to;
    struct EdgeStruct *next;
} Edge;

typedef struct QueueItemStruct
{
    Node *node;
    struct QueueItemStruct *next;
} QueueItem;

typedef struct QueueStruct
{
    QueueItem *head;
    QueueItem *tail;
} Queue;

typedef struct GraphStruct
{
    long n;
    long k;
    Node *nodes;
} Graph;

typedef struct
{
    bool found;
    Node *next;
} TopoList;

void queueInsert(Queue *queue, Node *node)
{
    QueueItem *item = calloc(1, sizeof(QueueItem));
    item->node = node;
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

Node *queueGet(Queue *queue)
{
    QueueItem *item = queue->head;
    Node *node = item->node;
    queue->head = item->next;
    free(item);
    if (queue->head == NULL)
        queue->tail = NULL;
    return node;
}

void edgeListInsert(Node *nodes, int from, int to)
{
    Edge *edge = calloc(1, sizeof(Edge));
    edge->to = &nodes[to];
    edge->next = nodes[from].edgeHead;
    nodes[from].edgeHead = edge;
}

Graph *readGraphFile(char file[])
{
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    Graph *graph = malloc(sizeof(Graph));

    fscanf(fp, "%li %li\n", &graph->n, &graph->k);
    printf("n: %li k: %li\n", graph->n, graph->k);

    graph->nodes = calloc(graph->n, sizeof(Node));

    for (long i = 0; i < graph->k; i++)
    {
        long from, to;
        fscanf(fp, "%li %li\n", &from, &to);
        edgeListInsert(graph->nodes, from, to);
    }

    fclose(fp);
    return graph;
}

void displayBFS(Node *nodes, long n, int start)
{
    printf("%7s %7s %7s\n", "Node", "Forgj", "Dist");
    for (long i = 0; i < n; i++)
    {
        if (i == start)
        {
            printf("%7ld %7s %7s\n", i, "start", "0");
        }
        else if (nodes[i].previous == NULL)
        {
            printf("%7ld %7s %7s\n", i, "none", "∞");
        }
        else
        {
            printf("%7ld %7ld %7ld\n", i, nodes[i].previous->nr, nodes[i].distance);
        }
    }
}

void bfs(char file[], int start)
{
    Graph *graph = readGraphFile(file);
    long n = graph->n;
    Node *nodes = graph->nodes;

    for (long i = 0; i < n; i++)
    {
        nodes[i].nr = i;
        nodes[i].distance = infinity;
    }

    Queue *queue = calloc(1, sizeof(Queue));
    queueInsert(queue, &nodes[start]);

    while (queue->head != NULL)
    {
        Node *prevNode = queueGet(queue);
        if (prevNode->previous == NULL)
            prevNode->distance = 0;

        for (Edge *edge = prevNode->edgeHead; edge != NULL; edge = edge->next)
        {
            Node *node = edge->to;
            if (node->distance == infinity)
            {
                node->distance = prevNode->distance + 1;
                node->previous = prevNode;
                queueInsert(queue, edge->to);
            }
        }
    }

    free(queue);

    displayBFS(nodes, n, start);
}

void displayTopo(Node *first, int n)
{
    // We tried traversing the nodes using only the next pointer in data,
    // but we had trouble using the void pointer data in Node as a struct

    // for (Node *node = first; node != NULL; node = node->data->next)
    // {
    //     printf("%li\n", node->nr);
    // }

    TopoList *data = first->data;

    printf("%li\n", first->nr);

    for (int i = 1; i < n; i++)
    {
        printf("%li\n", data->next->nr);
        data = data->next->data;
    }
}

Node *topoDF(Node *node, Node *list)
{
    TopoList *data = node->data;
    if (data->found)
        return list;
    data->found = true;

    for (Edge *edge = node->edgeHead; edge; edge = edge->next)
    {
        list = topoDF(edge->to, list);
    }
    data->next = list;
    return node;
}

void topo(char file[])
{
    Graph *graph = readGraphFile(file);
    Node *list = NULL;

    for (long i = 0; i < graph->n; i++)
    {
        graph->nodes[i].nr = i;
    }

    for (int i = graph->n; i--;)
    {
        graph->nodes[i].data = calloc(1, sizeof(TopoList));
    }
    for (int i = graph->n; i--;)
    {
        list = topoDF(&graph->nodes[i], list);
    }

    displayTopo(list, graph->n);
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
            topo(argv[2]);
        }
        // invalid first argument (neither 'bfs' nor 'top')
        else
        {
            printf("usage: %s <bfs|top> <file> [bfsStart]\n", argv[0]);
            return 1;
        }
    }
    else
    {
        printf("usage: %s <bfs|top> <file> [bfsStart]\n", argv[0]);
        return 1;
    }
}