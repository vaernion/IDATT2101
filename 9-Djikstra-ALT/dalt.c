#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define infinity 1000000000

enum
{
    MODE_DJIKSTRA = 0,
    MODE_FUEL = 2,
    MODE_CHARGER = 4,
    MODE_ALT = 9
};

typedef struct NodeStruct
{
    int nr;
    char mode;  // 1 byte for space efficiency
    char *name; // most nodes don't have a name, should not be too expensive
    int weight;
    int startDist;
    int estimateToGoal;
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
    int m;
    int *fromMarks; // used as 2d array
    int *toMarks;   // used as 2d array
} Graph;

typedef struct RouteStruct
{
    int start;
    int destination;
    int numNodes;
    Node **path; // pointers to graph->nodes[i]
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
           nodes[heap->nodes[i]].weight <
               nodes[heap->nodes[f = heapOver(i)]].weight)
    {
        heapSwap(&heap->nodes[i], &heap->nodes[f]);
        i = f;
    }
}

void heapInsert(Heap *heap, int x, Node *nodes)
{
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
        int hDistance = nodes[heap->nodes[h]].weight;
        int mDistance = nodes[heap->nodes[m]].weight;
        if (h < heap->length && hDistance < mDistance)
        {
            m = h;
            mDistance = nodes[heap->nodes[m]].weight;
        }
        int iDistance = nodes[heap->nodes[i]].weight;
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

void initNodeDistances(Graph *graph, int start)
{
    for (int i = 0; i < graph->n; i++)
    {
        graph->nodes[i].weight = infinity;
        graph->nodes[i].startDist = infinity;
    }
    graph->nodes[start].weight = 0;
    graph->nodes[start].startDist = 0;
}

void resetNodes(Graph *graph, Route *route, int start)
{
    initNodeDistances(graph, start);
    for (int i = 0; i < graph->n; i++)
    {
        graph->nodes[i].checked = false;
        graph->nodes[i].previous = NULL;
        graph->nodes[i].estimateToGoal = 0;
    }

    if (route->path != NULL)
    {
        free(route->path);
    }
    route->numNodes = 0;
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
    printf("n: %i k: %i names: %i\nloading graph...",
           graph->n, graph->k, graph->numNames);
    fflush(stdout);

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

        Node *node = &graph->nodes[nr];
        node->mode = (char)mode;
        node->name = calloc(nameLength + 1, sizeof(char));
        strncpy(node->name, name, nameLength);
    }

    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = endTime - startTime;
    printf("\r\33[2K"); // VT100 clear line escape code
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
        snprintf(lat, coordLength, "%.7f", route->path[i]->lat);
        snprintf(lon, coordLength, "%.7f", route->path[i]->lon);
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

int estimateALT(Graph *graph, int goal, int node)
{
    int estimate = 0;

    for (int i = 0; i < graph->m; i++)
    {
        int distanceBehind =
            (graph->fromMarks + goal * graph->m)[i] -
            (graph->fromMarks + node * graph->m)[i];
        if (distanceBehind > estimate)
            estimate = distanceBehind;

        int distanceAfter =
            (graph->toMarks + node * graph->m)[i] -
            (graph->toMarks + goal * graph->m)[i];
        if (distanceAfter > estimate)
            estimate = distanceAfter;
    }

    return estimate;
}

// uses Djikstra or ALT (A*, Landmarks, Triangle inequality)
// to find the shortest path
// to a destination, all other nodes or the closest gas stations/chargers
// modes --- 0: djikstra, 2: fuel 4: chargers, 9: ALT
// route->destination should be < 0 when checking all nodes (stopEarly = false)
void djikstra(Graph *graph, Route *route,
              bool stopEarly, char mode, int stations[], int stationsN)
{
    float startTime = (float)clock() / CLOCKS_PER_SEC;

    printf("\n%s from: %s (%i) to: %s (%i)\n",
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

        // workaround instead of re-prioritizing queue for updated distances
        // typically ~5% wasted heap insertions
        if (node->checked)
        {
            duplicateNodes++;
            continue;
        }

        node->checked = true;
        checked++;

        // handle gas stations/chargers
        if ((mode == MODE_FUEL || mode == MODE_CHARGER) &&
            node->mode == mode && stationsFound < stationsN)
        {
            stations[stationsFound++] = node->nr;
            if (stationsFound == stationsN)
            {
                printf("found %i %s\n",
                       stationsN, mode == MODE_FUEL ? "gas stations" : "chargers");
                break;
            }
        }

        // found destination
        if (stopEarly && node->nr == route->destination)
        {
            printf("distance: %i time: ", node->startDist);
            printDrivingTime(node->startDist);
            printf(" ");
            findPath(graph, route);
            printf("nodes: %i\n", route->numNodes);
            break;
        }

        // check all neighbors and update distances
        for (Edge *edge = graph->nodes[nodeNr].edgeHead; edge != NULL; edge = edge->next)
        {
            Node *neighbor = edge->to;
            int newDist = node->startDist + edge->weight;

            int estimate = 0;
            if (mode == MODE_ALT)
            {
                if (neighbor->estimateToGoal == 0)
                    neighbor->estimateToGoal = estimateALT(graph, route->destination, neighbor->nr);
                estimate = neighbor->estimateToGoal;
            }

            if (!neighbor->checked &&
                newDist < neighbor->startDist)
            {
                neighbor->weight = newDist + estimate;
                neighbor->startDist = newDist;
                neighbor->previous = node;
                heapInsert(heap, neighbor->nr, graph->nodes);
            }
        }
    }

    free(heap);

    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = endTime - startTime;
    printf("%s done in %.2fs, checked:%i duplicates:%i\n",
           mode == MODE_ALT ? "ALT" : "Djikstra", timeElapsed, checked, duplicateNodes);
}

// preProcess(norNode, norEdge, norPoi, norPre, landmarks, m);
void preProcess(char nodeFile[], char edgeFile[], char poiFile[],
                char outFile[], int landmarks[], int m)
{
    printf("preprocessing %i landmarks\n", m);
    float startTime = (float)clock() / CLOCKS_PER_SEC;

    Graph *graph = readGraph(nodeFile, edgeFile, poiFile, false);
    Graph *graphRev = readGraph(nodeFile, edgeFile, poiFile, true);

    int *fromMarks = calloc(m * graph->n, sizeof(int));
    int *toMarks = calloc(m * graphRev->n, sizeof(int));

    for (int i = 0; i < m; i++)
    {
        int landmark = landmarks[i];
        printf("\nprocessing landmark %s (%i)",
               graph->nodes[landmark].name, landmark);
        Route *route = initRoute(landmark, -1);

        resetNodes(graph, route, landmark);
        djikstra(graph, route, false, MODE_DJIKSTRA, NULL, 0);

        resetNodes(graphRev, route, landmark);
        djikstra(graphRev, route, false, MODE_DJIKSTRA, NULL, 0);

        for (int j = 0; j < graph->n; j++)
        {
            *(fromMarks + j * m + i) = graph->nodes[j].weight;
            *(toMarks + j * m + i) = graphRev->nodes[j].weight;
        }
    }

    FILE *fpOut = fopen(outFile, "wb");
    if (fpOut == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    // m landmarks, landmark ids,
    // m*n ints (from node n to landmark n1,n2...), m*n ints (to node...)
    fwrite(&m, sizeof(m), 1, fpOut);
    fwrite(landmarks, sizeof(int), m, fpOut);
    fwrite(fromMarks, sizeof(int), m * graph->n, fpOut);
    fwrite(toMarks, sizeof(int), m * graph->n, fpOut);

    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = endTime - startTime;
    printf("preprocessed %i landmarks for %i nodes in %.2fs\n",
           m, graph->n, timeElapsed);
    exit(0);
}

void loadPreProcess(Graph *graph, char poiFile[])
{
    printf("loading preprocessed landmarks from %s\n", poiFile);
    float startTime = (float)clock() / CLOCKS_PER_SEC;

    FILE *fp = fopen(poiFile, "rb");
    if (fp == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    int m;
    fread(&m, sizeof(int), 1, fp);
    int *landmarks = calloc(m, sizeof(int));
    fread(landmarks, sizeof(int), m, fp);
    int *fromMarks = calloc(m * graph->n, sizeof(int));
    int *toMarks = calloc(m * graph->n, sizeof(int));
    fread(fromMarks, sizeof(int), m * graph->n, fp);
    fread(toMarks, sizeof(int), m * graph->n, fp);

    graph->m = m;
    graph->fromMarks = fromMarks;
    graph->toMarks = toMarks;

    float endTime = (float)clock() / CLOCKS_PER_SEC;
    float timeElapsed = endTime - startTime;
    printf("loaded %i landmarks for %i nodes in %.2fs\n",
           m, graph->n, timeElapsed);
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
    initNodeDistances(graph, from);
    Route *route = initRoute(from, -1);
    char outFile[] = "stations.csv";

    findStations(graph, route, outFile, mode, 10);
    exit(0);
}

void writePathAlt(Graph *graph, char outFile[])
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

    for (int i = 0; i < graph->n; i++)
    {
        if (!graph->nodes[i].checked)
            continue;

        if (i % 100 != 0)
            continue;

        char pathNr[10] = {0};
        char nodeNr[30] = {0};
        char lat[coordLength];
        char lon[coordLength];
        memset(lat, 0, coordLength);
        memset(lon, 0, coordLength);
        snprintf(pathNr, 10, "%i", i + 1);
        snprintf(nodeNr, 30, "%i", graph->nodes[i].nr);
        snprintf(lat, coordLength, "%.7f", graph->nodes[i].lat);
        snprintf(lon, coordLength, "%.7f", graph->nodes[i].lon);

        fwrite(pathNr, sizeof(char), strlen(pathNr), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(nodeNr, sizeof(char), strlen(nodeNr), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lat, sizeof(char), strlen(lat), fpOut);
        fwrite(",", sizeof(char), 1, fpOut);
        fwrite(lon, sizeof(char), strlen(lon), fpOut);
        fwrite("\n", sizeof(char), 1, fpOut);
    }
    printf("checked coordinates written to %s\n", outFile);
}

void testAlt(char nodeFile[], char edgeFile[], char poiFile[], char preFile[],
             int from, int to)
{
    printf("nodes:%s edges:%s pois:%s\n", nodeFile, edgeFile, poiFile);
    Graph *graph = readGraph(nodeFile, edgeFile, poiFile, false);
    initNodeDistances(graph, from);
    Route *route = initRoute(from, to);
    char outFile[] = "path.csv";
    char outFileChecked[] = "path-checked.csv";

    loadPreProcess(graph, preFile);

    djikstra(graph, route, true, MODE_ALT, NULL, 0);
    writePathAlt(graph, outFileChecked);
    if (!(route->destination < 0))
    {
        writePath(route, outFile);
    }
    resetNodes(graph, route, 0);
    exit(0);
}

void test(char nodeFile[], char edgeFile[], char poiFile[], int from, int to)
{
    printf("nodes:%s edges:%s pois:%s\n", nodeFile, edgeFile, poiFile);
    Graph *graph = readGraph(nodeFile, edgeFile, poiFile, false);
    initNodeDistances(graph, from);
    Route *route = initRoute(from, to);
    char outFile[] = "path.csv";

    djikstra(graph, route, true, MODE_DJIKSTRA, NULL, 0);
    if (!(route->destination < 0))
    {
        writePath(route, outFile);
    }
    resetNodes(graph, route, 0);
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

        char norPre[] = "pre-norden";
        int nordkapp = 2151398; // northern norway
        int kvalheim = 2435670; // western norway
        int tonder = 1156810;   // southern denmark
        int vaalimaa = 3182202; // south-eastern finland

        int trondheim = 6861306;
        int vaernes = 6590451;
        int oslo = 2518118;
        int stockholm = 6487468;
        int meraaker = 6579983;
        int stjordal = 1693246;
        int steinkjer = 1536705;
        int stavanger = 3447384;
        int tampere = 136963;
        int kaarvaag = 6368906;
        int gjemnes = 6789998;
        int snaasa = 5379848;
        int mehamn = 2951840;

        if (strcmp(argv[1], "ti1") == 0)
            test(iceNode, iceEdge, icePoi, reykjavik, selfoss);

        if (strcmp(argv[1], "tr2a") == 0)
            test(norNode, norEdge, norPoi, meraaker, stjordal);
        if (strcmp(argv[1], "tr2b") == 0)
            test(norNode, norEdge, norPoi, stjordal, steinkjer);
        if (strcmp(argv[1], "tr2c") == 0)
            test(norNode, norEdge, norPoi, oslo, stockholm);
        if (strcmp(argv[1], "tr3a") == 0)
            test(norNode, norEdge, norPoi, trondheim, oslo);
        if (strcmp(argv[1], "tr3b") == 0)
            test(norNode, norEdge, norPoi, oslo, trondheim);
        if (strcmp(argv[1], "tr5a") == 0)
            test(norNode, norEdge, norPoi, stavanger, tampere);
        if (strcmp(argv[1], "tr5b") == 0)
            test(norNode, norEdge, norPoi, tampere, stavanger);
        if (strcmp(argv[1], "tr6") == 0)
            test(norNode, norEdge, norPoi, kaarvaag, gjemnes);
        if (strcmp(argv[1], "tr7") == 0)
            test(norNode, norEdge, norPoi, tampere, trondheim);
        if (strcmp(argv[1], "tr9a") == 0)
            test(norNode, norEdge, norPoi, nordkapp, trondheim);
        if (strcmp(argv[1], "tr9b") == 0)
            test(norNode, norEdge, norPoi, trondheim, nordkapp);
        if (strcmp(argv[1], "tr9c") == 0)
            test(norNode, norEdge, norPoi, vaalimaa, trondheim);
        if (strcmp(argv[1], "tr9d") == 0)
            test(norNode, norEdge, norPoi, trondheim, vaalimaa);

        if (strcmp(argv[1], "talt2a") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, meraaker, stjordal);
        if (strcmp(argv[1], "talt2b") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, stjordal, steinkjer);
        if (strcmp(argv[1], "talt3a") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, trondheim, oslo);
        if (strcmp(argv[1], "talt3b") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, oslo, trondheim);
        if (strcmp(argv[1], "talt4a") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, snaasa, mehamn);
        if (strcmp(argv[1], "talt4b") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, mehamn, snaasa);
        if (strcmp(argv[1], "talt5a") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, stavanger, tampere);
        if (strcmp(argv[1], "talt5b") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, tampere, stavanger);
        if (strcmp(argv[1], "talt6") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, kaarvaag, gjemnes);
        if (strcmp(argv[1], "talt7") == 0)
            testAlt(norNode, norEdge, norPoi, norPre, tampere, trondheim);

        if (strcmp(argv[1], "tpre1") == 0)
        {
            int landmarks[] = {nordkapp};
            int m = sizeof(landmarks) / sizeof(int);
            preProcess(norNode, norEdge, norPoi, norPre, landmarks, m);
        }
        if (strcmp(argv[1], "tpre2") == 0)
        {
            int landmarks[] = {nordkapp, tonder, vaalimaa};
            int m = sizeof(landmarks) / sizeof(int);
            preProcess(norNode, norEdge, norPoi, norPre, landmarks, m);
        }
        if (strcmp(argv[1], "tpre3") == 0)
        {
            int landmarks[] = {nordkapp, kvalheim, tonder, vaalimaa};
            int m = sizeof(landmarks) / sizeof(int);
            preProcess(norNode, norEdge, norPoi, norPre, landmarks, m);
        }

        if (strcmp(argv[1], "tfuel1") == 0)
            testFindStations(iceNode, iceEdge, icePoi, MODE_FUEL, reykjavik);
        if (strcmp(argv[1], "tfuel2") == 0)
            testFindStations(norNode, norEdge, norPoi, MODE_FUEL, trondheim);
        if (strcmp(argv[1], "tfuel3") == 0)
            testFindStations(norNode, norEdge, norPoi, MODE_FUEL, vaernes);
        if (strcmp(argv[1], "tcharger1") == 0)
            testFindStations(iceNode, iceEdge, icePoi, MODE_CHARGER, reykjavik);
        if (strcmp(argv[1], "tcharger2") == 0)
            testFindStations(norNode, norEdge, norPoi, MODE_CHARGER, trondheim);
        if (strcmp(argv[1], "tcharger3") == 0)
            testFindStations(norNode, norEdge, norPoi, MODE_CHARGER, vaernes);
    }

    printf("usage: %1$s route|pre\n"
           "pre-process ALT: %1$s pre <nodes> <edges> <out> <landmark> [landmark2..]\n"
           "routing: %1$s route <nodes> <edges> <poi> <pre>\n"
           "\tafter loading:\n"
           "\tdjik|alt <from> <to> [file]\n"
           "\t\tfind shortest path with Djikstra or ALT\n"
           "\tfuel|charger <node> <n> <file>\n"
           "\t\tfind n closest gas stations or chargers\n"
           "Routes can optionally be written as CSV of nr,node,lat,long\n",
           argv[0]);

    return 1;
}