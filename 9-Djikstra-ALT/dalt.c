#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define infinity 1000000000

// --- needed ---
// X heap priority queue - adjust max heap functions for min heap
// djikstra - bugged? too long distance
// ALT
// preprocessing landmark distances - compact binary file
//      metadata at start of file? (nr of landmarks & landmark ids)
//      x ints per node, use zero index as node nr
//      write preprocessed file
//      progress indicator (update every hundred/thousand nodes?)
// X read node & edge files - fscanf fast enough?
// write coordinates of routes as lat,long
// time djikstra & ALT
// CLI arguments for preprocessing, or hardcode and check if preprocessed file exists?
//      load preprocessed data, then spinwait for user input to check more than one route without restarting?
//      auto lookup node nr from provided name? allow either? djikn trondheim vs djik 1234

typedef struct NodeStruct
{
    int nr;
    int distance;
    bool checked;
    double lat;
    double lon;
    struct EdgeStruct *edgeHead;
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
    int start;
    int destination;
    int numNodes;
    Node **path;
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
    while (i && heap->nodes[i] < heap->nodes[f = heapOver(i)])
    {
        heapSwap(&heap->nodes[i], &heap->nodes[f]);
        i = f;
    }
}

void heapInsert(Heap *heap, int x)
{
    // printf("heapInsert x:%d heap->length: %i\n", x, heap->length);
    int i = heap->length;
    heap->nodes[i] = x;
    heapPrioUp(heap, i, 0);
    heap->length++;
}

void heapFix(Heap *heap, int i)
{
    int m = heapLeft(i);
    if (m < heap->length)
    {
        int h = m + 1;
        if (h < heap->length && heap->nodes[h] < heap->nodes[m])
            m = h;
        if (heap->nodes[m] < heap->nodes[i])
        {
            heapSwap(heap->nodes + i, heap->nodes + m);
            heapFix(heap, m);
        }
    }
}

// void heapCreate(Heap *heap)
// {
//     int i = heap->length / 2;
//     while (i--)
//         heapFix(heap, i);
// }

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

