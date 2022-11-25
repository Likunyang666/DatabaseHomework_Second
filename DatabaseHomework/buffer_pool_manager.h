#pragma once 
#include <list>
#include <mutex>
#include "buffer/lru_replacer.h"
#include "disk/disk_manager.h"
#include "hash/extendible_hash.h"
#include "logging/log_manager.h"
#include "page/page.h"

namespace cmudb {
class BufferPoolManager {
public:
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager,
                          LogManager *log_manager = nullptr);

  ~BufferPoolManager();

  Page *FetchPage(page_id_t page_id);

  bool UnpinPage(page_id_t page_id, bool is_dirty);

  bool FlushPage(page_id_t page_id);

  Page *NewPage(page_id_t &page_id);

  bool DeletePage(page_id_t page_id);

private:
  size_t pool_size_; // 缓冲池中的页数
  Page *pages_;      // 页面组数 
  DiskManager *disk_manager_;
  LogManager *log_manager_;
  HashTable<page_id_t, Page *> *page_table_; // 用来跟踪页面 
  Replacer<Page *> *replacer_;   // 查找要替换的页面 
  std::list<Page *> *free_list_;
  std::mutex latch_;             
  Page *GetVictimPage();
};
} 
