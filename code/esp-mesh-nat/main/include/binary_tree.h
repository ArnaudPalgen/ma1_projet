#include <stdio.h>
#include <stdlib.h>

struct node{
    int key;
    void* data;
    struct node* left;
    struct node* right;
};

void print_inorder(struct node* root);

struct node *newNode(int key, void* data);

struct node* insert(struct node* tree, int key, void* data);

void* get(struct node* node, int key);