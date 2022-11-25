
#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include "buffer/replacer.h"

using namespace std;
namespace cmudb {

template <typename T> class LRUReplacer : public Replacer<T> {
  struct Node {
    Node() {};
    Node(T val) : val(val) {};
    T val;
    shared_ptr<Node> prev;
    shared_ptr<Node> next;
  };
public:
  // do not change public interface
  LRUReplacer();

  ~LRUReplacer();
  
/*将元素T标记为已在数据库中被访问。这意味着该元素现在是最频繁访问的，不应该被选择为从缓冲池中删除的受害者（假设存在多个元素）。*/
  void Insert(const T &value);

/*删除与替换器跟踪的所有元素相比，最近访问次数最少的对象，将其内容存储在值T中，然后返回true。如果替换器中只有一个元素，
那么它总是被认为是最近使用得最少的元素。如果替换器中存在零元素，则此函数应该返回false。*/
  bool Victim(T &value);

/*从替换器的内部跟踪数据结构中完全删除元素T，无论它出现在LRU替换器中的哪个位置。这应该会从元素中删除所有的跟踪数据。
如果元素T存在并且它被删除，那么该函数应该返回true。否则，返回false。*/
  bool Erase(const T &value);

  size_t Size();//返回此替换器正在跟踪的元素数。

private:
  shared_ptr<Node> head;
  shared_ptr<Node> tail;
  unordered_map<T,shared_ptr<Node>> map;
  mutable mutex latch;
  // add your member variables here
};

} // namespace cmudb
