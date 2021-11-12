#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define infinity 1000000000

// --- needed ---
// heap priority queue - adjust max heap functions for min heap
// djikstra
// ALT
// preprocessing landmark distances - compact binary file
//      metadata at start of file? (nr of landmarks & landmark ids)
//      x ints per node, use zero index as node nr
//      write preprocessed file
//      progress indicator (update every hundred/thousand nodes?)
// read node & edge files - fscanf fast enough?
// write coordinates of routes as lat,long
// time djikstra & ALT
// CLI arguments for preprocessing, or hardcode and check if preprocessed file exists?
//      load preprocessed data, then spinwait for user input to check more than one route without restarting?
//      auto lookup node nr from provided name? allow either? djikn trondheim vs djik 1234

typedef struct NodeStruct
{
    struct EdgeStruct *edgeHead;
    int distance;
    double lat;
    double lon;
    struct NodeStruct *previous;
} Node;

typedef struct EdgeStruct
{
    Node *to;
    struct EdgeStruct *next;
    int weight;
} Edge;

typedef struct GraphStruct
{
    int n;
    int k;
    Node *nodes;
} Graph;

typedef struct RouteStruct
{
    int source;
    int destination;
    int i;
    Node *path;
} Route;

typedef struct HeapStruct
{
    int length;
    int *nodes;
} Heap;

void heapSwap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

int heapOver(int i)
{
    return (i - 1) >> 1;
}
int heapLeft(int i)
{
    return (i << 1) + 1;
}
int heapRight(int i)
{
    return (i + 1) << 1;
}

void heapPrioUp(Heap *heap, int i, int p)
{
    int f;
    heap->nodes[i] += p;
    while (i && heap->nodes[i] > heap->nodes[f = heapOver(i)])
    {
        heapSwap(&heap->nodes[i], &heap->nodes[f]);
        i = f;
    }
}

void heapInsert(Heap *heap, int x)
{
    heap->length++;
    printf("Starting heapInsert x:%d\n", x);
    // if (heap->nodes == NULL)
    // {
    //     heap->nodes = malloc(heap->length * sizeof(int));
    //     printf("malloc'd\n");
    // }
    // else
    // {
    //     heap->nodes = realloc(heap->nodes, heap->length * sizeof(int));
    //     printf("realloc'd length: %d\n", heap->length);
    // }

    int i = heap->length;
    heap->nodes[i] = x;
    heapPrioUp(heap, i, 0);
}

void heapFix(Heap *heap, int i)
{
    int m = heapLeft(i);
    if (m < heap->length)
    {
        int h = m + 1;
        if (h < heap->length && heap->nodes[h] > heap->nodes[m])
            m = h;
        if (heap->nodes[m] > heap->nodes[i])
        {
            heapSwap(heap->nodes + i, heap->nodes + m);
            heapFix(heap, m);
        }
    }
}

void heapCreate(Heap *heap)
{
    int i = heap->length / 2;
    while (i--)
        heapFix(heap, i);
}

int heapGetMin(Heap *heap)
{
    int min = heap->nodes[0];
    heap->nodes[0] = heap->nodes[--heap->length];
    heapFix(heap, 0);
    return min;
}

void edgeListInsert(Node *nodes, int from, int to, int weight)
{
    Edge *edge = calloc(1, sizeof(Edge));
    edge->to = &nodes[to];
    edge->next = nodes[from].edgeHead;
    edge->weight = weight;
    nodes[from].edgeHead = edge;
}

void initDistances(Graph *graph, int start)
{
    for (int i = 0; i < graph->n; i++)
    {
        graph->nodes[i].distance = infinity;
    }
    graph->nodes[start].distance = 0;
}

