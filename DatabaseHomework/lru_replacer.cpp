/**
 * LRU implementation
 */
#include "buffer/lru_replacer.h"
#include "page/page.h"

namespace cmudb {

template <typename T> LRUReplacer<T>::LRUReplacer() {
  head = make_shared<Node>();
  tail = make_shared<Node>();
  //初始时链表只有头和尾 
  head->next = tail;
  tail->prev = head;
}

template <typename T> LRUReplacer<T>::~LRUReplacer() {}


template <typename T> void LRUReplacer<T>::Insert(const T &value) {
  lock_guard<mutex> lck(latch);
  shared_ptr<Node> curent;
  if (map.find(value) != map.end()) {
    curent = map[value];//curent为要插入的当前位置的指针 
    shared_ptr<Node> prev = curent->prev;//插入指针后插入位置前后指针的原指向都要进行变化 
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

//删除与替换程序跟踪的所有元素相比最近访问最少的对象，将其内容存储在值 T 中，然后返回 true。如果替换器中只有一个
//元素，则始终认为该元素是最近最少使用的元素。如果替换器中有零个元素，则此函数应返回 false。
template <typename T> bool LRUReplacer<T>::Victim(T &value) {
  lock_guard<mutex> lck(latch);
  if (map.empty()) {
    return false;
  }
  shared_ptr<Node> last = tail->prev;
  tail->prev = last->prev;
  last->prev->next = tail;
  value = last->val;
  map.erase(last->val);//删除最后一个元素 
  return true;
}

/*
 * Remove value from LRU. If removal is successful, return true, otherwise
 * return false
 */
template <typename T> bool LRUReplacer<T>::Erase(const T &value) {
  lock_guard<mutex> lck(latch);
  if (map.find(value) != map.end()) {
    shared_ptr<Node> curent = map[value];//将指针指向要删除的位置 
    curent->prev->next = curent->next;//删除后让该删除位的前一个位置指向后面的指针指向该删除位的下一个 
    curent->next->prev = curent->prev;//删除后让该删除位的后一个位置指向前面的指针指向该删除位的上一个 
  }
  return map.erase(value);
}

template <typename T> size_t LRUReplacer<T>::Size() {//返回此替换器正在跟踪的元素数。
  lock_guard<mutex> lck(latch);
  return map.size();
}

template class LRUReplacer<Page *>;
// test only
template class LRUReplacer<int>;

} // namespace cmudb
