#include "util.h"
#include <stdlib.h>
#include <time.h>

void shuffle_array(int array[], const int length) {
    if (length > 1) {
        static unsigned int seed = 0;
        if (seed == 0) {
            seed = (unsigned int)time(nullptr);
        }
        
        for (size_t i = 0; i < length - 1; i++) {
            const size_t j = i + rand_r(&seed) / (RAND_MAX / (length - i) + 1);
            const int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}