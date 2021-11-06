#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define FREQS 256

typedef struct HuffNodeStruct
{
    char value;
    int freq;
    struct HuffNodeStruct *left;
    struct HuffNodeStruct *right;
} HuffNode;

typedef struct QueueItemStruct
{
    HuffNode *node;
    struct QueueItemStruct *next;
} QueueItem;

typedef struct QueueStruct
{
    QueueItem *head;
    QueueItem *tail;
} Queue;

void queueInsert(Queue *queue, HuffNode *node)
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

void queueInsertAt(Queue *queue, HuffNode *node, QueueItem *position)
{
    QueueItem *item = calloc(1, sizeof(QueueItem));
    item->node = node;

    if (position == NULL)
    {
        item->next = queue->head;
        queue->head = item;
    }

    for (QueueItem *cur = queue->head; cur != NULL; cur = cur->next)
    {
        if (cur == position)
        {
            item->next = cur->next;
            cur->next = item;
        }
    }

    if (position == queue->tail)
    {
        queue->tail = item;
    }
}

HuffNode *queueGet(Queue *queue)
{
    QueueItem *item = queue->head;
    HuffNode *node = item->node;
    queue->head = item->next;
    free(item);
    if (queue->head == NULL)
        queue->tail = NULL;
    return node;
}

short shortFromBytes(char input[], int i)
{
    return *((short *)&input[i]);
}

void insertShortToBytes(char bytes[], int i, short x)
{
    *((short *)&bytes[i]) = x;
}

int *huffFreqs(char input[], int n)
{
    int *freq = calloc(FREQS, sizeof(int));

    int i = 0;
    while (i < n)
    {
        short lzRef = shortFromBytes(input, i);
        if (lzRef < 0)
        {
            printf("huffFreqs: skipping 3 bytes lzref\n");
            i += 3;
        }
        else if (lzRef > 0)
        {
            i += 2;
            printf("huffFreqs: encoding %i bytes & skipping 2 bytes lzref \n", lzRef);

            for (int j = 0; j < lzRef; j++)
            {
                unsigned char x = input[i];
                freq[x] += 1;
                i += 1;
            }
        }
        else
        {
            printf("ERROR: huufFreqs - null lzref\n");
            exit(1);
        }
    }

    return freq;
}

void bubbleSort(HuffNode t[], int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < (n - i - 1); j++)
        {
            if (t[j].freq > t[j + 1].freq)
            {
                HuffNode temp = t[j];
                t[j] = t[j + 1];
                t[j + 1] = temp;
            }
        }
    }
}

HuffNode *genHuffTree(int freq[])
{
    int numLeaves = 0;
    for (int i = 0; i < FREQS; i++)
    {
        if (freq[i] > 0)
            numLeaves += 1;
    }

    // generate leaves
    HuffNode *leaves = calloc(numLeaves, sizeof(HuffNode));
    int currentLeafIndex = 0;
    for (int i = 0; i < FREQS; i++)
    {
        if (freq[i] > 0)
        {
            leaves[currentLeafIndex].freq = freq[i];
            leaves[currentLeafIndex].value = i;
            currentLeafIndex += 1;
        }
    }

    // sort leaves with bubblesort for simplicity, we only have up to 256 elements
    bubbleSort(leaves, numLeaves);

    // add sorted leaves to queue
    Queue *queue = calloc(1, sizeof(Queue));
    for (int i = 0; i < numLeaves; i++)
    {
        queueInsert(queue, &leaves[i]);
    }

    // iterate queue, add new nodes to queue at appropiate position
    while (true)
    {
        HuffNode *first = queueGet(queue);

        if (queue->head == NULL)
        {
            free(queue);
            return first; // must be root
        }

        HuffNode *second = queueGet(queue);

        HuffNode *newNode = calloc(1, sizeof(HuffNode));
        newNode->left = first;
        newNode->right = second;
        newNode->freq = first->freq + second->freq;

        QueueItem *position = NULL; // will insert at head if it remains NULL
        for (QueueItem *cur = queue->head; cur != NULL; cur = cur->next)
        {
            // update position if it should be in the middle or at tail
            if (newNode->freq <= cur->node->freq || cur->next == NULL)
            {
                position = cur;
                break;
            }
        }
        queueInsertAt(queue, newNode, position);
    }
}

