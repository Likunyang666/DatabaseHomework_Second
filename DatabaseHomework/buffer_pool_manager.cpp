#include "buffer/buffer_pool_manager.h"

namespace cmudb {

BufferPoolManager::BufferPoolManager(size_t pool_size,
                                                 DiskManager *disk_manager,
                                                 LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager),
      log_manager_(log_manager) {
      	
  pages_ = new Page[pool_size_];//为缓冲池分配一个连续的内存空间。
  page_table_ = new ExtendibleHash<page_id_t, Page *>(BUCKET_SIZE);
  replacer_ = new LRUReplacer<Page *>;
  free_list_ = new std::list<Page *>;


  for (size_t j = 0; j < pool_size_; ++j) {
    free_list_->push_back(&pages_[j]);//最初每个页面都在free_list中 
  }
}


BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete page_table_;
  delete replacer_;
  delete free_list_;
}

Page *BufferPoolManager::FetchPage(page_id_t page_id) {//返回一个包含给定page_id的内容的页面对象即从缓冲池中获取请求页面 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  if (page_table_->Find(page_id,target)) { //检查内部页表，以查看是否已经存在一个被映射到page_id的页面。
    target->pin_count_++;//页存在就将其固定 
    replacer_->Erase(target);
    return target;//如果存在就返回这个页面 
  }

  target = GetVictimPage();
  if (target == nullptr) return target;//当所有页面都被固定时就返回一个空指针 
 
 
  if (target->is_dirty_) {/*如果选定的页面为脏就要使用磁盘管理器将其内容写入磁盘*/
    disk_manager_->WritePage(target->GetPageId(),target->data_);
  }
  page_table_->Remove(target->GetPageId());//将原来的页移除将新的页加入 
  page_table_->Insert(page_id,target);
  disk_manager_->ReadPage(page_id,target->data_);/*然后，再使用磁盘管理器从磁盘中读取目标物理页面，并将其内容复制到该页面对象中。*/
  target->pin_count_ = 1;
  target->is_dirty_ = false;
  target->page_id_= page_id;

  return target;
}


bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {//取消固定页面 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  page_table_->Find(page_id,target);
  if (target == nullptr) {
    return false;
  }
  target->is_dirty_ = is_dirty;//如果输入的is_dirty为true则计该页面为脏 
  if (target->GetPinCount() <= 0) {
    return false;
  }
  ;
  if (--target->pin_count_ == 0) {//如果引脚计数器为零(即没有被访问过)，则该函数将该页面对象添加到LRU替换器跟踪器中。
    replacer_->Insert(target);
  }
  return true;
}


bool BufferPoolManager::FlushPage(page_id_t page_id) {//将指定页刷新到磁盘 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  page_table_->Find(page_id,target);//根据页号找到页表 
  if (target == nullptr || target->page_id_ == INVALID_PAGE_ID) {//页表中没有针对给定的page_id的条目（即在页表中找不到该页面） ，返回false 
    return false;
  }
  if (target->is_dirty_) {
    disk_manager_->WritePage(page_id,tar->GetData());//将指定页面的内容写进磁盘 
    target->is_dirty_ = false;
  }

  return true;
}

bool BufferPoolManager::DeletePage(page_id_t page_id) {
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  page_table_->Find(page_id,target);
  if (target != nullptr) {
    if (target->GetPinCount() > 0) {//pincount大于0表示还要被访问就不能删除 
      return false;
    }
	//当在页表中找到该页面时，缓冲池管理器就要将该条页信息从页表中删除 
    replacer_->Erase(target);
    page_table_->Remove(page_id);
    target->is_dirty_= false; //删除后要更新页面数据 
    target->ResetMemory();//释放该页面内容占用的内存 
    free_list_->push_back(target);//将更新的页面添加到空闲列表中 
  }
  disk_manager_->DeallocatePage(page_id);//从磁盘文件中删除页面相关信息 
  return true;
}


Page *BufferPoolManager::NewPage(page_id_t &page_id) {//创建新的页面 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  target = GetVictimPage();
  if (target == nullptr) return target;//当页面都固定时返回nullptr 

  page_id = disk_manager_->AllocatePage();

  if (target->is_dirty_) {
    disk_manager_->WritePage(target->GetPageId(),target->data_);
  }
  //清零内存并将相应的条目添加到页表中 
  page_table_->Remove(target->GetPageId());
  page_table_->Insert(page_id,target);

  //更新新页面的元数据
  target->page_id_ = page_id;
  target->ResetMemory();
  target->is_dirty_ = false;
  target->pin_count_ = 1;

  return target;
}

Page *BufferPoolManager::GetVictimPage() {
  Page *target = nullptr;
  if (free_list_->empty()) {
    if (replacer_->Size() == 0) {
      return nullptr;
    }
    replacer_->Victim(target);
  } else {
    target = free_list_->front();
    free_list_->pop_front();//始终优先从空闲列表中选择替换页面 
    assert(target->GetPageId() == INVALID_PAGE_ID);
  }
  assert(target->GetPinCount() == 0);
  return target;
}

} // namespace cmudb
