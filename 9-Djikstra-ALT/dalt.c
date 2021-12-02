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
void heapSwap2(Heap *heap, int a, int b)
{
    int temp = heap->nodes[a];
    heap->nodes[a] = heap->nodes[b];
    heap->nodes[b] = temp;
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
    while (i && nodes[heap->nodes[i]].weight <
                    nodes[heap->nodes[f = heapOver(i)]].weight)
    {
        heapSwap(&heap->nodes[i], &heap->nodes[f]);
        i = f;
    }
}

void heapInsert(Heap *heap, int x, Node *nodes)
{
    int i = heap->length++;
    heap->nodes[i] = x;
    heapPrioUp(heap, i, nodes);
}

void heapFixOrig(Heap *heap, int i, Node *nodes)
{
    int m = heapLeft(i);
    if (m < heap->length)
    {
        int h = m + 1;
        int hWeight = nodes[heap->nodes[h]].weight;
        int mWeight = nodes[heap->nodes[m]].weight;

        if (h < heap->length && hWeight < mWeight)
        {
            m = h;
            mWeight = hWeight;
        }

        int iWeight = nodes[heap->nodes[i]].weight;

        if (mWeight < iWeight)
        {
            // heapSwap(&heap->nodes[i], &heap->nodes[m]);
            heapSwap2(heap, i, m);
            heapFixOrig(heap, m, nodes);
        }
    }
}

