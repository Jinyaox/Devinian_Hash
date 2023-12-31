#include <iostream>
#include <stdlib.h>
#include "crt.h"
#include "xor.h"
#include "helper.h"
#include "cuckoo.cpp"
#include "../xxHash/xxhash.h"
#include <limits.h>
#include <chrono>

//compile with clang++ dv_hash_Cuckoo.cpp ../xxHash/libxxhash.0.8.2.dylib -o dv_cuckoo

typedef struct receipt{
    uint64_t bitmap;
    uint64_t outkey;
}receipt;

typedef struct storage_info{
    short table;
    int location;
}storage_info;

class prime_hashes{
    public:
    
    int* primes;
    int* lock;
    int size;


    prime_hashes(unsigned size, unsigned starting=2){
        this->size=size;
        this->primes= new int[size];
        this->lock=new int[size];
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

    void clear_lock(){
        //clear all locks in this thing
        memset(this->lock,0,this->size*sizeof(int));
    }

    int largest(){
        return this->primes[size-1];
    }

    ~prime_hashes(){
        delete[] this->primes;
        delete[] this->lock;
    }
};

class node{
    public:
    uint64_t index_key; //the mask thing for m1 m2 m3 hash (a mask)
    uint64_t verifier;
    short ID;
    int place;
    int prime_size;
    CuckooHash* table;
    XXH64_hash_t seed1;
    XXH64_hash_t seed2;

    node(){
        seed1 =(XXH64_hash_t) rand();
        seed2 =(XXH64_hash_t) rand();
    }

    void init(int table_size, int p_size, short id, uint64_t verifier){
        this->ID=id;
        this->index_key=(uint64_t)rand()<<32|rand();;
        prime_size=p_size;
        this->verifier=verifier;
        table=new CuckooHash(table_size,seed1,seed2,verifier,index_key);
        table->InitHashTable();
    }

    int eval(int val,int max_prime){
        return (int)((uint64_t) XXH64(&val,4,seed1) % max_prime);
    }

    int store(uint64_t out_key, uint64_t sum_m, uint64_t m,short* tab){
        return table->Insert(out_key,sum_m^m,tab);
    }

    short retrieve(uint64_t out_key, uint64_t index, int prime_remainder_pair[]){
        //store the result in prime remainder pair if valid, else return 0

        uint64_t mask=((uint64_t)1)<<this->ID;
        if((index&mask)==0){return 0;} //not your turn
        hash_store* res=table->retrieve(out_key);

        if (res !=NULL){
            hash_store result=*res;
            uint64_t piri=result.m_sum^seed1^result.sum_without_i; //this contains a bug
            prime_remainder_pair[0]=piri>>32;
            prime_remainder_pair[1]=piri & 0x00000000FFFFFFFF;
            return 1;
        }
        return 0;
    }

    //finish the retrieve function requires k_sum, m_sum, 

    uint64_t gen_m(prime_hashes* ph, int idx, uint64_t val){
        uint64_t prime=ph->primes[idx];
        uint64_t remainder=val%prime;
        uint64_t piri=(prime<<32|remainder);
        //cout<<piri<<endl;
        uint64_t res=piri ^ index_key;
        return res;
    }

    void remove(int table_idx,int loc){
        this->table->remove(table_idx-1,loc);
    }

    ~node(){
        delete table;
    }
};



class dv_hash{
    
    private:
    node* parties;
    prime_hashes* ph;
    uint64_t verify_key;

    public:

    dv_hash(int parties, int degree=3){
        ph=new prime_hashes(256);
        this->parties= new node[parties];
        verify_key=(uint64_t)rand()<<32|rand();
        for(int i=0;i<parties;i++){
            this->parties[i].init(1024,256,i,verify_key);
        }
    }

    void revert(storage_info* storage, int party_size){
        node* current;
        for(int i=0;i<party_size;i++){
            if((storage[i].table!=0)&&(storage[i].location!=0)){
                current=&this->parties[i];
                current->remove(storage[i].table,storage[i].location);
            }
        }
    }


