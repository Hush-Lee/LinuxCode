#include <cmath>
#include <iostream>
#include <sstream>



template <class K, class E>
struct skipNode {
  typedef std::pair<const K, E> pairType;

  pairType element;

  skipNode<K, E> **next;

  skipNode(const pairType thePair, int size) : element(thePair) {
    next = new skipNode<K, E> *[size];
  }
};

template <class K, class E>
class skipList {
  float cutOff;
  int levels;
  int dSize;
  int maxLevel;
  K tailKey;
  skipNode<K, E> *headerNode;
  skipNode<K, E> *tailNode;
  skipNode<K, E> **last;

  public:
  skipList(K largeKey, int maxPairs, float prob);
  std::pair<const K, E> find(const K &theKey) const;
  int level() const;
  skipNode<K, E> *search(const K &theKey) const;
  void insert(const std::pair<const K, E> &thePair);
  void erase(const K &theKey);
};

template<class K,class E> 
skipList<K,E>::skipList(K largeKey,int maxPairs,float prob){
  cutOff = prob * RAND_MAX;//prob between 0 and 1,
  maxLevel = (int)std::ceil(logf((float)maxPairs)/logf(1/prob))-1;
  levels = 0 ;
  dSize = 0 ;
  tailKey = largeKey ;
  std::pair<K,E> tailPair;
  tailPair.first = tailKey ;
  headerNode = new skipNode<K,E>(tailPair,maxLevel + 1);
  tailNode=new skipNode<K,E>(tailPair,0);
  last = new skipNode<K,E> *[maxLevel + 1];
  for(int i=0;i<maxLevel;++i){//make headerNode point to tailNode
    headerNode->next[i] = tailNode;
  }
}

template<class K,class E> 
std::pair<const K,E> skipList<K,E>::find(const K &theKey) const{
  if(theKey >= tailKey){
    return nullptr;
  }
  skipNode<K,E> *beforeNode=headerNode;
  for(int i = levels;i>=0;--i){
    while (beforeNode->next[i]->element.first < theKey)
    {
      beforeNode = beforeNode->next[i];//when the first key less than the target,the pointer to the next element of this level,
    }
    if(beforeNode->next[0]->element.first==theKey){//compare the current key with the first key of the next level;
      return &beforeNode->next[0]->element;
    }
    
  }
  return nullptr;
}

template<class K,class E> 
int skipList<K,E>::level() const{
  int lev=0;
  while(rand()<=cutOff){
    lev++;
  }
  return (lev <= maxLevel)? lev : maxLevel;
}

template<class K,class E> 
skipNode<K,E> *skipList<K,E>::search(const K &theKey) const{
  skipNode<K,E> *beforeNode=headerNode;
  for(int i = levels;i>=0;--i){
    while(beforeNode->next[i]->element.first < theKey){
      beforeNode = beforeNode->next[i];
    }
    last[i] = beforeNode;
  }
  return beforeNode->next[0];
}

template<class K,class E> 
void skipList<K,E>::insert(const std::pair<const K, E> &thePair){
  if(thePair.first >= tailKey){//out of the range
    std::ostringstream s;
    s << "the key " << thePair.first << " is larger than the tail key " << tailKey;
    throw std::runtime_error(s.str());
 }
 skipNode<K,E> theNode=search(thePair.first);//search the key in the list
 if(theNode->element.first==thePair.first){//if the key already exists ,change the value
  theNode->element.second=thePair.second;
  return;
 }
  int theLevel = level();//generate the level
  if(theLevel > maxLevel){
    theLevel = ++levels;
    last[theLevel] = headerNode;
  }//out of the max level,add new level,the first node of the new level is the headerNode
  skipNode<K,E> *newNode = new skipNode<K,E>(thePair,theLevel+1);
  for(int i=0;i<=theLevel;++i){
    newNode->next[i] = last[i]->next[i];
    last[i]->next[i] = newNode;
  }
  ++dSize;
  return;
}
template <class K,class E>
void skipList<K,E>::erase(const K &theKey){
  if(theKey>=tailKey){//out of the range
    return;
  }
  skipNode<K,E> * theNode = search(theKey);//search the key in the list
  if(theNode->element.first!=theKey){//if the key does not exist,return
    return;
  }
  for(int i=0;i<=levels&&last[i]->next[i]==theNode;++i){
    last[i]->next[i] = theNode->next[i];
  }
  while(levels>0&&last[levels]->next[levels]==tailNode){
    --levels;
    delete theNode;
    --dSize;
  }
 
}