void heapFix(Heap *heap, int i, Node *nodes)
{
    int l = heapLeft(i);
    int r = l + 1;
    int m;

    int lWeight = nodes[heap->nodes[l]].weight;
    int iWeight = nodes[heap->nodes[i]].weight;

    if (l < heap->length && lWeight < iWeight)
        m = l;
    else
        m = i;

    int rWeight = nodes[heap->nodes[r]].weight;
    int mWeight = nodes[heap->nodes[m]].weight;

    if (r < heap->length && rWeight < mWeight)
    {
        m = r;
        mWeight = rWeight;
    }

    if (m != i)
    {
        // heapSwap(&heap->nodes[i], &heap->nodes[m]);
        heapSwap2(heap, i, m);
        heapFix(heap, m, nodes);
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

        if (weight < 0)
        {
            printf("negative edge %i  ", weight);
        }

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
    const int outputLimit = 2000;
    int moduloFilter = (route->numNodes / outputLimit) + 1;

    for (int i = 0; i < route->numNodes; i++)
    {
        if (i % moduloFilter != 0)
            continue;

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

        if (distanceBehind > estimate && distanceBehind < infinity)
            estimate = distanceBehind;

        int distanceAfter =
            (graph->toMarks + node * graph->m)[i] -
            (graph->toMarks + goal * graph->m)[i];

        if (distanceAfter > estimate && distanceAfter < infinity)
            estimate = distanceAfter;

        // debug estimates
        if ((graph->fromMarks + goal * graph->m)[i] < 0 ||
            (graph->fromMarks + goal * graph->m)[i] >= infinity)
        {
            printf("invalid_estimate1 ");
        }
        if ((graph->fromMarks + node * graph->m)[i] < 0 ||
            (graph->fromMarks + node * graph->m)[i] >= infinity)
        {
            printf("invalid_estimate2 ");
        }
        // if ((graph->toMarks + node * graph->m)[i] < 0 ||
        //     (graph->toMarks + node * graph->m)[i] >= infinity)
        // {
        //     printf("invalid_estimate3: %i node:%i ",
        //            (graph->toMarks + node * graph->m)[i], node);
        // }
        if ((graph->toMarks + goal * graph->m)[i] < 0 ||
            (graph->toMarks + goal * graph->m)[i] >= infinity)
        {
            printf("invalid_estimate4 ");
        }
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

    int prevQueueWeight = 0;
    int queueWeightSmallerCount = 0;
    int prevStartWeight = 0;
    int queueStartSmallerCount = 0;

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

        if (node->weight < prevQueueWeight)
        {
            // printf("queue weight:%i < prevQueueWeight:%i\n",
            //        node->weight, prevQueueWeight);
            queueWeightSmallerCount++;
        }
        prevQueueWeight = node->weight;

        if (node->startDist < prevStartWeight)
            queueStartSmallerCount++;
        prevStartWeight = node->startDist;

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
            int newNeighborDist = node->startDist + edge->weight;

            if (mode == MODE_ALT && neighbor->estimateToGoal == 0)
            {
                neighbor->estimateToGoal = estimateALT(graph, route->destination, neighbor->nr);
                if (neighbor->estimateToGoal < 0)
                    printf("estimateALT returned negative  ");
            }

            if (!neighbor->checked &&
                newNeighborDist < neighbor->startDist &&
                newNeighborDist + neighbor->estimateToGoal < neighbor->weight)
            {
                neighbor->weight = newNeighborDist + neighbor->estimateToGoal;
                neighbor->startDist = newNeighborDist;
                neighbor->previous = node;
                heapInsert(heap, neighbor->nr, graph->nodes);
            }
        }
    }

    free(heap);

    printf("queueWeightSmallerCount: %i\n", queueWeightSmallerCount);
    printf("queueStartSmallerCount: %i\n", queueStartSmallerCount);

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

void loadPreProcess(Graph *graph, char preFile[])
{
    printf("loading preprocessed landmarks from %s\n", preFile);
    float startTime = (float)clock() / CLOCKS_PER_SEC;

    FILE *fp = fopen(preFile, "rb");
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

void runFindStations(char nodeFile[], char edgeFile[], char poiFile[], char outFile[],
                     char mode, int n, int node)
{
    printf("\n nodes:%s edges:%s pois:%s\n", nodeFile, edgeFile, poiFile);
    Graph *graph = readGraph(nodeFile, edgeFile, poiFile, false);
    initNodeDistances(graph, node);
    Route *route = initRoute(node, -1);

    findStations(graph, route, outFile, mode, n);
    exit(0);
}

void writeCheckedNodes(Graph *graph, char outFile[])
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

void shortestPath(char nodeFile[], char edgeFile[], char poiFile[], char preFile[], char outFile[],
                  char mode, int from, int to)
{
    printf("nodes:%s edges:%s pois:%s\n", nodeFile, edgeFile, poiFile);
    Graph *graph = readGraph(nodeFile, edgeFile, poiFile, false);
    initNodeDistances(graph, from);
    Route *route = initRoute(from, to);

    if (preFile != NULL)
        loadPreProcess(graph, preFile);

    djikstra(graph, route, true, mode, NULL, 0);
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
    if (argc > 3 && strcmp(argv[1], "route") == 0)
    {
        routeTerminal(argv[2]);
        return 0;
    }
    else if (argc > 6 && strcmp(argv[1], "pre") == 0)
    {
        int m = argc - 6;
        int landmarks[m];
        for (int i = 0; i < m; i++)
        {
            landmarks[i] = atoi(argv[6 + i]);
        }

        preProcess(argv[2], argv[3], argv[4], argv[5], landmarks, m);
        return 0;
    }
    else if (argc > 7 && strcmp(argv[1], "djik") == 0)
    {
        int from = atoi(argv[6]);
        int to = atoi(argv[7]);
        shortestPath(argv[2], argv[3], argv[4], NULL, argv[5], MODE_DJIKSTRA, from, to);
        return 0;
    }
    else if (argc > 8 && strcmp(argv[1], "alt") == 0)
    {
        int from = atoi(argv[7]);
        int to = atoi(argv[8]);
        shortestPath(argv[2], argv[3], argv[4], argv[5], argv[6], MODE_ALT, from, to);
        return 0;
    }
    else if (argc > 7 && (strcmp(argv[1], "fuel") == 0 || strcmp(argv[1], "charger") == 0))
    {
        int n = atoi(argv[6]);
        int node = atoi(argv[7]);
        char mode = strcmp(argv[1], "fuel") == 0 ? MODE_FUEL : MODE_CHARGER;

        runFindStations(argv[2], argv[3], argv[4], argv[5], mode, n, node);
        return 0;
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
        char norPre[] = "pre-norden";
        char pathFile[] = "path.csv";
        char stationsFile[] = "stations.csv";

        int reykjavik = 30979;
        int selfoss = 107046;

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
            shortestPath(iceNode, iceEdge, icePoi, NULL, pathFile, MODE_DJIKSTRA, reykjavik, selfoss);

        if (strcmp(argv[1], "tr2a") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, meraaker, stjordal);
        if (strcmp(argv[1], "tr2b") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, stjordal, steinkjer);
        if (strcmp(argv[1], "tr2c") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, oslo, stockholm);
        if (strcmp(argv[1], "tr3a") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, trondheim, oslo);
        if (strcmp(argv[1], "tr3b") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, oslo, trondheim);
        if (strcmp(argv[1], "tr5a") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, stavanger, tampere);
        if (strcmp(argv[1], "tr5b") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, tampere, stavanger);
        if (strcmp(argv[1], "tr6") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, kaarvaag, gjemnes);
        if (strcmp(argv[1], "tr7") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, tampere, trondheim);
        if (strcmp(argv[1], "tr9a") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, nordkapp, trondheim);
        if (strcmp(argv[1], "tr9b") == 0)
            shortestPath(norNode, norEdge, norPoi, NULL, pathFile, MODE_DJIKSTRA, trondheim, nordkapp);

        if (strcmp(argv[1], "talt2a") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, meraaker, stjordal);
        if (strcmp(argv[1], "talt2b") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, stjordal, meraaker);
        if (strcmp(argv[1], "talt2c") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, stjordal, steinkjer);
        if (strcmp(argv[1], "talt3a") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, trondheim, oslo);
        if (strcmp(argv[1], "talt3b") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, oslo, trondheim);
        if (strcmp(argv[1], "talt4a") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, snaasa, mehamn);
        if (strcmp(argv[1], "talt4b") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, mehamn, snaasa);
        if (strcmp(argv[1], "talt5a") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, stavanger, tampere);
        if (strcmp(argv[1], "talt5b") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, tampere, stavanger);
        if (strcmp(argv[1], "talt6") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, kaarvaag, gjemnes);
        if (strcmp(argv[1], "talt7") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, tampere, trondheim);
        if (strcmp(argv[1], "talt9a") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, nordkapp, trondheim);
        if (strcmp(argv[1], "talt9b") == 0)
            shortestPath(norNode, norEdge, norPoi, norPre, pathFile, MODE_ALT, trondheim, nordkapp);

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
            runFindStations(iceNode, iceEdge, icePoi, stationsFile, MODE_FUEL, 10, reykjavik);
        if (strcmp(argv[1], "tfuel2") == 0)
            runFindStations(norNode, norEdge, norPoi, stationsFile, MODE_FUEL, 10, trondheim);
        if (strcmp(argv[1], "tfuel3") == 0)
            runFindStations(norNode, norEdge, norPoi, stationsFile, MODE_FUEL, 10, vaernes);
        if (strcmp(argv[1], "tcharger1") == 0)
            runFindStations(iceNode, iceEdge, icePoi, stationsFile, MODE_CHARGER, 10, reykjavik);
        if (strcmp(argv[1], "tcharger2") == 0)
            runFindStations(norNode, norEdge, norPoi, stationsFile, MODE_CHARGER, 10, trondheim);
        if (strcmp(argv[1], "tcharger3") == 0)
            runFindStations(norNode, norEdge, norPoi, stationsFile, MODE_CHARGER, 10, vaernes);
    }

    printf("usage:\n"
           "Pre-process ALT: %1$s pre <nodes> <edges> <poi> <out> <landmark> [landmark2..]\n"
           "Djikstra: %1$s djik <nodes> <edges> <poi> <out> <from> <to>\n"
           "ALT: %1$s alt <nodes> <edges> <poi> <pre> <out> <from> <to>\n"
           "Find stations: %1$s fuel|charger <nodes> <edges> <poi> <out> n <node>\n"
           "Routes will be written to <out> as CSV of nr,node,lat,long\n",
           argv[0]);

    return 1;
}