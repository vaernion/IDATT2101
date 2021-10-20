#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define infinity 1000000000

typedef struct NodeStruct
{
    int nr;
    int distance;
    struct NodeStruct *previous;
    struct EdgeStruct *edgeHead;
    int isNotSource;
    int isNotDestination;
} Node;

typedef struct EdgeStruct
{
    Node *to;
    int capacity;
    int flow;
    struct EdgeStruct *reverse;
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
    int n;
    int k;
    Node *nodes;
    int source;
    int destination;
    int pathNodes;
} Graph;

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

void edgeListInsert(Node *nodes, int from, int to, int capacity)
{
    Edge *edge = calloc(1, sizeof(Edge));
    edge->to = &nodes[to];
    edge->next = nodes[from].edgeHead;
    edge->capacity = capacity;
    nodes[from].edgeHead = edge;

    Edge *reverse = calloc(1, sizeof(Edge));
    reverse->to = &nodes[from];
    reverse->next = nodes[to].edgeHead;
    nodes[to].edgeHead = reverse;

    reverse->reverse = edge;
    edge->reverse = reverse;

    // here we can easily mark that the nodes are normal nodes
    // the remaining nodes will be source and destination
    nodes[from].isNotDestination = 1;
    nodes[to].isNotSource = 1;
}

int restCapacity(Edge *edge)
{
    return edge->capacity - edge->flow;
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

    fscanf(fp, "%i %i\n", &graph->n, &graph->k);
    printf("n: %i k: %i\n", graph->n, graph->k);

    graph->nodes = calloc(graph->n, sizeof(Node));

    for (int i = 0; i < graph->k; i++)
    {
        int from, to, capacity;
        fscanf(fp, "%i %i %i\n", &from, &to, &capacity);
        edgeListInsert(graph->nodes, from, to, capacity);
    }

    fclose(fp);
    return graph;
}

void displayBFS(Graph *graph, int path[], int increase)
{
    printf("%-10i", increase);

    for (int i = 0; i < graph->pathNodes; i++)
    {
        printf("%i ", path[i]);
    }
    printf("\n");
}

// called between each iteration of edmon-karp to find the next path
void resetBFS(Graph *graph)
{
    for (int i = 0; i < graph->n; i++)
    {
        graph->nodes[i].distance = infinity;
        graph->nodes[i].previous = NULL;
    }
    graph->pathNodes = 0;
}

void findPathLength(Graph *graph)
{
    Node *node = &graph->nodes[graph->destination];
    graph->pathNodes = 1;

    while (node->previous)
    {
        graph->pathNodes += 1;
        node = node->previous;
    }
}

void findPath(Graph *graph, int path[])
{
    int cur = graph->pathNodes - 1;

    Node *node = &graph->nodes[graph->destination];

    while (node->previous)
    {
        path[cur] = node->nr;

        cur -= 1;
        node = node->previous;
    }
    path[0] = node->nr;
}

int findEdgeIncrease(Graph *graph, int path[])
{
    int increase = 0;
    for (int i = 0; i < graph->pathNodes; i++)
    {
        for (Edge *edge = graph->nodes[path[i]].edgeHead; edge; edge = edge->next)
        {
            if (edge->to->nr == path[i + 1])
            {
                int rest = restCapacity(edge);
                if (rest < increase || increase == 0)
                    increase = rest;
            }
        }
    }
    return increase;
}

void updatePathFlows(Graph *graph, int path[], int increase)
{
    for (int i = 0; i < graph->pathNodes; i++)
    {
        Node *node = &graph->nodes[path[i]];
        for (Edge *edge = node->edgeHead; edge; edge = edge->next)
        {
            if (edge->to->nr == path[i + 1])
            {
                edge->flow += increase;
                edge->reverse->flow -= increase;
            }
        }
    }
}

int bfs(Graph *graph)
{
    Queue *queue = calloc(1, sizeof(Queue));
    queueInsert(queue, &graph->nodes[graph->source]);

    while (queue->head != NULL)
    {
        Node *prevNode = queueGet(queue);
        if (prevNode->previous == NULL)
            prevNode->distance = 0;

        for (Edge *edge = prevNode->edgeHead; edge != NULL; edge = edge->next)
        {
            if (restCapacity(edge) <= 0)
                continue;

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

    findPathLength(graph);
    int path[graph->pathNodes];
    findPath(graph, path);
    int increase = findEdgeIncrease(graph, path);
    updatePathFlows(graph, path, increase);

    if (increase > 0)
        displayBFS(graph, path, increase);

    resetBFS(graph);

    return increase;
}

void edmond(char file[])
{
    Graph *graph = readGraphFile(file);
    int n = graph->n;
    Node *nodes = graph->nodes;

    // initialize distances and helper numbers
    //
    for (int i = 0; i < n; i++)
    {
        nodes[i].nr = i;
        nodes[i].distance = infinity;
        if (nodes[i].isNotSource == 0)
            graph->source = i;
        if (nodes[i].isNotDestination == 0)
            graph->destination = i;
    }

    printf("Max flow from %i to %i with Edmonds-Karp\n", graph->source, graph->destination);
    printf("%-9s Path\n", "Increase");

    int prevInc = infinity;
    int maxInc = 0;
    while (prevInc > 0)
    {
        prevInc = bfs(graph);
        maxInc += prevInc;
    };
    printf("Max flow is %i\n", maxInc);
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        edmond(argv[1]);
    }
    else
    {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }
}