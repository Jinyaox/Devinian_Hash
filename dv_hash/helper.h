#ifndef __HELPER__
#define __HELPER__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_set>
#include <cmath>
using namespace std;

long calc_permutations(int size){
    //size must be greater than 0
    if(size==1){
        return 1;
    }
    else{
        return size * calc_permutations(size-1);
    }
}

int primeInRange(int L, int R, int table[])
{
    int i, j, flag;
    int counter=0;
 
    // Traverse each number in the
    // interval with the help of for loop
    for (i = L; i <= R; i++) {
 
        // Skip 0 and 1 as they are
        // neither prime nor composite
        if (i == 1 || i == 0)
            continue;
 
        // flag variable to tell
        // if i is prime or not
        flag = 1;
 
        // Iterate to check if i is prime
        // or not
        for (j = 2; j <= i / 2; ++j) {
            if (i % j == 0) {
                flag = 0;
                break;
            }
        }
 
        // flag = 1 means i is prime
        // and flag = 0 means i is not prime
        if (flag == 1){
            table[counter]=i;
            counter++;
        }
    }
    return counter; //return how many has been counted
}

int gen_n_primes(int n,int table[], int start=2)
{
    int j, flag;
    int counter=0;
    int sequence=start;
 
    // Traverse each number in the
    // interval with the help of for loop
    while(counter<n){
        flag=1;
        for (j = 2; j <= sequence / 2; ++j) {
            if ((sequence % j) == 0) {
                flag = 0;
                break;
            }
        }
 
        // flag = 1 means i is prime
        // and flag = 0 means i is not prime
        if (flag == 1){
            table[counter]=sequence;
            counter++;
        }
        sequence++;
    }
    return counter; //return how many has been counted
}


void select_primes(int primes[],int table[],int k,int prime_size){

   int item;
   
   std::unordered_set<int> dup_check;

   for(int i=0;i<k+1;i++){
    do{
        item=primes[rand()%prime_size];
    }while(dup_check.count(item)!=0);

    dup_check.insert(item);

    table[i]=item;
   }
    return;
}

#endif