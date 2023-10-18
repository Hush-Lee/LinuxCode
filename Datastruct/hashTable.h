#include <functional>
#include <iostream>
#include <utility>
template <class K, class E>
class HashTable {
    std::pair<const K, E>** table;
    std::hash<K> hash;
    int dSize;
    int divisor;
public:
    HashTable(int theDivisor);
    int sercher(const K&theKey)const;
    void insert(const std::pair<const K, E>&thePair);
    std::pair<const K, E>* find(const K&theKey)const;
};

template <class K, class E>
HashTable<K, E>::HashTable(int theDivisor) {
    divisor = theDivisor;
    dSize = 0;
    table = new std::pair<const K, E>*[divisor];
    for(int i = 0; i < divisor;++i){
        table[i] = nullptr;
    }
}

template<class K,class E>
int HashTable<K, E>::sercher(const K&theKey)const{
    int i=(int)std::hash(theKey)%divisor;
    int j=i;
    do{
        if(table[j]==nullptr||table[j]->first==theKey){
            return j;
        }
        j=(j+1)%divisor;
    }while(j!=i);
    return j;
}

template <class K,class E>
std::pair<const K,E>* HashTable<K,E>::find(const K&theKey)const{
    int b = sercher(theKey);
    if(table[b]==nullptr||table[b]->second==theKey){
        return nullptr;
    }
    return table[b];
    
}

template <class K,class E>
void HashTable<K,E>::insert(const std::pair<const K, E>&thePair){
    int b = sercher(thePair.first);
    if(table[b]==nullptr){
        table[b] = new std::pair<const K, E>(thePair.first, thePair.second);
        ++dSize;
    }
    else{
        if(table[b]->first==thePair.first){
            table[b]->second = thePair.second;
        }else{
            std::cerr<<"Error   inserting   ";
        }
    }
}