void resetNodes(Graph *graph, Route *route, int start)
{
    initDistances(graph, start);
    for (int i = 0; i < graph->n; i++)
    {
        graph->nodes[i].checked = false;
        graph->nodes[i].previous = NULL;
    }

    route->numNodes = 0;
    free(route->path);
    route->path = NULL;
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

    // read nodes
    for (int i = 0; i < graph->n; i++)
    {
        int nr;
        double lat, lon;
        fscanf(fpNodes, "%i %lf %lf\n", &nr, &lat, &lon);
        Node *node = &graph->nodes[nr];
        node->nr = nr;
        node->lat = lat;
        node->lon = lon;

        // graph->nodes[nr].nr = nr;
        // graph->nodes[nr].lat = lat;
        // graph->nodes[nr].lon = lon;
    }

    // read edges
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

Route *initRoute(int start, int destination)
{
    Route *route = calloc(1, sizeof(Route));
    route->start = start;
    route->destination = destination;
    return route;
}

Heap *initHeap(int n)
{
    Heap *heap = calloc(1, sizeof(Heap));
    heap->nodes = calloc(n, sizeof(int));
    return heap;
}

void printHeap(Heap *heap, int n)
{
    for (int i = 0; i < n; i++)
    {
        printf("%i ", heap->nodes[i]);
    }
    printf("\n");
}

void testHeap()
{
    int n = 7;
    int abc[] = {8, 52, 2, 3, 1, 8, 77};
    Heap *heap = initHeap(n);

    // insert test data
    for (int i = 0; i < n; i++)
    {
        // print heap contents on one line
        printf("HEAP INSERT:%i\n", abc[i]);
        printHeap(heap, n);

        heapInsert(heap, abc[i]);
        printf("%i i:%i heapLength: %i\n", abc[i], i, heap->length);

        printHeap(heap, n);
        printf("\n");
    }

    // retrieve test data in min sorted order (priority queue)
    while (heap->length > 0)
    {
        printf("HEAP GETMIN\n");
        printHeap(heap, n);

        int min = heapGetMin(heap);
        printf("%i heapLength: %i\n", min, heap->length);

        printHeap(heap, n);
        printf("\n");
    }
}

void printDrivingTime(int carTime)
{
    const int secondsInHour = 3600;
    int rawSeconds = carTime / 100;
    int h = rawSeconds / secondsInHour;
    int remainingSeconds = rawSeconds - (h * secondsInHour);
    int m = remainingSeconds / 60;
    int minutesAsSeconds = m * 60;
    int s = remainingSeconds - minutesAsSeconds;

    printf("%d:%02d:%02d", h, m, s);
}

void findPath(Graph *graph, Route *route)
{
    printf("\n---findPath---\n");
    int pathLength = 0;

    for (Node *node = &graph->nodes[route->destination];
         node != NULL; node = node->previous)
    {
        // printf("%i ", node->nr);
        pathLength++;
    }
    printf("pathLength: %i\n", pathLength);

    route->numNodes = pathLength;
    route->path = calloc(route->numNodes, sizeof(Node *));

    Node *node = &graph->nodes[route->destination];
    for (int i = route->numNodes - 1; i >= 0; i--)
    {
        route->path[i] = node;
        node = node->previous;
    }

    // path in correct order
    for (int i = 0; i < route->numNodes; i++)
    {
        // printf("%i ", route->path[i]->nr);
    }
    printf("\n");
}

void writePath(Route *route, char outFile[])
{
    FILE *fpOut = fopen(outFile, "w");
    if (fpOut == NULL)
    {
        perror("Error while opening outfile");
        exit(1);
    }

    char csvColumns[] = "nr,node,latitude,longitude\n";
    fwrite(csvColumns, strlen(csvColumns), 1, fpOut);
    // up to 3 chars for negative, 1 dot, 7 decimals, and 1 string termination
    const int coordLength = 12;

    for (int i = 0; i < route->numNodes; i++)
    {
        char pathNr[10];
        char nodeNr[30];
        char lat[coordLength], lon[coordLength];
        snprintf(pathNr, 10, "%i", i + 1);
        snprintf(nodeNr, 30, "%i", route->path[i]->nr);
        snprintf(lat, coordLength, "%.8f", route->path[i]->lat);
        snprintf(lon, coordLength, "%.8f", route->path[i]->lon);
        // printf("node: %i lat: %s lon: %s\n", route->path[i]->nr, lat, lon);

        fwrite(pathNr, sizeof(char), strlen(pathNr), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(nodeNr, sizeof(char), strlen(nodeNr), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lat, sizeof(char), strlen(lat), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lon, sizeof(char), strlen(lat), fpOut);
        fwrite("\n", sizeof(char), 1, fpOut);
    }
}

void djikstra(Graph *graph, Route *route, bool stopEarly)
{
    printf("\n--- djikstra from: %i to: %i---\n", route->start, route->destination);
    Heap *heap = initHeap(graph->n);
    heapInsert(heap, route->start);
    int checked = 0;

    while (heap->length > 0)
    {
        int nodeNr = heapGetMin(heap);
        Node *node = &graph->nodes[nodeNr];
        node->checked = true;
        checked++;

        if (node->nr == route->start)
        {
            printf("start\n");
        }

        // printf("nodeNr: %i node.nr: %i lat: %i lon: %i dist: %i\n",
        //        nodeNr, node.nr, node.lat, node.lon, node.distance);

        if (stopEarly && node->nr == route->destination)
        {
            printf("found target: %i dist: %i carTime (h:m:s) ",
                   node->nr, node->distance);
            printDrivingTime(node->distance);
            printf("\n");
            break;
        }

        int edges = 0;

        // check all neighbors and update distances
        for (Edge *edge = graph->nodes[nodeNr].edgeHead; edge != NULL; edge = edge->next)
        {
            edges++;
            if (edge->to->nr == route->start)
            {
                printf("back at start, checked: %i, edges: %i\n", checked, edges);
            }

            Node *neighbor = edge->to;
            int newDistance = node->distance + edge->weight;

            if (!neighbor->checked && newDistance < neighbor->distance)
            {
                neighbor->distance = newDistance;
                neighbor->previous = node;
                heapInsert(heap, edge->to->nr);
            }
        }
    }
    printf("--- djikstra done, checked %i nodes\n", checked);
    free(heap);
}

void preProcess()
{
}

void test(char nodeFile[], char edgeFile[], int from, int to)
{
    printf("---test--- nodeFile: %s edgeFile: %s\n", nodeFile, edgeFile);
    Graph *graph = readGraph(nodeFile, edgeFile, false);
    initDistances(graph, from);
    Route *route = initRoute(from, to);
    char outFile[] = "path.csv";

    // testHeap();

    // Node start = graph->nodes[from];
    // printf("\nstart:%i nr:%i dist:%i lat:%f lon:%f\n",
    //        from, start.nr, start.distance, start.lat, start.lon);

    // printf("--- first 5 nodes ---\n");
    // for (int i = 0; i < 5; i++)
    // {
    //     Node node = graph->nodes[i];
    //     printf("node:%i dist:%i lat:%f lon:%f\n",
    //            i, node.distance, node.lat, node.lon);
    // }
    // printf("--- last 5 nodes ---\n");
    // for (int i = graph->n - 5; i < graph->n; i++)
    // {
    //     Node node = graph->nodes[i];
    //     printf("node:%i dist:%i lat:%f lon:%f\n",
    //            i, node.distance, node.lat, node.lon);
    // }

    djikstra(graph, route, true);
    findPath(graph, route);
    writePath(route, outFile);

    if (strcmp(nodeFile, "ice/noder.txt") == 0)
    {
        resetNodes(graph, route, from);
        djikstra(graph, route, false);
    }
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
            int selfoss = 107046;
            test(nodeFile, edgeFile, reykjavik, selfoss);
            return 0;
        }
        if (strcmp(argv[1], "tr2") == 0)
        {
            char nodeFile[] = "../../../norden/noder.txt";
            char edgeFile[] = "../../../norden/kanter.txt";
            int meraaker = 6579983;
            int stjordal = 1693246;
            test(nodeFile, edgeFile, meraaker, stjordal);
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