#include <iostream>
#include <stdlib.h>
#include "helper.h"
#include "crt.h"
#include "xor.h"
#include "Poly.h"
#include <limits.h>

#define MAX_DEGREE 10

typedef struct hash_store{
    uint8_t active;
    uint64_t m_sum;
    uint64_t sum_without_i;
}hash_store;

//change all these to long! 

int cmpfunc_min (const void * a, const void * b) {
   return -( *(int*)a - *(int*)b );
}

/*helper functions*/
long find_max(int array[],int table_size, int computation_size){
   qsort(array, table_size, sizeof(int), cmpfunc_min); 

   long product=0;

   for(int i=0;i<computation_size;i++){
    product=product*array[i];
   }
   return product;
}

class prime_hashes{
    public:
    
    int* primes;
    short* lock;
    unsigned size;


    prime_hashes(unsigned size, unsigned starting=2){
        this->size=size;
        this->primes= new int[size];
        this->lock=new short[size];
        memset(this->lock,0,size*sizeof(short));
        gen_n_primes(size,this->primes,starting);
    }

    short can_gen(int indexes[], uint64_t value, int parties){
        uint64_t result;
        //check too big? ret 0;
        for(int i=0;i<parties;i++){
            if(indexes[i]!=-1){
                result=result*primes[indexes[i]];
                if(result>=value){
                    return 1; //(ok)
                }
            }
        }
        return 0;
    }

    short locked(int index){
        return this->lock[index]==1;
    }

    uint64_t gen_hash(Poly* func, long value){
        uint32_t prime_result=primes[func->evaluate(value)];
        uint32_t remainder_result=value%prime_result;
        return prime_result<<32|remainder_result;
    }

    int largest(){
        return this->primes[size-1];
    }

    ~prime_hashes(){
        delete[] this->primes;
        delete[] this->lock;
    }
};

class hash_table{
    public: 
    hash_store* hash;
    unsigned t_size;

    int current_value; //to check if it's too small or too big for hash

    hash_table(unsigned size){
        this->t_size=size;
        hash= new hash_store[size];
        memset(hash,0,sizeof(hash_store)*size);
    }

    short store(Poly* func, uint64_t value, uint64_t m_sum, uint m_withou_i){
        short ret_val;
        long temp=func->T;
        func->T=t_size;

        long position=func->evaluate(value);
        long incre=1;
        while(hash[position].active=0){
            position=(position+(incre*incre))%t_size;
            incre++;
            if(incre>128){
                func->T=temp;
                return 0;
            }
        }
        hash[position].m_sum=m_sum;
        hash[position].sum_without_i=m_withou_i;
        func->T=temp;
        return 1;
    }

    short retrieve(uint64_t out_key, int prime_remainder_pair[], uint64_t k_sum, Poly* func, uint64_t index_key){
        short ret_val;
        long temp=func->T;
        func->T=t_size;
         
        unsigned loc=func->evaluate(out_key);
        uint64_t m_sum=hash[loc].m_sum;
        unsigned incre=1;

        while((out_key^m_sum)!=k_sum){
            loc=(loc+(incre*incre))%t_size;
            incre++;
            if(incre>128){
                func->T=temp;
                return 0; //not expected to return 0;
            }
        }
        uint64_t final_answer=retrieval(out_key,hash[loc].sum_without_i,k_sum) ^ index_key;

        prime_remainder_pair[0]=final_answer>>32;
        prime_remainder_pair[1]=final_answer & 0x00000000FFFFFFFF;
        func->T=temp;
        return 1;
    }

    ~hash_table(){
        delete[] hash;
    }
};

class node{
    public:
    uint64_t index_key; //the mask thing for m1 m2 m3 hash (a mask)
    uint64_t secret_k; //the key for storage
    short ID;
    int prime_size;
    int key_size; //the degree of the polynomial 
    hash_table* table;
    Poly* func;


    node(){}

