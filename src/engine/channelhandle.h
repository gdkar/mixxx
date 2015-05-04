#ifndef CHANNELHANDLE_H
#define CHANNELHANDLE_H
// ChannelHandle defines a unique identifier for channels of audio in the engine
// (e.g. headphone output, master output, deck 1, microphone 3). Previously we
// used the group string of the channel in the engine to uniquely identify it
// and key associative containers (e.g. QMap, QHash) but the downside to this is
// that we waste a lot of callback time hashing and re-hashing the strings.
//
// To solve this problem we introduce ChannelHandle, a thin wrapper around an
// integer. As engine channels are registered they are assigned a ChannelHandle
// starting at 0 and incrementing. The benefit to this scheme is that the hash
// and equality of ChannelHandles are simple to calculate and a QVarLengthArray
// can be used to create a fast associative container backed by a simple array
// (since the keys are numbered [0, num_channels]).
//
// A helper class, ChannelHandleFactory, keeps a running count of handles that
// have been assigned.

#include <QtDebug>
#include <QHash>
#include <QString>
#include <QVarLengthArray>
#include <qsharedpointer.h>
#include <qatomic.h>
#include <qmath.h>
#include "util/assert.h"
#include "util/singleton.h"

// A wrapper around an integer handle. Used to uniquely identify and refer to
// channels (headphone output, master output, deck 1, microphone 4, etc.) of
// audio in the engine.
class ChannelHandle {
    class ChannelCache : public QObject, public Singleton<ChannelCache>{
      QAtomicInteger<int>    m_serial;
      QVector<QString>       m_intern;
      const QString          null_str;
      QHash<QString,int>     m_lookup;
      public:
          ChannelCache():m_serial(0){}
          int query(const QString &s){
          if(m_lookup.contains(s)){
            return m_lookup.value(s);
          }else{
            int i=0, next = m_serial.fetchAndAddRelaxed(1);
            if(next >= m_intern.size()) m_intern.resize((next+1)*4);
            m_intern[next] = s;
            for(;i<next && m_intern[i]!=s;i++){}
            if(next==i){m_lookup.insert(s,next);}
            else{next = i;}
            return next;
          }
        }
        int alloc(){return m_serial.fetchAndAddRelaxed(1);}
        void rename(const int i, const QString &s){
          QString prev = s;
          m_intern[i].swap(prev);
          m_lookup.insert(s,i);
        }
        const QString &name(const int i){
          if(0<=i&&i < m_intern.size())
            return m_intern.at(i);
          return null_str;
          }
    };
  public:
    ChannelHandle() : m_iHandle(ChannelCache::instance()->alloc()) {}
    ChannelHandle(const QString & _name):m_iHandle(ChannelCache::instance()->query(_name)){}
    ChannelHandle(const ChannelHandle &other):m_iHandle(other.m_iHandle){}
    const QString &name() const{return ChannelCache::instance()->name(m_iHandle);}
    void setName(const QString &_name){ChannelCache::instance()->rename(m_iHandle,_name);}
    inline bool  valid() const {return m_iHandle >= 0;}
    inline int   handle() const {return m_iHandle;}
  private:
    ChannelHandle(int iHandle): m_iHandle(iHandle) {}
    void setHandle(int iHandle) {m_iHandle = iHandle;}
    int m_iHandle;
    friend class ChannelHandleFactory;
};

inline bool operator==(const ChannelHandle& h1, const ChannelHandle& h2) {
    return h1.handle() == h2.handle();
}

inline bool operator!=(const ChannelHandle& h1, const ChannelHandle& h2) {
    return h1.handle() != h2.handle();
}

inline QDebug operator<<(QDebug stream, const ChannelHandle& h) {
    stream << "ChannelHandle(" << h.handle() << ")";
    return stream;
}

inline uint qHash(const ChannelHandle& handle) {
    return qHash(handle.handle());
}

// Convenience class that mimics QPair<ChannelHandle, QString> except with
// custom equality and hash methods that save the cost of touching the QString.
class ChannelHandleAndGroup {
  public:
    ChannelHandleAndGroup(const ChannelHandle& handle, const QString& name)
            : m_handle(handle),
              m_name(name) {
    }

