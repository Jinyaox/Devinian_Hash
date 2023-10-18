#ifndef __CRT__
#define __CRT__
#include <stdlib.h>
#include <iostream>
using namespace std;

long exgcd(long a, long b, long &x, long &y)
{
    if (b == 0)
    {
        x = 1;
        y = 0;
        return a;
    }
    long d = exgcd(b, a % b, y, x);
    y -= (a / b) * x;
    return d;
}

long inv(long a, long p)
{
    long x, y;
    exgcd(a, p, x, y);
    return (x % p + p) % p;
}

long compute(long lcm, long prime, long number)
/*
    the function takes a prime number and a number. It computes the number to share for summation
*/
{
    long r = lcm / prime;
    long b= number % prime;
    return (b * r * inv(r, prime)) % lcm;
}


long CRT(long lcm, long prime[], long number,long n){ //proof of concepts
    long sum=0;
    for(int i=0;i<n;i++){
        sum=sum+compute(lcm,prime[i],number);
    }
    return sum % lcm;
}

#endif