#include "string.h"
#include "aes.h"
#include "stdlib.h"
#include "stdio.h"
#include "../xxHash/xxhash.h"
#define HASH_SPACE 1024
#define SEED 5328

// clang main.c aes.h ../xxHash/libxxhash.0.8.2.dylib -o dv_hash_test


//the AES benchmark
int store(int* hash,char* buf,XXH32_hash_t seed){
    int location=(uint64_t)XXH32(buf,4,seed) % HASH_SPACE;
    unsigned counter=1;
    while(hash[location]!=0){
      location=(location+counter*counter)%HASH_SPACE;
      counter++;
      if(counter>128){
        return -1;
      }
    }
    return location;
}

int main(void)
{
    static struct tc_aes_key_sched_struct s;
    //initialize the key structure
    char buf[5]="abbc";
    int hash[1024];
    memset(hash,0,sizeof(int)*HASH_SPACE);
    tc_aes128_set_encrypt_key(&s,(uint8_t*)"HELLO");

    //encryption test
    for(int i=0;i<5;i++){
      for(int j=0;j<(2<<i);j++){
        tc_aes_encrypt((uint8_t *)buf,(uint8_t *)buf,&s);
        uint64_t loc=store(hash,buf,SEED);
        if(loc!=-1){hash[loc]=1;}
      }
    }

	return 0;
}