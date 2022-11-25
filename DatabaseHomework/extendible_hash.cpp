#include <list>
#include "hash/extendible_hash.h"
#include "page/page.h"
using namespace std;

namespace cmudb {

template <typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash(size_t size) :  globaldepth(0),bucketSize(size),bucketNum(1) {
  buckets.push_back(make_shared<Bucket>(0));//��Ͱĩβ��Ӳ�������Ӧ�ڴ棬ÿ��Ͱ�Ĵ�С�̶� 
}
template<typename K, typename V>
ExtendibleHash<K, V>::ExtendibleHash() {
  ExtendibleHash(64);
}

template <typename K, typename V>
size_t ExtendibleHash<K, V>::HashKey(const K &key) const{//���ڸ�����k����Ͱ��ƫ���� 
  return hash<K>{}(key);
}

template <typename K, typename V>
int ExtendibleHash<K, V>::get_gdp() const{//���ص�ǰ��ϣ���ȫ�����
  lock_guard<mutex> lock(latch);//��ΪglobaldepthΪ������Դ����Ӧ�ö�����м��� 
  return globaldepth;
}

template <typename K, typename V>
int ExtendibleHash<K, V>::GetLocalDepth(int bucket_id) const {//����Ͱ�ĵ�ǰ�ֲ����
  if (buckets[bucket_id]) {
    lock_guard<mutex> lck(buckets[bucket_id]->latch);
    if (buckets[bucket_id]->kmap.size() == 0) return -1;
    return buckets[bucket_id]->localDepth;
  }
  return -1;
}

template <typename K, typename V>
int ExtendibleHash<K, V>::GetNumBuckets() const{//�����ڹ�ϣ���з����Ͱ������
  lock_guard<mutex> lock(latch);
  return bucketNum;
}

template <typename K, typename V>
bool ExtendibleHash<K, V>::Find(const K &key, V &value) {//�жϸü��Ƿ�����ڹ�ϣ����

  int Sequence_number = getIdx(key);
  lock_guard<mutex> lck(buckets[Sequence_number]->latch);
  if (buckets[Sequence_number]->kmap.find(key) != buckets[Sequence_number]->kmap.end()) {
    value = buckets[Sequence_number]->kmap[key];//���ü�����ʱvalue��ָ���Ӧֵ��ָ�� 
    return true;//�����ڷ���true 
  }
  return false;
}

template <typename K, typename V>
int ExtendibleHash<K, V>::getIdx(const K &key) const{
  lock_guard<mutex> lck(latch);
  return HashKey(key) & ((1 << globaldepth) - 1);//��1��value����õ�λ�� 
}

template <typename K, typename V>
bool ExtendibleHash<K, V>::Remove(const K &key) {//ɾ��Ԫ�� 
  int Sequence_number = getIdx(key);//���ҵ���ɾ��Ԫ�� 
  lock_guard<mutex> lck(buckets[Sequence_number]->latch);
  shared_ptr<Bucket> curent = buckets[Sequence_number];
  if (curent->kmap.find(key) == curent->kmap.end()) {//���û�и�Ԫ�ؾͷ���false 
    return false;
  }
  curent->kmap.erase(key);//��kmap��ɾ����ֵkeyָ���Ԫ�� 
  return true;
}

template <typename K, typename V>
void ExtendibleHash<K, V>::Insert(const K &key, const V &value) {
  int Sequence_number = getIdx(key);
  shared_ptr<Bucket> curent = buckets[Sequence_number];//�������ʱ����Ҫ���ҵ���ӵ�λ�� 
  while (true) {
    lock_guard<mutex> lck(cur->latch);
    if (curent->kmap.find(key) != curent->kmap.end() || curent->kmap.size() < bucketSize) {
      curent->kmap[key] = value;//��Ͱδ��ʱ��ֱ����� 
      break;
    }
    int cld = (1 << (curent->localDepth));//��ǰ�ֲ���ȵĶ����Ʊ�ʾ��ʽ 
    //����Ҫ��ӽ���Ͱ�Ѿ����˵�ʱ�򣬾���Ҫ��Ͱ����split����Ͱ�ľֲ�����Լ���ϣ���ȫ����ȶ�Ҫ���� 
	curent->localDepth++;
    {
      lock_guard<mutex> lck2(latch);
      if(curent->localDepth > globaldepth) {
        size_t length = buckets.size();//Ϊ�˸���ȫ����� 
        for (size_t j = 0; j < length; j++) {
          buckets.push_back(buckets[j]);
        }
        globaldepth++;//ȫ��������� 

      }
      bucketNum++;
      auto newBuc = make_shared<Bucket>(curent->localDepth);

      typename map<K, V>::iterator it;//��������˳������ 
      for (it = curent->kmap.begin(); it != curent->kmap.end(); ) {//�����µ�Ͱ����Ҫ����ԭ���Ĺ�ϣ�� 
        if (HashKey(it->first) & cld) {
          newBuc->kmap[it->first] = it->second;
          it = curent->kmap.erase(it);
        } else it++;//ָ������Ԫ�� 
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