void printByte(char byte)
{
    printf("\nBITS for value: %hhu char: %c ---- ", byte, byte);
    for (int i = 7; i >= 0; i--)
    {
        char bit = (byte >> i) & 1;
        printf("%i", bit);
    }
    printf("\n");
}

void genHuffRoutes(char huffRoutes[FREQS][FREQS], int depths[], HuffNode *node, char route[], int depth)
{
    if (!node->left)
    {
        printf("found leaf: value %i %c freq: %i ### route %s ### depth %i\n",
               node->value, node->value, node->freq, route, depth);
        strncpy(huffRoutes[(int)node->value], route, depth);
        depths[(int)node->value] = depth;
        return;
    }
    char leftStr[FREQS] = {0};
    strncpy(leftStr, route, depth);
    leftStr[depth] = '0';
    genHuffRoutes(huffRoutes, depths, node->left, leftStr, depth + 1);

    char rightStr[FREQS] = {0};
    strncpy(rightStr, route, depth);
    rightStr[depth] = '1';
    genHuffRoutes(huffRoutes, depths, node->right, rightStr, depth + 1);
}

int writeHuff(char lzData[], int n, FILE *fp, int freq[], HuffNode *root)
{
    // bitstrings to find the nodes in the tree
    // inner array could likely be smaller but
    // we pay the cost of 64k memory while compressing
    // to handle very skewed frequencies
    char huffRoutes[FREQS][FREQS] = {0};
    // parallel to huffRoutes,
    int depths[FREQS];
    printf("-- huffRoutes --- \n");
    genHuffRoutes(huffRoutes, depths, root, 0, 0);
    printf("\n");

    int bytesWritten = 0;

    // iterate LZ data blocks until end of lzData
    for (int i = 0; i < n;)
    {
        short lzRef = shortFromBytes(lzData, i);
        printf("lzRef: %i -- ", lzRef);

        // skip LZ repetition symbols
        if (lzRef < 0)
        {
            char lzRefLength = lzData[i + 2];
            printf("skipping LZ ref %i length %i\n", lzRef, lzRefLength);
            fwrite(&lzRef, sizeof(lzRef), 1, fp);             // write LZ reference
            fwrite(&lzRefLength, sizeof(lzRefLength), 1, fp); // write LZ length
            // 2 byte for LZ reference + 1 for LZ length
            i += 3;
            bytesWritten += 3;
        }
        // compress uncompressed lz data with huffman
        else if (lzRef > 0)
        {
            fwrite(&lzRef, sizeof(short), 1, fp); // positive LZ block
            bytesWritten += 2;
            printf("encoding %i bytes\n", lzRef);

            unsigned char bitBuff = 0;
            int bitBuffUsed = 0;

            // iterate bytes in uncompressed section
            for (int j = 2; j < lzRef + 2; j++)
            {
                unsigned char value = lzData[i + j];
                int huffCodeLength = depths[value];

                // add bits in huffCode to buffer
                for (int k = 0; k < huffCodeLength; k++)
                {
                    char huffBit = 0;

                    if (huffRoutes[value][k] == '1')
                        huffBit = 1;

                    bitBuff = (bitBuff << 1) | huffBit;
                    bitBuffUsed++;

                    // if bitbuff full, write byte to file and clear bitbuff
                    if (bitBuffUsed == 8)
                    {
                        fwrite(&bitBuff, sizeof(bitBuff), 1, fp);
                        bitBuff = 0;
                        bitBuffUsed = 0;
                        bytesWritten++;
                    }
                }
            }

            // write padded remaining bit buffer to file
            if (bitBuffUsed > 0)
            {
                int remaining = 8 - bitBuffUsed;
                bitBuff = bitBuff << remaining;
                fwrite(&bitBuff, sizeof(bitBuff), 1, fp);
                bytesWritten++;
            }

            i += lzRef + 2; // go to next LZ section
        }
        else
        {
            printf("ERROR: neither LZ uncompressed section nor reference\n");
            exit(1);
        }
    }
    return bytesWritten;
}

