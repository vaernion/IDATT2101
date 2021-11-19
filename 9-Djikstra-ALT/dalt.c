#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define infinity 1000000000

enum
{
    MODE_FUEL = 2,
    MODE_CHARGER = 4,
    MODE_ALT = 9
};

// --- needed ---
// X heap priority queue - adjust max heap functions for min heap
// X djikstra
// ALT
// preprocessing landmark distances - compact binary file
//      metadata at start of file? (nr of landmarks & landmark ids)
//      x ints per node, use zero index as node nr
//      write preprocessed file
//      progress indicator (update every thousand/10k nodes?)
// X read node & edge files - fscanf fast enough?
// X write coordinates of routes as lat,long
// X time djikstra & ALT
// CLI arguments for preprocessing, or hardcode and check if preprocessed file exists?
//      load preprocessed data, then spinwait for user input to check more than one route without restarting?
//      auto lookup node nr from provided name? allow either? djikn trondheim vs djik 1234

typedef struct NodeStruct
{
    int nr;
    char mode;  // byte for space efficiency
    char *name; // most nodes don't have a name, should not be too expensive
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
    int numNames;
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

void heapPrioUp(Heap *heap, int i, Node *nodes)
{
    int f;
    while (i &&
           nodes[heap->nodes[i]].distance <
               nodes[heap->nodes[f = heapOver(i)]].distance)
    {
        heapSwap(&heap->nodes[i], &heap->nodes[f]);
        i = f;
    }
}

void heapInsert(Heap *heap, int x, Node *nodes)
{
    // printf("heapInsert x:%d heap->length: %i\n", x, heap->length);
    int i = heap->length;
    heap->nodes[i] = x;
    heapPrioUp(heap, i, nodes);
    heap->length++;
}

void heapFix(Heap *heap, int i, Node *nodes)
{
    int m = heapLeft(i);
    if (m < heap->length)
    {
        int h = m + 1;
        int hDistance = nodes[heap->nodes[h]].distance;
        int mDistance = nodes[heap->nodes[m]].distance;
        if (h < heap->length && hDistance < mDistance)
        {
            m = h;
            mDistance = nodes[heap->nodes[m]].distance;
        }
        int iDistance = nodes[heap->nodes[i]].distance;
        if (mDistance < iDistance)
        {
            heapSwap(&heap->nodes[i], &heap->nodes[m]);
            heapFix(heap, m, nodes);
        }
    }
}

int heapGetMin(Heap *heap, Node *nodes)
{
    int min = heap->nodes[0];
    heap->nodes[0] = heap->nodes[--heap->length];
    heapFix(heap, 0, nodes);
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

Graph *readGraph(char nodeFile[], char edgeFile[], char poiFile[], bool reverseGraph)
{
    float startTime = (float)clock() / CLOCKS_PER_SEC;

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

    FILE *fpPOI = fopen(poiFile, "r");
    if (fpPOI == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    Graph *graph = malloc(sizeof(Graph));

    fscanf(fpNodes, "%i\n", &graph->n);
    fscanf(fpEdges, "%i\n", &graph->k);
    fscanf(fpPOI, "%i\n", &graph->numNames);
    printf("n: %i k: %i names: %i\n", graph->n, graph->k, graph->numNames);

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

        int weight = carTime; // can change to length to get meters

        if (reverseGraph)
            edgeListInsert(graph->nodes, to, from, weight);
        else
            edgeListInsert(graph->nodes, from, to, weight);
    }

    // read names and fuel/charger (mode)
    for (int i = 0; i < graph->numNames; i++)
    {
        int nr;
        int mode;
        char name[255] = {0};
        fscanf(fpPOI, "%i %i ", &nr, &mode);

        // workaround to handle quoted node names
        char c;
        int namePos = 0;
        while ((c = fgetc(fpPOI)) != EOF)
        {
            if (c == '\n')
                break;

            if (c != '"')
            {
                name[namePos] = c;
                namePos++;
            }
        }
        int nameLength = strlen(name);

        // printf("name:%s nameLength: %i sizeof: %i\n",
        //        name, nameLength, sizeof(name));

        Node *node = &graph->nodes[nr];
        node->mode = (char)mode;
        node->name = calloc(nameLength + 1, sizeof(char));
        strncpy(node->name, name, nameLength);
    }

    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = endTime - startTime;
    printf("loaded graph in %.2fs\n", timeElapsed);

    fclose(fpNodes);
    fclose(fpEdges);
    fclose(fpPOI);
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

// void printHeap(Heap *heap, int n)
// {
//     for (int i = 0; i < n; i++)
//     {
//         printf("%i ", heap->nodes[i]);
//     }
//     printf("\n");
// }

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
    int pathLength = 0;

    for (Node *node = &graph->nodes[route->destination];
         node != NULL; node = node->previous)
    {
        pathLength++;
    }

    route->numNodes = pathLength;
    route->path = calloc(route->numNodes, sizeof(Node *));

    Node *node = &graph->nodes[route->destination];
    for (int i = route->numNodes - 1; i >= 0; i--)
    {
        route->path[i] = node;
        node = node->previous;
    }
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
        char pathNr[10] = {0};
        char nodeNr[30] = {0};
        char lat[coordLength];
        char lon[coordLength];
        memset(lat, 0, coordLength);
        memset(lon, 0, coordLength);
        snprintf(pathNr, 10, "%i", i + 1);
        snprintf(nodeNr, 30, "%i", route->path[i]->nr);
        sprintf(lat, "%.7f", route->path[i]->lat);
        sprintf(lon, "%.7f", route->path[i]->lon);
        // printf("node: %i lat: %s lon: %s\n", route->path[i]->nr, lat, lon);

        fwrite(pathNr, sizeof(char), strlen(pathNr), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(nodeNr, sizeof(char), strlen(nodeNr), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lat, sizeof(char), strlen(lat), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lon, sizeof(char), strlen(lon), fpOut);
        fwrite("\n", sizeof(char), 1, fpOut);
    }
    printf("coordinates written to %s\n", outFile);
}

// modes --- default: djikstra, 2: fuel 4: chargers, 9: ALT
void djikstra(Graph *graph, Route *route, bool stopEarly, char mode, int stations[], int n)
{
    float startTime = (float)clock() / CLOCKS_PER_SEC;

    printf("\n--- %s from: %s (%i) to: %s (%i) ---\n",
           mode == MODE_ALT ? "ALT" : "Djikstra",
           graph->nodes[route->start].name,
           route->start,
           route->destination < 0 ? "ALL" : graph->nodes[route->destination].name,
           route->destination);

    Heap *heap = initHeap(graph->n);
    heapInsert(heap, route->start, graph->nodes);
    int checked = 0;
    int duplicateNodes = 0;
    int stationsFound = 0;

    while (heap->length > 0)
    {
        int nodeNr = heapGetMin(heap, graph->nodes);
        Node *node = &graph->nodes[nodeNr];

        // workaround instead of re-prioritizing queue
        if (node->checked)
        {
            duplicateNodes++;
            continue;
        }

        node->checked = true;
        checked++;

        if ((mode == MODE_FUEL || mode == MODE_CHARGER) &&
            node->mode == mode && stationsFound < n)
        {
            stations[stationsFound++] = node->nr;
            if (stationsFound == n)
            {
                printf("found %i %s\n", n, mode == MODE_FUEL ? "gas stations" : "chargers");
                break;
            }
        }

        // printf("nodeNr: %i node.nr: %i lat: %i lon: %i dist: %i\n",
        //        nodeNr, node.nr, node.lat, node.lon, node.distance);

        if (stopEarly && node->nr == route->destination)
        {
            printf("distance: %i time: ", node->distance);
            printDrivingTime(node->distance);
            printf(" ");
            findPath(graph, route);
            printf("nodes: %i\n", route->numNodes);
            break;
        }

        // check all neighbors and update distances
        for (Edge *edge = graph->nodes[nodeNr].edgeHead; edge != NULL; edge = edge->next)
        {
            Node *neighbor = edge->to;
            int newDistance = node->distance + edge->weight;

            if (!neighbor->checked && newDistance < neighbor->distance)
            {
                neighbor->distance = newDistance;
                neighbor->previous = node;
                heapInsert(heap, edge->to->nr, graph->nodes);
            }
        }
    }

    free(heap);

    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = endTime - startTime;
    printf("--- %s done in %.2fs, checked:%i duplicates:%i ---\n",
           mode == MODE_ALT ? "ALT" : "Djikstra", timeElapsed, checked, duplicateNodes);
}

void preProcess()
{
}

void writeStations(Graph *graph, char mode, int stations[], int n, char outFile[])
{
    FILE *fpOut = fopen(outFile, "w");
    if (fpOut == NULL)
    {
        perror("Error while opening outfile");
        exit(1);
    }

    char csvColumns[] = "mode,node,latitude,longitude\n";
    fwrite(csvColumns, strlen(csvColumns), 1, fpOut);
    // up to 3 chars for negative, 1 dot, 7 decimals, and 1 string termination
    const int coordLength = 12;

    char modeChar = '0';
    if (mode == MODE_FUEL)
    {
        modeChar = '2';
    }
    else if (mode == MODE_CHARGER)
    {
        modeChar = '4';
    }

    for (int i = 0; i < n; i++)
    {
        char nodeNr[30];
        char lat[coordLength], lon[coordLength];
        snprintf(nodeNr, 30, "%i", graph->nodes[stations[i]].nr);
        snprintf(lat, coordLength, "%.8f", graph->nodes[stations[i]].lat);
        snprintf(lon, coordLength, "%.8f", graph->nodes[stations[i]].lon);
        // printf("node: %i lat: %s lon: %s\n", route->path[i]->nr, lat, lon);

        fwrite(&modeChar, sizeof(char), 1, fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(nodeNr, sizeof(char), strlen(nodeNr), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lat, sizeof(char), strlen(lat), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lon, sizeof(char), strlen(lat), fpOut);
        fwrite("\n", sizeof(char), 1, fpOut);
    }
    printf("coordinates written to %s\n", outFile);
}

void findStations(Graph *graph, Route *route, char outFile[], char mode, int n)
{
    int stations[n];
    for (int i = 0; i < n; i++)
    {
        stations[i] = 0;
    }
    djikstra(graph, route, false, mode, stations, n);
    writeStations(graph, mode, stations, n, outFile);
}

void testFindStations(char nodeFile[], char edgeFile[], char poiFile[],
                      char mode, int from)
{
    printf("---testFindStations---\n nodes:%s edges:%s pois:%s\n", nodeFile, edgeFile, poiFile);
    Graph *graph = readGraph(nodeFile, edgeFile, poiFile, false);
    initDistances(graph, from);
    Route *route = initRoute(from, -1);
    char outFile[] = "stations.csv";

    findStations(graph, route, outFile, mode, 10);
    exit(0);
}

void test(char nodeFile[], char edgeFile[], char poiFile[], int from, int to)
{
    printf("---test---\n nodes:%s edges:%s pois:%s\n", nodeFile, edgeFile, poiFile);
    Graph *graph = readGraph(nodeFile, edgeFile, poiFile, false);
    initDistances(graph, from);
    Route *route = initRoute(from, to);
    char outFile[] = "path.csv";

    djikstra(graph, route, true, 1, NULL, 0);
    if (!(route->destination < 0))
    {
        writePath(route, outFile);
    }
    exit(0);
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
    // for testing purposes
    else if (argc > 1)
    {
        char iceNode[] = "ice/noder.txt";
        char iceEdge[] = "ice/kanter.txt";
        char icePoi[] = "ice/interessepkt.txt";
        char norNode[] = "norden/noder.txt";
        char norEdge[] = "norden/kanter.txt";
        char norPoi[] = "norden/interessepkt.txt";

        int reykjavik = 30979;
        int selfoss = 107046;

        int trondheim = 6861306;
        int oslo = 2518118;
        int stockholm = 6487468;
        int meraaker = 6579983;
        int stjordal = 1693246;
        int stavanger = 3447384;
        int tampere = 136963;
        int kaarvaag = 6368906;
        int gjemnes = 6789998;

        if (strcmp(argv[1], "tr0") == 0)
            test(iceNode, iceEdge, icePoi, reykjavik, reykjavik);
        if (strcmp(argv[1], "tr1") == 0)
            test(iceNode, iceEdge, icePoi, reykjavik, selfoss);
        if (strcmp(argv[1], "tr2") == 0)
            test(norNode, norEdge, norPoi, meraaker, stjordal);
        if (strcmp(argv[1], "tr3") == 0)
            test(norNode, norEdge, norPoi, trondheim, oslo);
        if (strcmp(argv[1], "tr4") == 0)
            test(norNode, norEdge, norPoi, oslo, stockholm);
        if (strcmp(argv[1], "tr5") == 0)
            test(norNode, norEdge, norPoi, stavanger, tampere);
        if (strcmp(argv[1], "tr6") == 0)
            test(norNode, norEdge, norPoi, kaarvaag, gjemnes);
        if (strcmp(argv[1], "tr7") == 0)
            test(norNode, norEdge, norPoi, tampere, trondheim);
        if (strcmp(argv[1], "tpre1") == 0)
            test(iceNode, iceEdge, icePoi, reykjavik, -1);
        if (strcmp(argv[1], "tpre2") == 0)
            test(norNode, norEdge, norPoi, trondheim, -1);
        if (strcmp(argv[1], "tfuel1") == 0)
            testFindStations(iceNode, iceEdge, icePoi, MODE_FUEL, reykjavik);
        if (strcmp(argv[1], "tfuel2") == 0)
            testFindStations(norNode, norEdge, norPoi, MODE_FUEL, trondheim);
        if (strcmp(argv[1], "tcharger1") == 0)
            testFindStations(iceNode, iceEdge, icePoi, MODE_CHARGER, reykjavik);
        if (strcmp(argv[1], "tcharger2") == 0)
            testFindStations(norNode, norEdge, norPoi, MODE_CHARGER, trondheim);
    }

    printf("usage: %s route|pre\n"
           "pre-process ALT: %s pre <nodes> <edges> <out> <landmark> [landmark2..]\n"
           "routing: %s route <nodes> <edges> <poi> <pre>\n"
           "\tafter loading:\n"
           "\tdjik|alt <from> <to> [file]\n"
           "\t\tfind shortest path with Djikstra or ALT\n"
           "\tfuel|charger <node> <n> <file>\n"
           "\t\tfind n closest gas stations or chargers\n"
           "Routes can optionally be written as CSV of nr,node,lat,long\n",
           argv[0], argv[0], argv[0]);

    printf("invisible lorem ipsum spaaaaaaaaaaaam spaaaaaaaaaaaam");
    printf("\33[2K"); // VT100 clear line escape code
    printf("\roverwrites previous\n");

    return 1;
}