
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
  
/*��Ԫ��T���Ϊ�������ݿ��б����ʡ�����ζ�Ÿ�Ԫ����������Ƶ�����ʵģ���Ӧ�ñ�ѡ��Ϊ�ӻ������ɾ�����ܺ��ߣ�������ڶ��Ԫ�أ���*/
  void Insert(const T &value);

/*ɾ�����滻�����ٵ�����Ԫ����ȣ�������ʴ������ٵĶ��󣬽������ݴ洢��ֵT�У�Ȼ�󷵻�true������滻����ֻ��һ��Ԫ�أ�
��ô�����Ǳ���Ϊ�����ʹ�õ����ٵ�Ԫ�ء�����滻���д�����Ԫ�أ���˺���Ӧ�÷���false��*/
  bool Victim(T &value);

/*���滻�����ڲ��������ݽṹ����ȫɾ��Ԫ��T��������������LRU�滻���е��ĸ�λ�á���Ӧ�û��Ԫ����ɾ�����еĸ������ݡ�
���Ԫ��T���ڲ�������ɾ������ô�ú���Ӧ�÷���true�����򣬷���false��*/
  bool Erase(const T &value);

  size_t Size();//���ش��滻�����ڸ��ٵ�Ԫ������

private:
  shared_ptr<Node> head;
  shared_ptr<Node> tail;
  unordered_map<T,shared_ptr<Node>> map;
  mutable mutex latch;
  // add your member variables here
};

} // namespace cmudb
