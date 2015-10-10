_Pragma("once")
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
#include <atomic>
#include <QHash>
#include <QString>
#include <QVarLengthArray>

#include "util/assert.h"

// A wrapper around an integer handle. Used to uniquely identify and refer to
// channels (headphone output, master output, deck 1, microphone 4, etc.) of
// audio in the engine.
class ChannelHandle {
  public:
    ChannelHandle() = default;
    bool valid() const {
        return m_iHandle >= 0;
    }
    int handle() const {
        return m_iHandle;
    }
  private:
    ChannelHandle(int iHandle)
            : m_iHandle(iHandle) {
    }
    void setHandle(int iHandle) {
        m_iHandle = iHandle;
    }
    int m_iHandle = -1;
    friend class ChannelHandleFactory;
};
bool operator==(const ChannelHandle& h1, const ChannelHandle& h2)
{
    return h1.handle() == h2.handle();
}

bool operator!=(const ChannelHandle& h1, const ChannelHandle& h2)
{
    return h1.handle() != h2.handle();
}

QDebug operator<<(QDebug stream, const ChannelHandle& h)
{
    stream << "ChannelHandle(" << h.handle() << ")";
    return stream;
}
uint qHash(const ChannelHandle& handle)
{
    return qHash(handle.handle());
}
// Convenience class that mimics QPair<ChannelHandle, QString> except with
// custom equality and hash methods that save the cost of touching the QString.
class ChannelHandleAndGroup {
  public:
    ChannelHandleAndGroup(const ChannelHandle& handle, const QString& name)
            : m_handle(handle),
              m_name(name)
    {
    }
    const QString& name() const
    {
        return m_name;
    }
    const ChannelHandle& handle() const
    {
        return m_handle;
    }
    const ChannelHandle m_handle;
    const QString m_name;
};
bool operator==(const ChannelHandleAndGroup& g1, const ChannelHandleAndGroup& g2)
{
    return g1.handle() == g2.handle();
}

bool operator!=(const ChannelHandleAndGroup& g1, const ChannelHandleAndGroup& g2)
{
    return g1.handle() != g2.handle();
}
QDebug operator<<(QDebug stream, const ChannelHandleAndGroup& g)
{
    stream << "ChannelHandleAndGroup(" << g.name() << "," << g.handle() << ")";
    return stream;
}
uint qHash(const ChannelHandleAndGroup& handle_group)
{
    return qHash(handle_group.handle());
}
// A helper class used by EngineMaster to assign ChannelHandles to channel group
// strings. Warning: ChannelHandles produced by different ChannelHandleFactory
// objects are not compatible and will produce incorrect results when compared,
// stored in the same container, etc. In practice we only use one instance in
// EngineMaster.
class ChannelHandleFactory {
  public:
    ChannelHandleFactory() = default;
    ChannelHandle getOrCreateHandle(const QString& group)
    {
        auto& handle = m_groupToHandle[group];
        if (!handle.valid())
        {
            handle.setHandle(m_iNextHandle++);
            DEBUG_ASSERT(handle.valid());
            DEBUG_ASSERT(!m_handleToGroup.contains(handle));
            m_handleToGroup.insert(handle, group);
        }
        return handle;
    }
    ChannelHandle handleForGroup(const QString& group) const
    {
        return m_groupToHandle.value(group, ChannelHandle());
    }
    QString groupForHandle(const ChannelHandle& handle) const
    {
        return m_handleToGroup.value(handle, QString());
    }
  private:
    std::atomic<int> m_iNextHandle{0};
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
class ChannelHandleMap
{
  public:
    using size_type      = int;
    using difference_type= int;
    static const int kMaxExpectedGroups = 256;
    using container_type = QVarLengthArray<T, kMaxExpectedGroups>;
    using const_iterator = typename container_type::const_iterator;
    using iterator       = typename container_type::iterator;
    const T& at(const ChannelHandle& handle) const
    {
        if (!handle.valid())  return m_dummy;
        return m_data.at(handle.handle());
    }
    void insert(const ChannelHandle& handle, const T& value) {
        if (!handle.valid())  return;
        auto iHandle = handle.handle();
        maybeExpand(iHandle);
        m_data[iHandle] = value;
    }
    T& operator[](const ChannelHandle& handle)
    {
        if (!handle.valid()) return m_dummy;
        auto iHandle = handle.handle();
        maybeExpand(iHandle);
        return m_data[iHandle];
    }
    const T& operator[](const ChannelHandle& handle) const
    {
      if (!handle.valid()) return m_dummy;
      auto iHandle = handle.handle();
      maybeExpand(iHandle);
      return m_data[iHandle];
    }
    void clear()
    {
        m_data.clear();
    }
    iterator begin()
    {
        return m_data.begin();
    }
    const_iterator begin() const
    {
        return m_data.cbegin();
    }
    const_iterator cbegin() const
    {
        return m_data.cbegin();
    }
    iterator end()
    {
        return m_data.end();
    }
    const_iterator end() const
    {
        return m_data.cend();
    }
    const_iterator cend() const
    {
      return m_data.cend();
    }

  private:
    void maybeExpand(difference_type iIndex)
    {
        if (m_data.size() <= iIndex) m_data.resize(iIndex+1);
    }
    container_type m_data;
    T              m_dummy{};
};
