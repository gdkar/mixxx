#ifndef UTIL_SPSC_QUEUE
#define UTIL_SPSC_QUEUE

#include <qmath.h>
#include <qatomic.h>
#include <qsharedpointer.h>

template<typename T>
class SPSCQueue{
  static size_t const cache_line_size = 64;
  struct node{
    QAtomicPointer<node>  m_next;
    T                     m_data;
  }; 
  public:
    SPSCQueue(){
      node *n = new node;
      m_tail.store(n);
      m_tail_copy.store(n);
      m_head store(n);
      m_first.store(n);
    }  
   ~SPSCQueue(){
    node *n = m_first.load();
    do{
      node *next = n->m_next.load();
      delete n;
      n = next;
    }while(n);
   }
   void enqueue(const T&v){
    node *n = alloc_node();
    n->m_next  = 0;
    n->m_value = v;
    m_head.m_next.store(n);
    m_head.store(n);
   }
   bool dequeue(T& v){
    node *n,*next;
    n=m_tail->load();
    next = n->m_next.load();
    if(next){
      v=next->value;
      m_tail.store(next);
      return true;
    }else{
      return false;
    }
   }
  private:
   QAtomicPointer<node> m_tail;
   char                 m_pad_sep[cache_line_size-sizeof(QAtomicPointer<node>)];
   QAtomicPointer<node> m_head;
   QAtomicPointer<node> m_first;
   QAtomicPointer<node> m_tail_copy;
   node *alloc_node(){
    node *first     = m_first.load();
    node *tail_copy = m_tail_copy.load();
    if(first!=tail_copy){
      node *n=first;
      m_first.store(first->next.load());
      return n;
    }else{
      tail_copy = m_tail.load();
      m_tail_copy.store(tail_copy);
      if(first != tail_copy){
        node *n = first;
        m_first.store( first->next.load());
        return n;
      }
    }
    node *n = new node;
    return n;
  }
  SPSCQueue(SPSCQueue const&other);
  SPSCQueue& operator = (SPSCQueue const &other);
};

#endif