    short store(int party_index[],int party_size,uint64_t val, uint64_t result[2]){
        int hash_index[64];
        int collision[64];

        memset(collision,-1,party_size*sizeof(int));
        memset(hash_index,-1,party_size*sizeof(int));
        ph->clear_lock();
        node* current;
        int id;
        for(int i=0;i<party_size;i++){
            current=&(this->parties[party_index[i]]);
            id=current->eval(val,256);
            if(ph->lock[id]!=1){
                hash_index[i]=id;
                ph->lock[id]=1;
            }
            else{
                collision[i]=id;
            }
        }

        int place=0; //the iterator for the normal parties (use this first)
        int collision_substitute=0; //iterator for substituting party
        int current_idx;
        int debug=0;
        int revert=0; //make sure clear it

        while(ph->can_gen(hash_index,val,party_size)==0){ //see if the current hash indexes are sufficiently large

            if(place>=party_size){ 
                cout<<1<<endl;
                debug = collision[collision_substitute];

                while(debug==-1){ //find the next 

                    //the if condition checks if we used all possible values
                    if(collision_substitute>=party_size){
                        return 0; //cannot do it sadly
                    }

                    collision_substitute++;
                    debug=collision[collision_substitute];
                }
                hash_index[collision_substitute]=debug;
                
                while((ph->lock[hash_index[collision_substitute]]==1)&&(hash_index[collision_substitute]<(ph->size)-1)){
                    hash_index[collision_substitute]++;
                }
                ph->lock[hash_index[collision_substitute]]=1;
                collision_substitute++;

            }
            else{
                ph->lock[hash_index[place]]=0;
                revert=hash_index[place];
                hash_index[place]++;

                while((ph->lock[hash_index[place]]==1)&&(hash_index[place]<(ph->size)-1)){
                    hash_index[place]++;
                }

                if((hash_index[place]>=ph->size)){
                    ph->lock[revert]=1;
                    hash_index[place]=revert;
                    place++;
                }else{
                    ph->lock[hash_index[place]]=1;
                }
                
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

        uint64_t all_m [64];
        int final_party[64];

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
        storage_info* storage=new storage_info[party_size];
        
        //check if individual hash table has issues 
        for(int i=0;i<party_size;i++){
            current_idx=hash_index[i];
            if(current_idx!=-1){
                current=&this->parties[party_index[i]]; //get the party associated with it 
                storage[i].location=current->store(sum_m^verify_key,sum_m,all_m[i],&storage[i].table);              
                if(storage[i].location==-1){
                    this->revert(storage,party_size);
                    delete[] storage;
                    return -1;
                }
            }
            //else continue the next loop, the party is not being used yet
        }
    
        result[0]= gen_party_indexes(final_party,party_size); 
        result[1]= sum_m ^ verify_key;

        delete[] storage;
        return 1;
    }

    //needs to fix this
    uint64_t retrieve(uint64_t parties, uint64_t out_key){
        int value_storage[]={0,0};
        uint64_t primes[64],remainder[64]; //1522395419
        int counter=0; uint64_t lcm=1; uint64_t OLD_lcm=1; //prevent LCM overflow
        for(int i=0;i<64;i++){
            if(this->parties[i].retrieve(out_key,parties,value_storage)==1){
                remainder[counter]=value_storage[1]; 
                primes[counter]=value_storage[0]; 
                counter++;
            };
        }
        return CRT(primes,remainder,counter);
    }

    ~dv_hash(){
        delete ph;
        delete[] this->parties;
    }
};















//testing scripts
int gen_rand_test_group(int array[]){
    uint32_t x=(rand()^rand());
    int counter=0;;
    for(int i=0;i<32;i++){
        if((x & 1<<i) != 0){
            array[counter]=i;
            counter++;
        }
    }
    return counter;
}

void store_test(){
    int failed=0;
    int success=0;
    int group_num;
    int pt_idx[64]; memset(pt_idx,0,64*sizeof(int));
    uint64_t res[2]; memset(res,0,2*sizeof(uint64_t));

    //timed unit for store
    for(int epoch=0; epoch<11; epoch++){
        dv_hash hash_t(64);
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        for(int i=0;i<(int)pow(2,epoch);i++){
            group_num=gen_rand_test_group(pt_idx); 
            if(hash_t.store(pt_idx,group_num,rand(),res)==0){
                failed++;
            }
            else{
                success++;
            }
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    }

    cout<<"failed Count: "<<failed<<endl;
    cout<<"Success Count: "<<success<<endl;
}

void retrieve_test(int amount=512){
    dv_hash hash_t(64);
    int group_num;
    int pt_idx[64]; memset(pt_idx,0,64*sizeof(int));
    uint64_t res[2]; memset(res,0,2*sizeof(uint64_t));
    receipt cache[1024];
    
    for(int i=0;i<1024;i++){
        group_num=gen_rand_test_group(pt_idx);
        uint32_t val= rand();
        //cout<<val<<endl;
        hash_t.store(pt_idx,group_num,val,res);
    }
    for(int loop=9;loop<10;loop++){
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        for(int i=0;i<(2<<loop);i++){
            hash_t.retrieve(cache[i].bitmap,cache[i].outkey);
        }
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        //std::cout << "Time difference for " << (2<<loop) << " = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    }
}

int main(){
    retrieve_test();
    return 0;
}
