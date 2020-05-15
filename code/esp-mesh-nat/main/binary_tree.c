#include "binary_tree.h"

void* get(struct node* node, int key){
    if(node == NULL){
        return NULL;
    }
    if(node->key == key){
        return node->data;
    }
    if(node->key > key){
        return get(node->left, key);
    }else{
        return get(node->right, key);
    }
}

struct node* insert(struct node* tree, int key, void* data){
    if(tree == NULL){
        return newNode(key, data);
    }

    if(key < tree->key){
        tree->left = insert(tree->left, key, data);
    }else if(key > tree->key){
        tree->right = insert(tree->right, key, data);
    }

    return tree;
}

struct node *newNode(int key, void* data){
    
    struct node* node  = (struct node*) malloc(sizeof(struct node));
    
    node->key=key;
    node->data=data;
    node->left=NULL;
    node->right=NULL;

    return node;
}

void print_inorder(struct node* root){
    if(root == NULL) return;

    print_inorder(root->left);
    printf("(key:%d; data: %p)\n", root->key, root->data);
    print_inorder(root->right);
}

void free_tree(struct node* root){
    if(root != NULL){
        free_tree(root->left);
        free_tree(root->right);
        //free(root->data);
        free(root);
    }
}

int main(){

    struct node* root;
    root = newNode(1, "one");
    insert(root, 2, "two");
    insert(root, 0, "zero");
    print_inorder(root);
   
}