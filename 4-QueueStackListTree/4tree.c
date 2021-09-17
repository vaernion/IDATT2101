#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct NodeStruct
{
    double val;
    char op;
    struct NodeStruct *left;
    struct NodeStruct *right;
} Node;

double calcTree(Node *node)
{
    double left, right;
    if (node->left != NULL)
        left = calcTree(node->left);
    if (node->left != NULL)
        right = calcTree(node->right);

    if (node->op == '+')
        return left + right;
    else if (node->op == '-')
        return left - right;
    else if (node->op == '*')
        return left * right;
    else if (node->op == '/')
        return left / right;
    else
        return node->val;
}

void printTree(Node *node)
{
    if (node->left != NULL)
    {
        printf("(");
        printTree(node->left);
    }
    if (node->op != '0')
    {
        printf("%c", node->op);
    }
    if (node->right != NULL)
    {
        printTree(node->right);
        printf(")");
    }
    if (node->val)
    {
        printf("%.0f", node->val);
    }
}

Node *createNode(double val, char op, bool isOperator)
{
    Node *node = malloc(sizeof(Node));
    if (isOperator)
        node->op = op;
    else
        node->val = val;
    return node;
}

Node *demoTree1()
{
    // (3*(2+4))/(7-(2*2)) = 6
    Node *root = createNode(0, '/', true);
    Node *l1 = createNode(0, '*', true);
    Node *r1 = createNode(0, '-', true);
    root->left = l1;
    root->right = r1;
    Node *ll2 = createNode(3, '0', false);
    Node *lr2 = createNode(0, '+', true);
    l1->left = ll2;
    l1->right = lr2;
    Node *rl2 = createNode(7, '0', false);
    Node *rr2 = createNode(0, '*', true);
    r1->left = rl2;
    r1->right = rr2;
    Node *lrl3 = createNode(2, '0', false);
    Node *lrr3 = createNode(4, '0', false);
    lr2->left = lrl3;
    lr2->right = lrr3;
    Node *rrl3 = createNode(2, '0', false);
    Node *rrr3 = createNode(2, '0', false);
    rr2->left = rrl3;
    rr2->right = rrr3;
    return root;
}

Node *demoTree2()
{
    // ((-8/4)*(5+25)) / (44-(1-2)) = -60/45 = -1.33
    Node *root = createNode(0, '/', true);
    Node *l1 = createNode(0, '*', true);
    Node *r1 = createNode(0, '-', true);
    root->left = l1;
    root->right = r1;
    Node *ll2 = createNode(0, '/', true);
    Node *lr2 = createNode(0, '+', true);
    l1->left = ll2;
    l1->right = lr2;
    Node *rl2 = createNode(44, '0', false);
    Node *rr2 = createNode(0, '-', true);
    r1->left = rl2;
    r1->right = rr2;
    Node *lll3 = createNode(-8, '0', false);
    Node *llr3 = createNode(4, '0', false);
    ll2->left = lll3;
    ll2->right = llr3;
    Node *lrl3 = createNode(5, '0', false);
    Node *lrr3 = createNode(25, '0', false);
    lr2->left = lrl3;
    lr2->right = lrr3;
    Node *rrl3 = createNode(1, '0', false);
    Node *rrr3 = createNode(2, '0', false);
    rr2->left = rrl3;
    rr2->right = rrr3;
    return root;
}

void runDemo(Node *root)
{
    double result = calcTree(root);
    printTree(root);
    printf(" = %.2f\n", result);
}

// Calculates and prints expression trees,
// using postorder traversal for calculation
// and inorder traversal for printing.
int main(int argc, char *argv[])
{
    runDemo(demoTree1());
    runDemo(demoTree2());
}