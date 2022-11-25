#pragma once
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <memory>//使用智能指针需要 
#include <mutex>
#include "hash/hash_table.h"
using namespace std;

namespace cmudb {

template <typename K, typename V>
class ExtendibleHash : public HashTable<K, V> {
  struct Bucket {
    Bucket(int depth) : localDepth(depth) {};//局部深度和桶相对应 
    int localDepth;
    map<K, V> kmap;
    mutex latch;
  };
  
public:
  ExtendibleHash(size_t size);
  ExtendibleHash();
  size_t HashKey(const K &key) const;//通过K得到桶的偏移量
  int get_gdp() const;//求全局深度 
  int GetLocalDepth(int bucket_id) const;//求局部深度 
  int GetNumBuckets() const;//返回桶的数量 
  bool Find(const K &key, V &value) override;
  bool Remove(const K &key) override;//删除键值对 
  void Insert(const K &key,const V &value) override;//插入键值对 
  int getIdx(const K &key) const;

private:
  int globaldepth;
  size_t bucketSize;
  int bucketNum;
  vector<shared_ptr<Bucket>> buckets;
  mutable mutex latch;
};
} 
