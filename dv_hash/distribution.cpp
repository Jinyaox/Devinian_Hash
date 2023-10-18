#include <iostream>
#include <stdlib.h>
#include "Poly.h"
#include "helper.h"


#define TABLE_SIZE 256

int main(){

    FILE *fptr;
    fptr = fopen("distribution.csv", "w");


    int h1[8];
    int h2[9];
    int h3[5];

    int p1[128];
    gen_n_primes(128,p1);
    
    //add parties
    select_primes(p1,h1,8,128);
    select_primes(p1,h2,9,128);
    select_primes(p1,h3,5,128);


    //test the hash distribution
    int hash_table[TABLE_SIZE];
    memset(hash_table,0,256*sizeof(int));

    Poly func1(8,TABLE_SIZE);

    for(int i=0;i<8;i++){
        func1.setCoeff(h1[i],i);
    }

    Poly func2(9,TABLE_SIZE);

    for(int i=0;i<9;i++){
        func2.setCoeff(h1[i],i);
    }

    Poly func3(5,TABLE_SIZE);

    for(int i=0;i<5;i++){
        func3.setCoeff(h1[i],i);
    }

    int item;

    for(int i=0;i<65536;i++){
        item=rand();
        hash_table[func1.evaluate(item)]++;
        hash_table[func2.evaluate(item)]++;
        hash_table[func3.evaluate(item)]++;
    }

    double percentage;

    for(int i=0;i<TABLE_SIZE;i++){
        fprintf(fptr, "%d,",hash_table[i]);
        cout<<"location "<<i<<" "<<hash_table[i]<<endl;
        cout<<"location "<<i<<" "<<percentage<<endl;
        cout<<endl;
    }

    return 0;
}