#include <iostream>
#include <stdlib.h>
#include "helper.h"
#include "crt.h"
#include "xor.h"
#include "Poly.h"
#include <limits.h>

#define MAX_DEGREE 10

static int conflict_counter=0; //for debuging and testing

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
        uint64_t result=1;
        int idx=0;
        //check too big? ret 0;
        for(int i=0;i<parties;i++){
            idx=indexes[i];
            if(idx!=-1){
                result=result*primes[idx];
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
        uint64_t prime_result=primes[func->evaluate(value)];
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

    short can_store(Poly* func, uint64_t value, uint64_t m_sum, uint64_t m_withou_i){
        short ret_val;
        long temp=func->T;
        func->T=t_size;

        long position=func->evaluate(value);
        long incre=1;
        while(hash[position].active!=0){
            position=(position+(incre*incre))%t_size;
            incre++;
            if(incre>128){
                func->T=temp;
                return 0;
            }
        }
        func->T=temp;
        return 1;
    }

    short store(Poly* func, uint64_t value, uint64_t m_sum, uint64_t m_withou_i){
        short ret_val;
        long temp=func->T;
        func->T=t_size;

        long position=func->evaluate(value);
        long incre=1;
        while(hash[position].active!=0){
            position=(position+(incre*incre))%t_size;
            incre++;
            if(incre>128){
                func->T=temp;
                return 0;
            }
        }
        hash[position].m_sum=m_sum;
        hash[position].sum_without_i=m_withou_i;
        hash[position].active=1;
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
        //retrieval not right answer
        uint64_t final_answer=retrieval(out_key,hash[loc].sum_without_i,k_sum);
        final_answer=final_answer ^ index_key;

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
        func=new Poly(n-1,p_size); //beware the size if 256 whereas table is 1024
        for(int i=0;i<n;i++){
            func->setCoeff(key[i],i);
        }

        secret_k=(uint64_t)(key[rand()%n])<<32|key[rand()%n];
        index_key=(uint64_t)rand()<<32|rand();
        table=new hash_table(table_size);
        prime_size=p_size;

        this->ID=id;
    }

    void print_view(){
        cout<<"print out party "<<this->ID<<"'s table information"<<endl;
        for(int i=0;i<this->table->t_size;i++){
            if(this->table->hash[i].active==1){
                cout<<"Index "<<i<<" : "<<"sum m ="<<this->table->hash[i].m_sum<<" m w/out i = "<<this->table->hash[i].sum_without_i<<endl;
            }
            else{
                cout<<"Index "<<i<<" : value not stored"<<endl;
            }
        }
    }

    short can_store(uint64_t out_key, uint64_t sum_m, uint64_t m){
        return table->can_store(this->func,out_key,sum_m,sum_m^m);
    }

    short store(uint64_t out_key, uint64_t sum_m, uint64_t m){
        return table->store(this->func,out_key,sum_m,sum_m^m);
    }

    short retrieve(uint64_t out_key, uint64_t index, int prime_remainder_pair[], uint64_t k_sum){
        //store the result in prime remainder pair if valid, else return 0
        if((index&(1<<this->ID))==0){return 0;} //not your turn
        return this->table->retrieve(out_key,prime_remainder_pair,k_sum,this->func,this->index_key);
    }

    //finish the retrieve function requires k_sum, m_sum, 

    uint64_t gen_m(prime_hashes* ph, int idx, uint64_t val){
        //hashed result ^ index_key 
        uint64_t prime=ph->primes[idx];
        uint64_t remainder=val%prime;
        uint64_t piri=(prime<<32|remainder);
        uint64_t res=piri ^ index_key; //m(i)
        uint64_t debug=res ^ index_key;
        return res;
    }

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
            select_primes(ph->primes,temp_key,degree,256);
            this->parties[i].initialize(temp_key,degree+1,1024,256,i);
            delete[] temp_key;
        }

        //communicating the sum of K I guess?
        sum_key=(uint64_t)rand()<<32|rand();
        uint64_t temp=sum_key;
        for(int i=0;i<parties;i++){
            sum_key^=this->parties[i].secret_k;
        }
        sum_key=sum_key^temp;
    }

    void print_table_view(uint64_t party_bit_map=0xFFFFFFFFFFFFFFFF){
        //print each party's table simulate attacker's view
        node* current;
        for(int i=0;i<64;i++){
            if((party_bit_map & (1<<i))==1){
                current=&this->parties[i];
                current->print_view();
                cout<<endl;
            }
        }
    }



    short store(int party_index[],int party_size,uint64_t val, uint64_t result[2]){
        int* hash_index=new int[party_size];
        int* collision=new int[party_size];

        memset(collision,-1,party_size*sizeof(int));
        memset(hash_index,-1,party_size*sizeof(int));
        node* current;
        long id;
        for(int i=0;i<party_size;i++){
            current=&(this->parties[party_index[i]]);
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
            else{ //bug here
                ph->lock[hash_index[place]]=0;

                hash_index[place]++;
                conflict_counter++;
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
                }
            }
        }

        //now we have a useable hash indexes indicating each parties 
        //generate individual hashes and all the associated keys check if we can store for table (no error)


        //we get m, total key then and communicate the general m to save to the place 
        uint64_t* all_m=new uint64_t[party_size];
        int* final_party=new int[party_size]; memset(final_party,-1,party_size*sizeof(int));

        memset(all_m,0,sizeof(uint64_t)*party_size);

        for(int i=0;i<party_size;i++){
            current_idx=hash_index[i];
            if(current_idx!=-1){
                current=&this->parties[party_index[i]];//get the party associated with it
                final_party[i]=party_index[i]; //get the final party size and nodes;
                all_m[i]=current->gen_m(ph,current_idx,val);
            }
        }
        uint64_t sum_m=gen_sum_m(all_m,party_size);
        
        //check if individual hash table has issues 
        for(int i=0;i<party_size;i++){
            current_idx=hash_index[i];
            if(current_idx!=-1){
                current=&this->parties[party_index[i]]; //get the party associated with it
                
                if(current->can_store(sum_m^sum_key,sum_m,all_m[i])!=1){
                    //local storage failure
                    delete[] hash_index;
                    delete[] collision;
                    delete[] all_m;
                    delete[] final_party;
                    return 0;
                };
            }
            //else continue the next loop, the party is not being used yet
        }

        //now we finally store something
        for(int i=0;i<party_size;i++){
            current_idx=hash_index[i];
            if(current_idx!=-1){
                current=&this->parties[party_index[i]]; //get the party associated with it
                current->store(sum_m^sum_key,sum_m,all_m[i]);
            }
            //else continue the next loop, the party is not being used yet
        }
    
        result[0]= gen_party_indexes(final_party,party_size); //needs debugging 
        result[1]= sum_m ^ sum_key; //the out_key

        delete[] hash_index;
        delete[] collision;
        delete[] all_m;
        delete[] final_party;
        return 1;
    }

    uint64_t retrieve(uint64_t parties, uint64_t out_key){
        int value_storage[]={0,0};
        long primes[64],remainder[64];
        int counter=0; uint64_t lcm=1;
        for(int i=0;i<64;i++){
            if(this->parties[i].retrieve(out_key,parties,value_storage,this->sum_key)==1){
                primes[counter]=value_storage[0];
                lcm*=value_storage[0];
                remainder[counter]=value_storage[1];  
                counter++;
            };
        }
        return CRT(lcm,primes,remainder,counter);
    }

    ~dv_hash(){
        delete ph;
        delete[] parties;
    }
};





//testing scripts
void gen_rand_test_group(int array[]){
    uint32_t x=(rand()^rand());
    int counter=0;;
    for(int i=0;i<32;i++){
        if((x & 1<<i) != 0){
            array[counter]=i;
            counter++;
        }
    }
}

//this is the main function

int main(){
    int failed=0;
    int success=0;
    dv_hash hash_t(27);
    int pt_idx[32]; memset(pt_idx,0,32*sizeof(int));
    uint64_t res[2]; memset(res,0,2*sizeof(uint64_t));
    for(int i=0;i<200;i++){
        gen_rand_test_group(pt_idx);
        if(hash_t.store(pt_idx,3,4,res)==0){failed++;}
        else{
            success++;
            //cout<<"Resulting Store: "<<res[0]<<" "<<res[1]<<endl;
        }
    }

    cout<<"failed Count: "<<failed<<endl;
    cout<<"Success Count: "<<success<<endl;
    cout<<"Conflict Counter: "<<conflict_counter<<endl;

    //cout<<hash_t.retrieve((uint64_t) 8 )
    return 0;
}