    void initialize(int key[],int n,int table_size, int p_size, short id){
        key_size=n;
        func=new Poly(n,table_size);
        for(int i=0;i<=n+1;i++){
            func->setCoeff(key[i],i);
        }

        secret_k=key[rand()%n]<<32|key[rand()%n];
        index_key=rand()<<32|rand();
        table=new hash_table(table_size);
        prime_size=p_size;

        this->ID=id;
    }

    short store(uint64_t out_key, uint64_t sum_m, uint64_t m){
        return table->store(this->func,out_key,sum_m,sum_m^m)==1;
    }

    short retrieve(uint64_t out_key, uint64_t index, int prime_remainder_pair[], uint64_t k_sum){
        //store the result in prime remainder pair if valid, else return 0
        if(index&(1<<this->ID)==0){return 0;} //not your turn
        return this->table->retrieve(out_key,prime_remainder_pair,k_sum,this->func,index);
    }

    //finish the retrieve function requires k_sum, m_sum, 

    ~node(){
        delete func;
        delete table;
    }
};



class dv_hash{
    
    private:
    node* parties;
    prime_hashes* ph;
    uint64_t sum_key;

    public:

    dv_hash(int parties, int degree=3){
        ph=new prime_hashes(256); //256 for now
        this->parties= new node[parties];

        for(int i=0;i<parties;i++){
            int* temp_key=new int[degree+1]; //poly of 3 for now 
            select_primes(ph->primes,temp_key,degree+1,256);
            this->parties[i].initialize(temp_key,degree+1,1024,256,i);
            delete[] temp_key;
        }

        //communicating the sum of K I guess?
        sum_key=rand()<<32|rand();
        uint64_t temp=sum_key;
        for(int i=0;i<parties;i++){
            sum_key^this->parties[i].secret_k;
        }
        sum_key=sum_key^temp;
    }


    short store(int party_index[],int party_size,uint64_t val){
        int* hash_index=new int[party_size];
        int* collision=new int[party_size];

        memset(collision,-1,party_size*sizeof(int));
        memset(hash_index,-1,party_size*sizeof(int));
        node* current;
        long id;
        for(int i=0;i<party_size;i++){
            current=&parties[party_index[i]];
            id=current->func->evaluate(val);
            if(ph->lock[id]==0){
                hash_index[i]=id;
                ph->lock[id]=1;
            }
            else{
                collision[i]=id;
            }
        }

        //resolve hash issues, see if everything is fine for the value(ie. not too big or small)

        int place=0; //the iterator for the normal parties (use this first)
        int collision_substitute=0; //iterator for substituting party
        int current_idx;

        while(ph->can_gen(hash_index,val,party_size)==0){ //see if the current hash indexes are sufficiently large

            if(place>=party_size){ //exhausted all the possible existing parties

                while(collision[collision_substitute]==-1){ //find the next 

                    //the if condition checks if we used all possible values
                    if(collision_substitute>=party_size){
                        return 0; //cannot do it sadly
                    }

                    collision_substitute++;
                }
                hash_index[collision_substitute]=collision[collision_substitute];
                while((ph->lock[hash_index[collision_substitute]]==1)&&(hash_index[collision_substitute]<ph->largest())){
                    hash_index[collision_substitute]++;
                }

            }
            else{
                ph->lock[hash_index[place]]==0;

                while((ph->lock[hash_index[place]]==1)&&(hash_index[place]<ph->largest())){
                    hash_index[place]++;
                }

                ph->lock[hash_index[place]]=1;
                
                while(hash_index[place]==-1){ //move to the next place for hash index
                    if(place>=party_size){
                        break;
                    }
                    else{
                        place++;
                    }
                };
            }
        }

        //now we have a useable hash indexes indicating each parties 
        //generate individual hashes and all the associated keys

        //we get m, total key then and communicate the general m to save to the place 



        for(int i=0;i<party_size;i++){
            current_idx=hash_index[i];
            if(current_idx!=-1){
                current=&this->parties[party_index[i]];//get the party associated with it
                current->store()
            }
            //else continue the next loop, the party is not being used yet
        }




        delete[] hash_index;
        delete[] collision;
    }


};


int main(){
    return 0;
}