#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct NodeStruct
{
    int data;
    struct NodeStruct *next;
} Node;

typedef struct SingleCircularLinkedListStruct
{
    Node *head;
    int elements;
} SingleCircularLinkedList;

// create node (nodeptr, id)
Node *createNode(Node *next, int data)
{
    Node *this = malloc(sizeof(Node));
    this->data = data;
    this->next = next != NULL ? next : this;
    return this;
}

// add node to list (listptr, nodeptr)

// remove node from list
void deleteNode(Node *node, SingleCircularLinkedList *list)
{
    Node *prev;
    Node *this = list->head;

    while (this != node)
    {
        prev = this;
        this = this->next;
    }

    prev->next = this->next; // skip to be deleted node
    if (list->head == this)
    {
        list->head = this->next; // update head if deleted node was head
    }
    list->elements--;
    free(this);
}

Node *skipNodes(Node *node, int n)
{
    for (int i = 0; i < n; i++)
    {
        node = node->next;
    }
    return node;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("usage: %s <n> <interval>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);
    if (n == 0 || m == 0)
    {
        printf("arguments must be integers\n");
        return 1;
    }

    printf("%i %i\n", n, m);

    SingleCircularLinkedList *list = malloc(sizeof(SingleCircularLinkedList));

    // Node *head = (malloc(sizeof(Node)));
    Node *head = createNode(NULL, 1);
    printf("list: %p\n", list);
    printf("head: %p data: %i next: %p\n", head, head->data, head->next);
    list->head = head;

    // loop generate nodes
    for (int i = 0; i < 5; i++)
    {
        // Node *next = NULL;
        // createNode(next, i + 1);
    }

    Node *temp = list->head;
    for (int i = 0; i < 5; i++)
    {
        printf("%i %p\n", temp->data, temp);
        temp = temp->next;
    }

    Node *tobeDeleted = skipNodes(list->head, 3);
    printf("tobedeleteD: %p\n", tobeDeleted);

    deleteNode(tobeDeleted, list);

    Node *temp2 = list->head;
    for (int i = 0; i < 5; i++)
    {
        printf("%i %p\n", temp2->data, temp2);
        temp = temp2->next;
    }
}