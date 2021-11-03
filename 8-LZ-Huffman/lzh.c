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
        // printf("POSITION WAS NULL\n");
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

int *huffFreqs(char input[], int n)
{
    int *freq = calloc(FREQS, sizeof(int));

    for (int i = 0; i < n; i++)
    {
        unsigned char x = input[i];
        freq[x] += 1;
    }

    // for (int i = 0; i < FREQS; i++)
    // {
    //     if (freq[i] > 0)
    //     {
    //         printf("ascii: %hhi char: %c  freq: %i\n", i, i, freq[i]);
    //     }
    // }

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
    // printf("numNodes: %i\n", numLeaves);

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

    for (int i = 0; i < numLeaves; i++)
    {
        printf("value: %hhi char: %c freq: %i\n", leaves[i].value, leaves[i].value, leaves[i].freq);
    }

    // iterate queue, add new nodes to queue at appropiate position
    while (true)
    {
        HuffNode *first = queueGet(queue);

        if (queue->head == NULL)
            return first; // must be root

        HuffNode *second = queueGet(queue);

        HuffNode *newNode = calloc(1, sizeof(HuffNode));
        newNode->left = first;
        newNode->right = second;
        newNode->freq = first->freq + second->freq;

        // printf("newnode freq: %i -- left freq:%i val:%c -- right: freq:%i val:%c\n",
        //        newNode->freq, left->freq, left->value, right->freq, right->value);

        QueueItem *position = NULL; // will insert at head if it remains NULL
        for (QueueItem *cur = queue->head; cur != NULL; cur = cur->next)
        {
            // update position if it should be in the middle or at tail
            if (newNode->freq <= cur->node->freq || cur->next == NULL)
            {
                position = cur;
                // printf("position: freq-%i val-%c\n", position->node->freq, position->node->value);
                break;
            }
        }
        queueInsertAt(queue, newNode, position);
    }
}

void printByte(char byte)
{

    printf("BITS for value: %hhu char: %c\n", byte, byte);
    for (int i = 7; i >= 0; i--)
    {
        // int bit = (byte & 1 << i) > 0;
        char bit = (byte >> i) & 1;
        printf("%i", bit);
    }
    // printf(" --- flipped %hhu to %hhu", byte, ~byte);
    printf("\n");
}

long getHuffCode(char value, int freq[])
{
    return 5;
}

void writeHuff(char input[], int n, char outfile[], int freq[])
{
    FILE *fp = fopen(outfile, "wb");
    if (fp == NULL)
    {
        perror("Error while opening outfile");
        exit(1);
    }

    // 1st section: 1024 byte frequency table
    fwrite(freq, sizeof(int), FREQS, fp);

    // 2nd section: bytes of de-huffman'd data (with LZ symbols)
    // simplifies array handling when decompressing, at the small cost of 4 bytes
    fwrite(&n, sizeof(n), 1, fp);

    // iterate LZ compressed data
    for (int i = 0; i < n;)
    {
        short lzRef = input[i] << 8 | input[i + 1];
        printf("lzRef: %i\n", lzRef);
        // skip LZ repetition symbols
        if (lzRef < 0)
        {
            int lzRefLength = input[i + 2];
            printf("skipping LZ ref %i length %i\n", lzRef, lzRefLength);
            fwrite(&lzRef, sizeof(lzRef), 1, fp);             // write LZ reference
            fwrite(&lzRefLength, sizeof(lzRefLength), 1, fp); // write LZ length
            // 2 byte for LZ reference + 1 for LZ length
            i += 3;
        }
        else if (lzRef > 0 || true)
        {
            int toEncode = input[i + 2];
            printf("encoding %i bytes\n\n", toEncode);

            toEncode = strlen(input);
            unsigned char bitBuff;
            int bitBuffUsed = 0;

            // iterate bytes in uncompressed section
            for (int j = 1; j < toEncode + 1; j++)
            {

                long huffCode = getHuffCode(input[i + j], freq);
                int huffCodeLength = 0;
                int tempCode = huffCode;

                // find number of bits in huffCode
                while (tempCode != 0)
                {
                    tempCode /= 2;
                    huffCodeLength++;
                }

                // printf("huffCode: %li huffCodeHex: %lx huffCodeLength: %i\n\n",
                //        huffCode, huffCode, huffCodeLength);

                // iterate bits in huffCode
                for (int k = 0; k < huffCodeLength; k++)
                {
                    // shift bitBuff to make room for k'th rightmost bit of huffCode
                    // printf("pre: bitBuff value: %hhu char: %c bitBuffUsed: %i\n",
                    //        bitBuff, bitBuff, bitBuffUsed);
                    // printf("k: %i bitBuff << 1 == %i   huffCode >> k & 1 == %li\n",
                    //        k, (bitBuff << 1), (huffCode >> k & 1));
                    bitBuff = (bitBuff << 1) | (huffCode >> k & 1);
                    bitBuffUsed++;

                    // printf("post: bitBuff value: %hhu char: %c bitBuffUsed: %i\n",
                    //        bitBuff, bitBuff, bitBuffUsed);
                    // printByte(bitBuff);
                    // printf("\n");

                    // if bitbuff full, write byte to file and clear bitbuff
                    if (bitBuffUsed == 8)
                    {
                        fwrite(&bitBuff, sizeof(bitBuff), 1, fp);
                        bitBuff = 0;
                        bitBuffUsed = 0;
                    }
                }
            }
            // write padded remaining bit buffer to file
            // printf("leftover bitbuff bits: %i\n", bitBuffUsed);
            int remaining = 8 - bitBuffUsed;
            bitBuff = bitBuff << remaining;
            // printByte(bitBuff);
            fwrite(&bitBuff, sizeof(bitBuff), 1, fp);

            i += toEncode + 1; // go to next LZ section
        }
        else
        {
            printf("ERROR: neither LZ uncompressed section nor reference\n");
            exit(1);
        }
    }
}

