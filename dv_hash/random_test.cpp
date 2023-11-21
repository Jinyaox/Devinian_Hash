#include "Poly.h"
#include "helper.h"
#include "stdio.h"
#include "iostream"
#include "time.h"
#define SPACE 1024
#define DEGREE 4
#define START 31
#define TOTAL_TEST 16384

void base_performance(){
    int space[SPACE];
    memset(space,0,1024*sizeof(int));
    for(int i=0;i<TOTAL_TEST;i++){
        space[rand()%SPACE]++;
    }
    for(int i=0;i<SPACE;i++){
        cout<<i<<","<<space[i]<<endl;
    }
}

void kwise_performance(long coe[]){
    int space[SPACE];
    Poly test(DEGREE,SPACE);
    int largest_prime=(int)coe[DEGREE];
    memset(space,0,1024*sizeof(int));

    for(int i=0;i<=DEGREE;i++){
        test.setCoeff(coe[i],i);
    }

    for(int i=0;i<TOTAL_TEST;i++){
        space[test.eval((uint64_t)rand(),largest_prime)]++;
    }

    for(int i=0;i<SPACE;i++){
        cout<<i<<","<<space[i]<<endl;
    }
}

int main(){
    //the following lines test the polynomial k-wise Uniformity
    int coe[DEGREE+2];
    gen_n_primes(DEGREE,coe,START);
    // cout<<"testing K-wise Performance"<<endl;
    // kwise_performance((long*)coe);

    cout<<"testing Base Benchmark"<<endl;
    base_performance();
}