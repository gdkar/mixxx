#ifndef UTIL_INTRUSIVE_FIFO_HPP
#define UTIL_INTRUSIVE_FIFO_HPP

#include <atomic>
#include <memory>
#include <utility>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>


struct intrusive_node {
    std::atomic<intrusive_node *> m_next{};
    constexpr intrusive_node() = default;
    constexpr intrusive_node(intrusive_node *_next)
    : m_next(_next) {}
    virtual ~intrusive_node()  = default;
    intrusive_node *next(std::memory_order ord = std::memory_order_seq_cst) const
    {
        return m_next.load(ord);
    }
    intrusive_node *exchange(intrusive_node *with, std::memory_order ord = std::memory_order_seq_cst)
    {
        return m_next.exchange(with,ord);
    }
    bool compare_exchange_strong(
        intrusive_node *&expected
      , intrusive_node *desired
      , std::memory_order ord_success = std::memory_order_seq_cst
      , std::memory_order ord_failure = std::memory_order_seq_cst)
    {
        return m_next.compare_exchange_strong(expected,desired,ord_success,ord_failure);
    }
    void set_next(intrusive_node *_next,std::memory_order ord = std::memory_order_seq_cst)
    {
        m_next.store(_next,ord);
    }
};
namespace impl {
class intrusive_fifo_base {
protected:
    intrusive_node                m_stub{nullptr};
    std::atomic<intrusive_node *> m_head{&m_stub};
    intrusive_node             *  m_tail{&m_stub};
    intrusive_node *skip_stub(void);
public:
    intrusive_fifo_base();
    virtual ~intrusive_fifo_base();
    bool empty();
    bool empty() const;
    intrusive_node *begin();
    intrusive_node *begin() const;
    intrusive_node &front();
    intrusive_node &front() const;
    intrusive_node *take();
    bool            pop();
    void            push(intrusive_node *node);
};
}
template<class T>
class intrusive_fifo : public impl::intrusive_fifo_base {
public:
    using super = impl::intrusive_fifo_base;
             intrusive_fifo() = default;
    virtual ~intrusive_fifo() = default;
    using super::empty;
    using super::pop;
    T *begin() { return static_cast<T*>(super::begin());}
    T *begin() const { return static_cast<const T*>(super::begin());}
    T &front() { return static_cast<T&>(super::front());}
    T &front() const { return static_cast<const T&>(super::front());}
    T *take() { return static_cast<T*>(super::take());}
    void push(T *node) { super::push(static_cast<intrusive_node*>(node));}
};
#endif