void traverseHuffCode(HuffNode *node, char input[], int i, char *value, int *blockBitsRead)
{
    int bytesRead = *blockBitsRead >> 3; // divide by 8
    int bitsIntoNextByte = *blockBitsRead % 8;
    int bitsToShift = 7 - bitsIntoNextByte;

    char bit = (input[i + bytesRead] >> bitsToShift) & 1;

    // leaves have no children, it's enough to check one
    if (!node->left)
    {
        *value = node->value;
        return;
    }

    (*blockBitsRead)++;

    // 0 -> left, 1 -> right
    if (bit)
        traverseHuffCode(node->right, input, i, value, blockBitsRead);
    else
        traverseHuffCode(node->left, input, i, value, blockBitsRead);
}

void readHuff(char input[], int n, char lzData[], int freq[], HuffNode *root)
{
    int lzDataIndex = 0;

    // iterate huffman compressed LZ data
    int i = 0;
    while (i < n)
    {
        short lzRef = shortFromBytes(input, i);

        if (lzRef < 0)
        {
            int lzLength = input[i + 2];
            insertShortToBytes(lzData, lzDataIndex, lzRef);
            lzData[lzDataIndex + 2] = lzLength;
            i += 3; // 2 byte for LZ reference + 1 for LZ length
            lzDataIndex += 3;
        }
        else if (lzRef > 0)
        {
            i += 2; // lzref size
            insertShortToBytes(lzData, lzDataIndex, lzRef);
            lzDataIndex += 2;
            int blockBitsRead = 0;

            // iterate until expected number of bytes are decompressed
            for (int j = 0; j < lzRef; j++)
            {
                char value = 0;

                // traverse tree with input bits until value is found
                traverseHuffCode(root, input, i, &value, &blockBitsRead);

                lzData[lzDataIndex++] = value;
            }

            // need to know how far ahead to skip
            // remaining bits of last byte in block are zeroed
            int bytesRead = (blockBitsRead + 7) / 8; // int rounds down
            i += bytesRead;                          // go to next LZ block
        }
        else
        {
            printf("ERROR: LZ reference was 0\n");
            exit(1);
        }
    }
}

void genLZ(char fileData[], int n, char lzData[], int lzDataLength, int *lzDataUsed)
{
    int i = 0;
    while (i < n)
    {
        //
        i += n;
    }
    insertShortToBytes(lzData, 0, n);
    strncpy(&lzData[2], fileData, n);
    *lzDataUsed += n + 2;
}

// compresses input file with LZ
// then uncompressed LZ blocks with Huffman
void compress(char infile[], char outfile[])
{
    printf("### compressing %s to %s ###\n", infile, outfile);

    FILE *fpIn = fopen(infile, "rb");
    if (fpIn == NULL)
    {
        perror("Error while opening infile");
        exit(1);
    }

    fseek(fpIn, 0, SEEK_END);
    int fileLength = ftell(fpIn);
    fseek(fpIn, 0, SEEK_SET);

    char *fileData = calloc(fileLength, sizeof(char));
    fread(fileData, fileLength, 1, fpIn);
    fclose(fpIn);

    // lack of proper handling in case file has no/few repetitions
    int lzDataLength = fileLength + 1024;
    char *lzData = calloc(fileLength + 1024, sizeof(char));
    int lzDataUsed = 0;

    genLZ(fileData, fileLength, lzData, lzDataLength, &lzDataUsed);
    free(fileData);

    printf("lzData: %s\n", lzData);
    for (int i = 0; i < lzDataUsed; i++)
        printf("%hhi ", lzData[i]);

    int n = lzDataUsed;

    int *freq = huffFreqs(lzData, n);
    HuffNode *root = genHuffTree(freq);

    FILE *fpOut = fopen(outfile, "wb");
    if (fpOut == NULL)
    {
        perror("Error while opening outfile");
        exit(1);
    }

    // 1st section: 1024 byte frequency table
    fwrite(freq, sizeof(int), FREQS, fpOut);

    // 2nd section: bytes of de-huffman'd data (with LZ symbols)
    // simplifies array handling when decompressing, at the cost of 4 bytes
    fwrite(&n, sizeof(n), 1, fpOut);

    int huffBytes = writeHuff(lzData, n, fpOut, freq, root);
    printf("\n  ---------- summary ----------\n");
    printf("  %-8s %8s %8s\n", "Data", "Bytes", "Size");
    printf("  %-8s %8i\n", "File", fileLength);
    printf("  %-8s %8i %8.2f%%\n", "LZ",
           lzDataUsed, (float)lzDataUsed / (float)fileLength * 100);
    printf("  %-8s %8i %8.2f%%\n", "Huff",
           huffBytes, (float)huffBytes / (float)fileLength * 100);
}

