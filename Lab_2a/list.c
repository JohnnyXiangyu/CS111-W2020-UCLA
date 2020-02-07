#include <stdio.h>
#include <stdlib.h>

struct list {
    struct list* previous;
    struct list* next;
    int data;
};

int main() {
    struct list temp;
    temp.data = 0;
    temp.previous = NULL;
    temp.next = NULL;
}