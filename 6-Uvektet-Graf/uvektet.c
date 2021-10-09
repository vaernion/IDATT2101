#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 18058 Moholt
// 37774 Kalvskinnet

typedef struct NodeStruct
{
    int to;
    struct NodeStruct *next;
} Node;

void insert(Node **nodes, int from, int to)
{
    printf("Node - from:%d - to:%d\n", from, to);
    Node *node = malloc(sizeof(Node));
    node->to = to;

    printf("insert: %p %d\n", &node[from], node[from].to);

    if (nodes[from] != NULL)
    {
        node->next = nodes[from];
    }

    nodes[from] = node;
}

void readGraphFile(char file[], Node **nodes)
{
    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        perror("Error while opening file");
        exit(1);
    }

    const int buffSize = 1024;
    char buffer[buffSize];

    // read number of nodes from first line in graph file
    fgets(buffer, buffSize, fp);
    int n = atoi(strtok(buffer, " "));
    printf("n: %d\n", n);

    *nodes = calloc(n, sizeof(Node));

    if (nodes == NULL)
    {
        printf("Failed to allocate memory\n");
        exit(1);
    }

    while (fgets(buffer, buffSize, fp))
    {
        int from = atoi(strtok(buffer, " "));
        int to = atoi(strtok(NULL, " "));
        printf("pre-insert: asdf from:%d to:%d   ", from, to);
        insert(nodes, from, to);
    }

    fclose(fp);
}

void bfs(char file[], int start)
{
    Node *nodes;
    readGraphFile(file, &nodes);
    // printf("%ld \n", nodes[0]->to);

    // Node *s = nodes[1];
    // printf("%d", s);

    // free(nodes);
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

            printf("bfs\n");
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