void readHuff(char input[], int n, char lzData[], int freq[], HuffNode *root)
{
    int lzDataIndex = 0;

    // iterate huffman compressed LZ data
    for (int i = 0; i < n;)
    {
        short lzRef = input[i] << 8 | input[i + 1];
        if (lzRef < 0 && false)
        {
            int lzRefLength = input[i + 2];
            printf("skipping LZ ref %i length %i\n", lzRef, lzRefLength);
            i += 3; // 2 byte for LZ reference + 1 for LZ length
            memcpy(&lzData[lzDataIndex], &input[i], lzRefLength);
        }
        else if (lzRef > 0 || true)
        {
            // int toDecode = input[i + 2]; // need lz sections
            int toDecode = n;

            for (int j = 1; j < toDecode + 1; j++)
            {

                char value = 0;
                HuffNode *treePosition = root;

                // traverse tree with input bits until value is found
                value = 97;

                // append value to lzData
                // printf("value: %hhi %c\n", value, value);
                lzData[++lzDataIndex] = value;
            }

            i += toDecode + 1; // go to next LZ block
        }
        else
        {
            printf("ERROR: neither LZ uncompressed section nor reference\n");
            exit(1);
        }
    }
}

void compress(char infile[], char outfile[])
{
    printf("### compressing %s to %s ###\n", infile, outfile);

    FILE *fp = fopen(infile, "rb");
    if (fp == NULL)
    {
        perror("Error while opening infile");
        exit(1);
    }

    // lz
    char input[] = "aabc\n";
    // char input[] = "spennende_pennevenner";
    // char input[] = "alle barna elsker selskap";
    // char input[] = "alle barna elsker selskapalle barna elsker selskapalle barna\
    // elsker selskapalle barna elsker selskapalle barna elsker selskapalle barna\
    //  elsker selskap";
    // char input[] = "alle barna elsker selskapalle barna elsker selskapalle barna\
    // elsker selskapalle barna elsker selskapalle barna elsker selskapalle barna\
    //  elsker selskapalle barna elsker selskapalle barna elsker selskapalle barna\
    // elsker selskapalle barna elsker selskapalle barna elsker selskapalle barna\
    //  elsker selskapalle barna elsker selskapalle barna elsker selskapalle barna\
    // elsker selskapalle barna elsker selskapalle barna elsker selskapalle barna\
    //  elsker selskap";

    int n = strlen(input);

    printf("using %i length hardcoded string:\n\n%s\n\n", n, input);
    int *freq = huffFreqs(input, n);
    HuffNode *root = genHuffTree(freq);
    printf("root freq: %i\n", root->freq);

    writeHuff(input, n, outfile, freq);
}

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

    // debug printing
    printf("lzStringLength: %i\n", lzDataLength);
    for (int i = 0; i < FREQS; i++)
    {
        int currentFreq = freq[i];
        if (currentFreq > 0)
        {
            printf("value: %i char: %c freq: %u\n", i, i, currentFreq);
        }
    }
    // read 3rd section: LZ and huffman encoded bytes
    // huffman
    HuffNode *root = genHuffTree(freq);

    int bodyStart = ftell(fp);
    printf("bodyStart ftell %i\n", bodyStart);

    fseek(fp, 0, SEEK_END);
    int bodyLength = ftell(fp) - bodyStart;
    printf("bodyLength %i\n", bodyLength);

    fseek(fp, bodyStart, SEEK_SET);
    printf("AFTER END AND BACK: %li\n", ftell(fp));

    char *huffData = calloc(1, sizeof(char));
    char *lzData = calloc(lzDataLength, sizeof(char));

    fread(huffData, sizeof(char), bodyLength, fp);

    // iterate huffdata, write to lzData
    readHuff(huffData, bodyLength, lzData, freq, root);

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

int main(int argc, char *argv[])
{
    printByte(97);
    printByte(128 + 64 + 32 + 16);

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