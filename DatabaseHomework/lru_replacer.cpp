/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace cmudb {

template <typename T> LRUReplacer<T>::LRUReplacer() {
  head = make_shared<Node>();
  tail = make_shared<Node>();
  //��ʼʱ����ֻ��ͷ��β 
  head->next = tail;
  tail->prev = head;
}

template <typename T> LRUReplacer<T>::~LRUReplacer() {}


template <typename T> void LRUReplacer<T>::Insert(const T &value) {
  lock_guard<mutex> lck(latch);
  shared_ptr<Node> curent;
  if (map.find(value) != map.end()) {
    curent = map[value];//curentΪҪ����ĵ�ǰλ�õ�ָ�� 
    shared_ptr<Node> prev = curent->prev;//����ָ������λ��ǰ��ָ���ԭָ��Ҫ���б仯 
    shared_ptr<Node> succ = curent->next;
    prev->next = succ;
    succ->prev = prev;
  } else {
    curent = make_shared<Node>(value);
  }
  shared_ptr<Node> fir = head->next;
  curent->next = fir;
  fir->prev = curent;
  curent->prev = head;
  head->next = curent;
  map[value] = curent;
  return;
}

//ɾ�����滻������ٵ�����Ԫ���������������ٵĶ��󣬽������ݴ洢��ֵ T �У�Ȼ�󷵻� true������滻����ֻ��һ��
//Ԫ�أ���ʼ����Ϊ��Ԫ�����������ʹ�õ�Ԫ�ء�����滻���������Ԫ�أ���˺���Ӧ���� false��
template <typename T> bool LRUReplacer<T>::Victim(T &value) {
  lock_guard<mutex> lck(latch);
  if (map.empty()) {
    return false;
  }
  shared_ptr<Node> last = tail->prev;
  tail->prev = last->prev;
  last->prev->next = tail;
  value = last->val;
  map.erase(last->val);//ɾ�����һ��Ԫ�� 
  return true;
}

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
template <typename T> bool LRUReplacer<T>::Erase(const T &value) {
  lock_guard<mutex> lck(latch);
  if (map.find(value) != map.end()) {
    shared_ptr<Node> curent = map[value];//��ָ��ָ��Ҫɾ����λ�� 
    curent->prev->next = curent->next;//ɾ�����ø�ɾ��λ��ǰһ��λ��ָ������ָ��ָ���ɾ��λ����һ�� 
    curent->next->prev = curent->prev;//ɾ�����ø�ɾ��λ�ĺ�һ��λ��ָ��ǰ���ָ��ָ���ɾ��λ����һ�� 
  }
  return map.erase(value);
}

template <typename T> size_t LRUReplacer<T>::Size() {//���ش��滻�����ڸ��ٵ�Ԫ������
  lock_guard<mutex> lck(latch);
  return map.size();
}

template class LRUReplacer<Page *>;
// test only
template class LRUReplacer<int>;

} // namespace cmudb
