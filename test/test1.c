#include <stdlib.h>

typedef struct {
    int i;
    float f;
    char* str;
} my_struct_type;

void p(my_struct_type ** s) {
    *s = malloc(sizeof(my_struct_type));
}

int main() {
    int * i;
    
    i = malloc(sizeof(int));

    i = realloc(i, 3 * sizeof(int));

    my_struct_type * my_struct = malloc(sizeof(my_struct_type));

    double * zero_float;
    zero_float = calloc(1, sizeof(double));

    p(&my_struct);

    return 0;
}
