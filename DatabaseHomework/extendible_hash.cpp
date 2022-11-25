#include <list>
#include "hash/extendible_hash.h"
#include "page/page.h"
using namespace std;

namespace cmudb {

template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size) :  globaldepth(0),bucketSize(size),bucketNum(1) {
  buckets.push_back(make_shared<Bucket>(0));//在桶末尾添加并分配相应内存，每个桶的大小固定 
}
template<typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash() {
  ExtendibleHash(64);
}

template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) const{//对于给定的k返回桶的偏移量 
  return hash<K>{}(key);
}

template <typename K, typename V>
int ExtendibleHash<K, V>::get_gdp() const{//返回当前哈希表的全局深度
  lock_guard<mutex> lock(latch);//因为globaldepth为共享资源所以应该对其进行加锁 
  return globaldepth;
}

template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {//返回桶的当前局部深度
  if (buckets[bucket_id]) {
    lock_guard<mutex> lck(buckets[bucket_id]->latch);
    if (buckets[bucket_id]->kmap.size() == 0) return -1;
    return buckets[bucket_id]->localDepth;
  }
  return -1;
}

template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const{//返回在哈希表中分配的桶的总数
  lock_guard<mutex> lock(latch);
  return bucketNum;
}

template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {//判断该键是否存在于哈希表中

  int Sequence_number = getIdx(key);
  lock_guard<mutex> lck(buckets[Sequence_number]->latch);
  if (buckets[Sequence_number]->kmap.find(key) != buckets[Sequence_number]->kmap.end()) {
    value = buckets[Sequence_number]->kmap[key];//当该键存在时value就指向对应值的指针 
    return true;//键存在返回true 
  }
  return false;
}

template <typename K, typename V>
int ExtendibleHash<K, V>::getIdx(const K &key) const{
  lock_guard<mutex> lck(latch);
  return HashKey(key) & ((1 << globaldepth) - 1);//用1与value相与得到位置 
}

template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {//删除元素 
  int Sequence_number = getIdx(key);//先找到待删除元素 
  lock_guard<mutex> lck(buckets[Sequence_number]->latch);
  shared_ptr<Bucket> curent = buckets[Sequence_number];
  if (curent->kmap.find(key) == curent->kmap.end()) {//如果没有该元素就返回false 
    return false;
  }
  curent->kmap.erase(key);//从kmap中删除键值key指向的元素 
  return true;
}

template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
  int Sequence_number = getIdx(key);
  shared_ptr<Bucket> curent = buckets[Sequence_number];//进行添加时就需要先找到添加的位置 
  while (true) {
    lock_guard<mutex> lck(cur->latch);
    if (curent->kmap.find(key) != curent->kmap.end() || curent->kmap.size() < bucketSize) {
      curent->kmap[key] = value;//当桶未满时就直接添加 
      break;
    }
    int cld = (1 << (curent->localDepth));//当前局部深度的二进制表示形式 
    //当需要添加进的桶已经满了的时候，就需要对桶进行split，该桶的局部深度以及哈希表的全局深度都要增加 
	curent->localDepth++;
    {
      lock_guard<mutex> lck2(latch);
      if(curent->localDepth > globaldepth) {
        size_t length = buckets.size();//为了更新全局深度 
        for (size_t j = 0; j < length; j++) {
          buckets.push_back(buckets[j]);
        }
        globaldepth++;//全局深度增加 

      }
      bucketNum++;
      auto newBuc = make_shared<Bucket>(curent->localDepth);

      typename map<K, V>::iterator it;//迭代访问顺序容器 
      for (it = curent->kmap.begin(); it != curent->kmap.end(); ) {//增加新的桶后需要更新原来的哈希表 
        if (HashKey(it->first) & cld) {
          newBuc->kmap[it->first] = it->second;
          it = curent->kmap.erase(it);
        } else it++;//指向后面的元素 
      }
      for (size_t j = 0; j < buckets.size(); j++) {
        if (buckets[j] == curent && (j & cld))
          buckets[j] = newBuc;
      }
    }
    Sequence_number = getIdx(key);
    curent = buckets[Sequence_number];
  }
}



template class ExtendibleHash<page_id_t, Page *>;
template class ExtendibleHash<Page *, std::list<Page *>::iterator>;

template class ExtendibleHash<int, std::string>;
template class ExtendibleHash<int, std::list<int>::iterator>;
template class ExtendibleHash<int, int>;
} 
