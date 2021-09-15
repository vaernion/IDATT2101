#include <stdio.h>
#include <stdlib.h>

typedef struct NodeStruct
{
    char c;
    int line;
    struct NodeStruct *next;
} Node;

typedef struct Stack
{
    Node *head;
    int elements;
} Stack;

Node *createNode(Node *next, char c, int line)
{
    Node *this = malloc(sizeof(Node));
    this->next = next;
    this->c = c;
    this->line = line;
    return this;
}

void pushStack(Stack *stack, Node *node)
{
    node->next = stack->head;
    stack->head = node;
    stack->elements++;
}

void popStack(Stack *stack)
{
    Node *popped = stack->head;
    stack->head = popped->next;
    free(popped);
    stack->elements--;
}

void codeError(char c, int line)
{
    printf("Invalid syntax: %c at line %i\n", c, line);
    exit(1);
}

// Adds or removes symbols from the stack to track unclosed brackets.
// Skips brackets inside strings or comments.
// Does not handle Windows newlines (CRLF) or /* comment blocks */
void parseCode(Stack *stack, char c, char cPrev, int line)
{
    // ignore values following escape char, except an escaped escape char
    if (cPrev == '\\' && c != '\\')
        return;

    // "//" style comments last until newline
    if (stack->head != NULL && stack->head->c == '/')
    {
        if (c == '\n')
            popStack(stack);
        return;
    }
    // "#" style comments last until newline
    if (stack->head != NULL && stack->head->c == '#')
    {
        if (c == '\n')
            popStack(stack);
        return;
    }

    // handle completed quotes or skip escaped quotes inside quotes
    if (stack->head != NULL && stack->head->c == '"')
    {
        if (c == '"' && cPrev != '\\')
            popStack(stack);
        return;
    }
    else if (stack->head != NULL && stack->head->c == '\'')
    {
        if (c == '\'' && cPrev != '\\')
            popStack(stack);
        return;
    }

    // C style single line comments
    if (c == '/' && cPrev == '/')
    {
        Node *node = createNode(stack->head, c, line);
        pushStack(stack, node);
    }
    // BASH/Python style single line comments
    else if (c == '#')
    {
        Node *node = createNode(stack->head, c, line);
        pushStack(stack, node);
    }
    // double quotes
    else if (c == '"' && stack->head != NULL && stack->head->c != c)
    {
        Node *node = createNode(stack->head, c, line);
        pushStack(stack, node);
    }
    // single quotes
    else if (c == '\'' && stack->head != NULL && stack->head->c != c)
    {
        Node *node = createNode(stack->head, c, line);
        pushStack(stack, node);
    }

    else if (c == '{' || c == '[' || c == '(')
    {
        Node *node = createNode(stack->head, c, line);
        pushStack(stack, node);
    }
    else if (c == '}')
    {
        (stack->head != NULL && stack->head->c == '{')
            ? popStack(stack)
            : codeError(c, line);
    }
    else if (c == ']')
    {
        (stack->head != NULL && stack->head->c == '[')
            ? popStack(stack)
            : codeError(c, line);
    }
    else if (c == ')')
    {
        (stack->head != NULL && stack->head->c == '(')
            ? popStack(stack)
            : codeError(c, line);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("usage: %s <file>\n", argv[0]);
        return 1;
    }

    Stack *stack = malloc(sizeof(Stack));
    FILE *fp;
    char c;
    char cPrev; // buffer previous char to handle comments and escape char
    int line = 1;

    fp = fopen(argv[1], "r");
    if (fp == NULL)
    {
        perror("Error while opening file");
        return 1;
    }

    while ((c = fgetc(fp)) != EOF)
    {
        parseCode(stack, c, cPrev, line);
        cPrev = c;
        if (c == '\n')
            line++;
    }

    fclose(fp);

    // stack should be empty when the entire code is parsed
    if (stack->elements > 0)
    {
        for (Node *e = stack->head; e != NULL; e = e->next)
        {
            codeError(e->c, e->line);
        }

        printf("Fail: code has invalid brackets\n");
        return 1;
    }

    printf("Success: code has valid brackets\n");
    return 0;
}