#ifndef REFERENCE_H
#define REFERENCE_H
#include <qatomic.h>
#include <qthread.h>
// General tool for removing concrete dependencies while still incrementing a
// reference count.
class BaseReferenceHolder {
  public:
    BaseReferenceHolder() { }
    virtual ~BaseReferenceHolder() { }
};
template <class T>
class ReferenceHolder : public BaseReferenceHolder {
  public:
    ReferenceHolder(QSharedPointer<T>& reference)
            : m_reference(reference) {}
    virtual ~ReferenceHolder() {}
  private:
    QSharedPointer<T> m_reference;
};

template<typename T>
class FreeList{
  public:
  struct Link{
    FreeList<T>      * const list;
    T                        item;
    Link             *next;
    QAtomicPointer<QThread> owner;
    Link(FreeList<T> *_list):
      list(_list),
      owner(QThread::currentThread()){
      do{
          next = list->m_head.loadAcquire();
      }while(!list->m_head.testAndSetRelaxed(next,this));
    }
  };
  QAtomicPointer<Link>    m_head;
  explicit FreeList()
    :m_head(0){
  }
  virtual ~FreeList(){
    Link *link = m_head.loadAcquire();
    while(link){
      Link *next = link->next;
      delete link;
      link = next;
    }
  }
  virtual Link *acquire(){
    QThread *thrd = QThread::currentThread();
    Link *link = head();
      while(link){
        if(!link->owner.loadAcquire()){
          if(link->owner.testAndSetRelaxed(0,thrd))
            return link;
        }
        link = link->next;
      }
    return new Link(this);
  }
  Link *head() const{return m_head.loadAcquire();}
};
template<typename T>
class FreeListIterator {
  typedef FreeList<T>   list_type;
  typedef typename FreeList<T>::Link link_type;
  list_type     &      m_list;
  link_type     *      m_link;
  public:
    explicit FreeListIterator(FreeList<T> &_list):m_list(_list),m_link(m_list.head()){}
    virtual ~FreeListIterator(){}
    virtual bool hasNext() const{return !!m_link;} 
    virtual T &  next(){ T &item = m_link->item; m_link=m_link->next; return item;}
    virtual T &  peekNext(){T &item = m_link->item; return item;}
    virtual void toFront(){m_link = m_list.head();}
    virtual FreeListIterator & operator = (FreeList<T> & _list){m_list = _list;toFront(); return *this;}
};
template<typename T>
class FreeListHolder {
  typedef FreeList<T>               list_type;
  typedef typename FreeList<T>::Link link_type;
  QThread                              *const m_thread;
  list_type                            *const m_list;
  link_type                            *const m_link;
public:
    FreeListHolder(FreeList<T> *_list):
      m_thread(QThread::currentThread()),
      m_list(_list),
      m_link(m_list->acquire()){
    }
    ~FreeListHolder(){ m_link->owner.storeRelease(0);}
    T  *item()const{return &m_link->item;}
};

#endif /* REFERENCE_H */
