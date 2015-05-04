#ifndef SINGLETON_H
#define SINGLETON_H

#include <QtDebug>
#include <QThread>
#include <qsharedpointer.h>
#include <qthreadstorage.h>
#include <qatomic.h>

template<class S>
class Singleton{
public:
    inline static S* create() {
        S* current = s_instance.load();
        if(Q_UNLIKELY(!current)){
          current = new S();
          if(Q_UNLIKELY(!s_instance.testAndSetRelaxed(0,current))){delete current;current = s_instance.load();}
        }
        return current;
    }
    inline static S* instance() {return create();}
    static void destroy() {S* current = s_instance.load();
      if(Q_LIKELY(current)){if(Q_LIKELY(s_instance.testAndSetRelaxed(current,0)))delete current;}}
protected:
    Singleton() {}
    virtual ~Singleton() {}
private:
    //hide copy constructor and assign operator
    Singleton(const Singleton&) {}
    const Singleton& operator= (const Singleton&) {}
    static QAtomicPointer<S> s_instance;
};
template<class S> QAtomicPointer<S>  Singleton<S>::s_instance = 0;

template<class  T>
class Threadington{
      T                                      item;
      static QAtomicPointer<Threadington<T> >  fl_head;
      static QAtomicInteger<qint64>            fl_size;

      QAtomicPointer<QThread>                thrd;
      QAtomicPointer<Threadington<T> >      next;
    struct TLSRef {
      Threadington<T>        *item;
      TLSRef(){
        QThread         *thrd = QThread::currentThread();
        Threadington<T> *it = fl_head.load();
        while(it!=0){
          if(it->thrd.load()==0 && it->thrd.testAndSetRelaxed(0,thrd)){
            item = it;
            return;
          }
        }
        item = new Threadington<T>();
        item->thrd = thrd;
        do{
          it = fl_head.load();
          item->next.store(it);
        }while(!fl_head.testAndSetRelaxed(it,item)); 
        fl_size++;
      }
     ~TLSRef(){item->thrd.store(0);}
    };
    static QThreadStorage<TLSRef>   t_instance;
  public:
    inline static T *instance(){return t_instance.localData().item;}
};
#endif // SINGLETON_H
