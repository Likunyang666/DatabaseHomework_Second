#include "buffer/buffer_pool_manager.h"

namespace cmudb {

BufferPoolManager::BufferPoolManager(size_t pool_size,
                                                 DiskManager *disk_manager,
                                                 LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager),
      log_manager_(log_manager) {
      	
  pages_ = new Page[pool_size_];//Ϊ����ط���һ���������ڴ�ռ䡣
  page_table_ = new ExtendibleHash<page_id_t, Page *>(BUCKET_SIZE);
  replacer_ = new LRUReplacer<Page *>;
  free_list_ = new std::list<Page *>;


  for (size_t j = 0; j < pool_size_; ++j) {
    free_list_->push_back(&pages_[j]);//���ÿ��ҳ�涼��free_list�� 
  }
}


BufferPoolManager::~BufferPoolManager() {
  delete[] pages_;
  delete page_table_;
  delete replacer_;
  delete free_list_;
}

Page *BufferPoolManager::FetchPage(page_id_t page_id) {//����һ����������page_id�����ݵ�ҳ����󼴴ӻ�����л�ȡ����ҳ�� 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  if (page_table_->Find(page_id,target)) { //����ڲ�ҳ���Բ鿴�Ƿ��Ѿ�����һ����ӳ�䵽page_id��ҳ�档
    target->pin_count_++;//ҳ���ھͽ���̶� 
    replacer_->Erase(target);
    return target;//������ھͷ������ҳ�� 
  }

  target = GetVictimPage();
  if (target == nullptr) return target;//������ҳ�涼���̶�ʱ�ͷ���һ����ָ�� 
 
 
  if (target->is_dirty_) {/*���ѡ����ҳ��Ϊ���Ҫʹ�ô��̹�������������д�����*/
    disk_manager_->WritePage(target->GetPageId(),target->data_);
  }
  page_table_->Remove(target->GetPageId());//��ԭ����ҳ�Ƴ����µ�ҳ���� 
  page_table_->Insert(page_id,target);
  disk_manager_->ReadPage(page_id,target->data_);/*Ȼ����ʹ�ô��̹������Ӵ����ж�ȡĿ������ҳ�棬���������ݸ��Ƶ���ҳ������С�*/
  target->pin_count_ = 1;
  target->is_dirty_ = false;
  target->page_id_= page_id;

  return target;
}


bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {//ȡ���̶�ҳ�� 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  page_table_->Find(page_id,target);
  if (target == nullptr) {
    return false;
  }
  target->is_dirty_ = is_dirty;//��������is_dirtyΪtrue��Ƹ�ҳ��Ϊ�� 
  if (target->GetPinCount() <= 0) {
    return false;
  }
  ;
  if (--target->pin_count_ == 0) {//������ż�����Ϊ��(��û�б����ʹ�)����ú�������ҳ�������ӵ�LRU�滻���������С�
    replacer_->Insert(target);
  }
  return true;
}


bool BufferPoolManager::FlushPage(page_id_t page_id) {//��ָ��ҳˢ�µ����� 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  page_table_->Find(page_id,target);//����ҳ���ҵ�ҳ�� 
  if (target == nullptr || target->page_id_ == INVALID_PAGE_ID) {//ҳ����û����Ը�����page_id����Ŀ������ҳ�����Ҳ�����ҳ�棩 ������false 
    return false;
  }
  if (target->is_dirty_) {
    disk_manager_->WritePage(page_id,tar->GetData());//��ָ��ҳ�������д������ 
    target->is_dirty_ = false;
  }

  return true;
}

bool BufferPoolManager::DeletePage(page_id_t page_id) {
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  page_table_->Find(page_id,target);
  if (target != nullptr) {
    if (target->GetPinCount() > 0) {//pincount����0��ʾ��Ҫ�����ʾͲ���ɾ�� 
      return false;
    }
	//����ҳ�����ҵ���ҳ��ʱ������ع�������Ҫ������ҳ��Ϣ��ҳ����ɾ�� 
    replacer_->Erase(target);
    page_table_->Remove(page_id);
    target->is_dirty_= false; //ɾ����Ҫ����ҳ������ 
    target->ResetMemory();//�ͷŸ�ҳ������ռ�õ��ڴ� 
    free_list_->push_back(target);//�����µ�ҳ����ӵ������б��� 
  }
  disk_manager_->DeallocatePage(page_id);//�Ӵ����ļ���ɾ��ҳ�������Ϣ 
  return true;
}


Page *BufferPoolManager::NewPage(page_id_t &page_id) {//�����µ�ҳ�� 
  lock_guard<mutex> lck(latch_);
  Page *target = nullptr;
  target = GetVictimPage();
  if (target == nullptr) return target;//��ҳ�涼�̶�ʱ����nullptr 

  page_id = disk_manager_->AllocatePage();

  if (target->is_dirty_) {
    disk_manager_->WritePage(target->GetPageId(),target->data_);
  }
  //�����ڴ沢����Ӧ����Ŀ��ӵ�ҳ���� 
  page_table_->Remove(target->GetPageId());
  page_table_->Insert(page_id,target);

  //������ҳ���Ԫ����
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
    free_list_->pop_front();//ʼ�����ȴӿ����б���ѡ���滻ҳ�� 
    assert(target->GetPageId() == INVALID_PAGE_ID);
  }
  assert(target->GetPinCount() == 0);
  return target;
}

} // namespace cmudb
