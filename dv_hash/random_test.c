#include "time.h"
#include "string.h"
#include "/Users/xuian/Desktop/vcpkg/installed/x64-osx/include/xxhash.h"
#define SPACE 1024
#define DEGREE 4
#define START 31
#define TOTAL_TEST 16384

// void base_performance(){
//     int space[SPACE];
//     memset(space,0,1024*sizeof(int));
//     for(int i=0;i<TOTAL_TEST;i++){
//         space[rand()%SPACE]++;
//     }
// }

void kwise_performance(){
    int space[1024];
    memset(space,0,sizeof(int));
    XXH32_hash_t seed=(XXH32_hash_t) 176;

    for(int i=0;i<TOTAL_TEST;i++){
        XXH32_hash_t hash=XXH32(&i,SPACE,seed);
        space[(int)hash]++;
    }
}

int main(){
    kwise_performance();
}