// decompress Huffman using frequency table from infile
// then decompress LZ references and write to outfile
void decompress(char infile[], char outfile[])
{
    printf("### decompressing %s to %s ###\n", infile, outfile);

    FILE *fp = fopen(infile, "rb");
    if (fp == NULL)
    {
        perror("Error while opening infile");
        exit(1);
    }

    // read 1st section: frequency table
    int freq[FREQS];
    fread(freq, sizeof(int), FREQS, fp);

    // read 2nd section: data bytes after decompressing huffman
    int lzDataLength = 0;
    fread(&lzDataLength, sizeof(int), 1, fp);

    printf("lzDataLength: %i\n", lzDataLength);
    printf("--- FREQUENCY TABLE ---\n");
    for (int i = 0; i < FREQS; i++)
    {
        int currentFreq = freq[i];
        if (currentFreq > 0)
        {
            printf("value: %i char: %c freq: %u\n", i, i, currentFreq);
        }
    }
    // read 3rd section: LZ and huffman encoded bytes
    HuffNode *root = genHuffTree(freq);

    int bodyStart = ftell(fp);
    fseek(fp, 0, SEEK_END);
    int bodyLength = ftell(fp) - bodyStart;
    fseek(fp, bodyStart, SEEK_SET);

    char *huffData = calloc(bodyLength, sizeof(char));
    char *lzData = calloc(lzDataLength, sizeof(char));

    fread(huffData, sizeof(char), bodyLength, fp);

    printf("\n--- HUFFDATA ---\n");
    for (int i = 0; i < bodyLength; i++)
        printf("%hhi ", huffData[i]);
    printf("\n\n");

    // iterate huffdata, write to lzData
    readHuff(huffData, bodyLength, lzData, freq, root);

    printf("\n--- LZDATA raw ---\n");
    for (int i = 0; i < lzDataLength; i++)
        printf("%hhi ", lzData[i]);
    printf("\n\n");

    printf("\n--- LZDATA char ---\n");
    for (int i = 0; i < lzDataLength; i++)
        printf("%c", lzData[i]);
    printf("\n\n");

    // fread(lzString, )

    // LZ

    // write outfile from LZ

    FILE *fpOut = fopen(outfile, "wb");
    if (fpOut == NULL)
    {
        perror("Error while opening outfile");
        exit(1);
    }
    fwrite(lzData, sizeof(lzData), 1, fpOut);
}

// compress or decompress a file using LZ and Huffman together
int main(int argc, char *argv[])
{
    if (argc > 3)
    {
        if (strcmp(argv[1], "c") == 0)
        {
            compress(argv[2], argv[3]);
            return 0;
        }
        if (strcmp(argv[1], "d") == 0)
        {
            decompress(argv[2], argv[3]);
            return 0;
        }
    }
    printf("usage: %s c|d infile outfile\n", argv[0]);
    return 1;
}