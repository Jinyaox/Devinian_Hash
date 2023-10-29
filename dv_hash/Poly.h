#ifndef POLY_H

#define POLY_H


#define MAXPOLY 50

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
using namespace std;

class Poly
{

private:
    // Data members   [implementation of ADT's data object]
    long k; //the degree of polynomial

    // array for holding the coefficients of the poly
    long coeff[MAXPOLY];              
public:
    long T; //the cardinality of modular T

    // Default Class constructor: initializes a polynomial to the constant 0
    Poly (long degree, long T){
        this->k=degree;

        for(int i=0;i<MAXPOLY;i++){
            //to initialize the whole thing
            coeff[i]=0; 
        }

        this->T=T;
    }

    // degree: finds the degree of a polynomial (the highest power with a non-zero coefficient)
    long degree () const{
        return this->k;
    }

    // setCoeff: sets a term, value*x^i, in a polynomial
    void setCoeff (long value, long i){
        coeff[i]=value;
    }

    // retrieveCoeff: finds the coefficient of the x^i term in poly
    // Throws <std::out_of_range> if index i does not meet the precondition.
    long retrieveCoeff (long i) const{
        return coeff[i];
    }


    long eval_a_term(long coefficient, long x, long power){
        if(power==1){
            return ((coefficient%T)*(x%T))%T;
        }else{
            long temp=eval_a_term(coefficient,x,power-1) % T;
            return ((x%T)*(temp%T))%T;
        }
    }

    // evaluate: evaluate a polynomial for a specified value of X
    long evaluate (long x){ //things not right here

        long result=0;
        int degree=(this->k+1);
        long temp;
        for(long i=1;i<degree;i++){
            temp=eval_a_term(this->retrieveCoeff(i),x,i);
            result=result+temp;
        }
        return result % T;
    }
};  

#endif