    inline const QString& name() const {
        return m_name;
    }

    inline const ChannelHandle& handle() const {
        return m_handle;
    }

    const ChannelHandle m_handle;
    const QString m_name;
};

inline bool operator==(const ChannelHandleAndGroup& g1, const ChannelHandleAndGroup& g2) {
    return g1.handle() == g2.handle();
}

inline bool operator!=(const ChannelHandleAndGroup& g1, const ChannelHandleAndGroup& g2) {
    return g1.handle() != g2.handle();
}

inline QDebug operator<<(QDebug stream, const ChannelHandleAndGroup& g) {
    stream << "ChannelHandleAndGroup(" << g.name() << "," << g.handle() << ")";
    return stream;
}

inline uint qHash(const ChannelHandleAndGroup& handle_group) {
    return qHash(handle_group.handle());
}

// A helper class used by EngineMaster to assign ChannelHandles to channel group
// strings. Warning: ChannelHandles produced by different ChannelHandleFactory
// objects are not compatible and will produce incorrect results when compared,
// stored in the same container, etc. In practice we only use one instance in
// EngineMaster.
class ChannelHandleFactory {
  public:
    ChannelHandleFactory() : m_iNextHandle(0) {
    }

    ChannelHandle getOrCreateHandle(const QString& group) {
        ChannelHandle& handle = m_groupToHandle[group];
        if (!handle.valid()) {
            handle.setHandle(m_iNextHandle++);
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(!m_handleToGroup.contains(handle));
            m_handleToGroup.insert(handle, group);
        }
        return handle;
    }

    ChannelHandle handleForGroup(const QString& group) const {
        return m_groupToHandle.value(group, ChannelHandle());
    }

    QString groupForHandle(const ChannelHandle& handle) const {
        return m_handleToGroup.value(handle, QString());
    }

  private:
    int m_iNextHandle;
    QHash<QString, ChannelHandle> m_groupToHandle;
    QHash<ChannelHandle, QString> m_handleToGroup;
};

// An associative container mapping ChannelHandle to a template type T. Backed
// by a QVarLengthArray with ChannelHandleMap::kMaxExpectedGroups pre-allocated
// entries. Insertions are amortized O(1) time (if less than kMaxExpectedGroups
// exist then no allocation will occur -- insertion is a mere copy). Lookups are
// O(1) and quite fast -- a simple index into an array using the handle's
// integer value.
template <class T>
class ChannelHandleMap {
    static const int kMaxExpectedGroups = 256;
    typedef QVarLengthArray<T, kMaxExpectedGroups> container_type;
  public:
    typedef typename QVarLengthArray<T, kMaxExpectedGroups>::const_iterator const_iterator;
    typedef typename QVarLengthArray<T, kMaxExpectedGroups>::iterator iterator;

    const T& at(const ChannelHandle& handle) const {
        if (!handle.valid()) {
            return m_dummy;
        }
        return m_data.at(handle.handle());
    }

    void insert(const ChannelHandle& handle, const T& value) {
        if (!handle.valid()) {
            return;
        }

        int iHandle = handle.handle();
        maybeExpand(iHandle + 1);
        m_data[iHandle] = value;
    }

    T& operator[](const ChannelHandle& handle) {
        if (!handle.valid()) {
            return m_dummy;
        }
        int iHandle = handle.handle();
        maybeExpand(iHandle + 1);
        return m_data[iHandle];
    }

    void clear() {
        m_data.clear();
    }

    typename container_type::iterator begin() {
        return m_data.begin();
    }

    typename container_type::const_iterator begin() const {
        return m_data.begin();
    }

    typename container_type::iterator end() {
        return m_data.end();
    }

    typename container_type::const_iterator end() const {
        return m_data.end();
    }

  private:
    inline void maybeExpand(int iSize) {
        if (m_data.size() < iSize) {
            m_data.resize(iSize);
        }
    }
    container_type m_data;
    T m_dummy;
};

#endif /* CHANNELHANDLE,_H */
