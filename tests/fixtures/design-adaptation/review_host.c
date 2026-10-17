#include <stdio.h>
#include "review_checks.h"
int main(void) {
    int failure = review_checks();
    if (failure) fprintf(stderr, "Review contract failed at line %d\n", failure);
    else printf("%lu\n", review_trace);
    return failure ? 1 : 0;
}