Graph *readGraph(char nodeFile[], char edgeFile[], bool reverseGraph)
{
    FILE *fpNodes = fopen(nodeFile, "r");
    if (fpNodes == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    FILE *fpEdges = fopen(edgeFile, "r");
    if (fpEdges == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    Graph *graph = malloc(sizeof(Graph));

    fscanf(fpNodes, "%i\n", &graph->n);
    fscanf(fpEdges, "%i\n", &graph->k);
    printf("n: %i k: %i\n", graph->n, graph->k);

    graph->nodes = calloc(graph->n, sizeof(Node));

    for (int i = 0; i < graph->k; i++)
    {
        int nr;
        double lat, lon;
        fscanf(fpNodes, "%i %lf %lf\n", &nr, &lat, &lon);
        graph->nodes[nr].lat = lat;
        graph->nodes[nr].lon = lon;
    }

    for (int i = 0; i < graph->k; i++)
    {
        int from, to, carTime, length, speedLimit;

        fscanf(fpEdges, "%i %i %i %i %i\n",
               &from, &to, &carTime, &length, &speedLimit);

        int weight = carTime; // can chance to length

        if (reverseGraph)
            edgeListInsert(graph->nodes, to, from, weight);
        else
            edgeListInsert(graph->nodes, from, to, weight);
    }

    fclose(fpNodes);
    fclose(fpEdges);
    return graph;
}

void djikstra(Graph *graph, Route *route, bool stopEarly)
{
}

void preProcess()
{
}

void test(char nodeFile[], char edgeFile[], int from, int to)
{
    Graph *graph = readGraph(nodeFile, edgeFile, false);
    initDistances(graph, from);

    int n = 7;
    int abc[] = {82, 21, 35, 11, 59, 111, 62};

    Heap *heap = calloc(1, sizeof(Heap));
    heap->nodes = calloc(graph->n, sizeof(int));
    // heap->length = n;
    for (int i = 0; i < n; i++)
    {
        // heap->nodes[i] = abc[i];
        heapInsert(heap, abc[i]);
        printf("inserted %i\n", i + 1);
    }

    for (int i = 0; i < n; i++)
    {
        // heap->nodes[i] = abc[i];
        int min = heapGetMin(heap);
        printf("heapLength: %i i:%i %d\n", heap->length, i, min);
    }

    Node start = graph->nodes[from];
    printf("\nstart:%i dist:%i lat:%f lon:%f\n",
           from, start.distance, start.lat, start.lon);

    printf("--- first 5 nodes ---\n");
    for (int i = 0; i < 5; i++)
    {
        Node node = graph->nodes[i];
        printf("node:%i dist:%i lat:%f lon:%f\n",
               i, node.distance, node.lat, node.lon);
    }
    printf("--- last 5 nodes ---\n");
    for (int i = graph->n - 5; i < graph->n; i++)
    {
        Node node = graph->nodes[i];
        printf("node:%i dist:%i lat:%f lon:%f\n",
               i, node.distance, node.lat, node.lon);
    }

    heapCreate(heap);
}

void routeTerminal(char preFile[])
{
    printf("pre file: %s\n", preFile);

    char input[12];
    char algorithm[5];
    int from = 0;
    int to = 0;
    char outFile[100];
    printf("djik <from> <to> [file]:\n");

    while (fgets(input, 12, stdin))
    {
        sscanf(input, "%5s %d %d %5s", algorithm, &from, &to, outFile);
        printf("alg: %s from: %d to: %d outfile: %s", algorithm, from, to, outFile);
        if (strncmp(input, "a", 1) == 0)
        {
            sleep(2);
        }
        sleep(1);

        printf("\r\33[2K"); // VT100 clear line escape code
        printf("\rdjik <from> <to> [file]:\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc > 3)
    {
        if (strcmp(argv[1], "route") == 0)
        {
            routeTerminal(argv[2]);
            return 0;
        }
        if (strcmp(argv[1], "pre") == 0)
        {
            return 0;
        }
    }
    if (argc > 1)
    {
        if (strcmp(argv[1], "tr") == 0)
        {
            char nodeFile[] = "ice/noder.txt";
            char edgeFile[] = "ice/kanter.txt";
            int reykjavik = 30979;
            int skogar = 12220;
            test(nodeFile, edgeFile, reykjavik, skogar);
            return 0;
        }
        if (strcmp(argv[1], "tp") == 0)
        {
            // char nodeFile[] = "ice/noder.txt";
            // char edgeFile[] = "ice/kanter.txt";

            return 0;
        }
    }

    printf("usage: %s route|pre file\n"
           "pre-processing: %s pre <nodes> <edges> <out> 123 213 124\n"
           "routing: %s route <nodes> <edges> <pre>\n"
           "\tafter loading:\n"
           "\tdjik|alt <from> <to> [file]\n"
           "\tfuel|charger <node> (find 10 closest gas stations or chargers)\n"
           "A path for the preprocessed file is required when preprocessing or looking for routes.\n"
           "Routes can optionally be written to a file as lines of <lat>,<long>\n",
           argv[0], argv[0], argv[0]);
    printf("invisible lorem ipsum spaaaaaaaaaaaam spaaaaaaaaaaaam");
    printf("\33[2K"); // VT100 clear line escape code
    printf("\roverwrites previous\n");

    return 1;
}