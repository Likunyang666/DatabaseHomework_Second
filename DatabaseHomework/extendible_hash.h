#pragma once
#include <cstdlib>
#include <vector>
#include <string>
#include <map>
#include <memory>//ʹ������ָ����Ҫ 
#include <mutex>
#include "hash/hash_table.h"
using namespace std;

namespace cmudb {

template <typename K, typename V>
class ExtendibleHash : public HashTable<K, V> {
  struct Bucket {
    Bucket(int depth) : localDepth(depth) {};//�ֲ���Ⱥ�Ͱ���Ӧ 
    int localDepth;
    map<K, V> kmap;
    mutex latch;
  };
  
public:
  ExtendibleHash(size_t size);
  ExtendibleHash();
  size_t HashKey(const K &key) const;//ͨ��K�õ�Ͱ��ƫ����
  int get_gdp() const;//��ȫ����� 
  int GetLocalDepth(int bucket_id) const;//��ֲ���� 
  int GetNumBuckets() const;//����Ͱ������ 
  bool Find(const K &key, V &value) override;
  bool Remove(const K &key) override;//ɾ����ֵ�� 
  void Insert(const K &key,const V &value) override;//�����ֵ�� 
  int getIdx(const K &key) const;

private:
  int globaldepth;
  size_t bucketSize;
  int bucketNum;
  vector<shared_ptr<Bucket>> buckets;
  mutable mutex latch;
};
} 
