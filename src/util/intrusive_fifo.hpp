#ifndef UTIL_INTRUSIVE_FIFO_HPP
#define UTIL_INTRUSIVE_FIFO_HPP

#include <atomic>
#include <memory>
#include <utility>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace intrusive {

struct node {
    std::atomic<node *> m_next{};
    constexpr node() = default;
    constexpr node(node *_next)
    : m_next(_next) {}
    virtual ~node()  = default;
    node *next(std::memory_order ord = std::memory_order_seq_cst) const
    {
        return m_next.load(ord);
    }
    node *exchange(node *with, std::memory_order ord = std::memory_order_seq_cst)
    {
        return m_next.exchange(with,ord);
    }
    bool compare_exchange_strong(
        node *&expected
      , node *desired
      , std::memory_order ord_success = std::memory_order_seq_cst
      , std::memory_order ord_failure = std::memory_order_seq_cst)
    {
        return m_next.compare_exchange_strong(expected,desired,ord_success,ord_failure);
    }
    void set_next(node *_next,std::memory_order ord = std::memory_order_seq_cst)
    {
        m_next.store(_next,ord);
    }
};
namespace impl {
class fifo_base {
protected:
    intrusive::node                m_stub{nullptr};
    std::atomic<node *> m_head{&m_stub};
    intrusive::node               *m_tail{&m_stub};
    intrusive::node *skip_stub(void);
public:
    using node_type = intrusive::node;
    template<class T>
    struct iterator_base {
        node_type *m_node{};
        const fifo_base* m_fifo{};
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = value_type*;
        using reference = value_type&;
        constexpr iterator_base() = default;
        constexpr iterator_base(const iterator_base&) = default;
        template<class U>
        constexpr iterator_base(const iterator_base<U> & other)
        : m_node(static_cast<T*>(other.m_node))
        , m_fifo(other.m_fifo)
        {}
        template<class U>
        iterator_base&operator=(const iterator_base<U> & other)
        {
            m_node = static_cast<T*>(other.m_node);
            m_fifo = other.m_fifo;
        }
        iterator_base(const fifo_base &_fifo, node_type *_node)
        : m_node(_node)
        , m_fifo(&_fifo)
        {
            if(!m_node) {
                m_fifo = nullptr;
                return;
            }
            if(m_node == std::addressof(m_fifo->m_stub)) {
                if(!(m_node = m_node->next())){
                    m_fifo = nullptr;
                }
            }
        }
        iterator_base(const fifo_base &_fifo)
        : m_node(_fifo.m_tail )
        , m_fifo(&_fifo)
        {
            if(!m_node) {
                m_fifo = nullptr;
                return;
            }
            if(m_node == std::addressof(m_fifo->m_stub)) {
                if(!(m_node = m_node->next())){
                    m_fifo = nullptr;
                }
            }
        }
        iterator_base(fifo_base &_fifo)
        : m_node(_fifo.skip_stub())
        , m_fifo(&_fifo)
        {
            if(!m_node) {
                m_fifo = nullptr;
            }
        }
        iterator_base &operator=(const iterator_base &) = default;
        void swap(iterator_base &other) noexcept
        {
            using std::swap;
            swap(m_node,other.m_node);
            swap(m_fifo,other.m_fifo);
        }
        constexpr reference operator *() const
        {
            return *static_cast<T*>(m_node);
        }
        constexpr pointer operator ->() const
        {
            return static_cast<T*>(m_node);
        }
        iterator_base &operator ++()
        {
            if(m_node) {
                auto _next = m_node->next();
                if(_next == std::addressof(m_fifo->m_stub)) {
                    if(auto _nnext = _next->next()) {
                        m_node->set_next(_nnext);
                        m_node = _nnext;
                    }else{
                        m_node = nullptr;
                        m_fifo = nullptr;
                    }
                }else{
                    m_node = _next;
                }
                m_node = m_node->next();
                if(m_node == std::addressof(m_fifo->m_stub))
                    m_node = m_node->next();
            }
            return *this;
        }
        iterator_base operator++(int)
        {
            auto retval = *this;
            ++*this;
            return retval;
        }
        constexpr bool operator == ( const iterator_base &other) const
        {
            return (m_node == other.m_node && m_fifo == other.m_fifo);
        }
        constexpr bool operator != ( const iterator_base &other) const
        {
            return !(*this == other);
        }
        constexpr operator pointer() const
        {
            return static_cast<T*>(m_node);
        }
    };
    using iterator = iterator_base<node_type>;
    using const_iterator = iterator_base<const node_type>;
    fifo_base();
    virtual ~fifo_base();
    bool empty();
    bool empty() const;
    iterator        begin();
    const_iterator  begin() const;
    const_iterator cbegin() const;
    iterator end()              { return iterator();}
    const_iterator end() const  { return const_iterator();}
    const_iterator cend() const { return const_iterator();}

    node_type &front();
    const node_type &front() const;
    node_type *take();
    bool            pop();
    void            push(node_type *node);
    void            push_back(node_type*node);
    void            push_front(node_type*node);
};
}
template<class T>
class fifo : public impl::fifo_base {
public:
    using node_type = T;
    using super = impl::fifo_base;
             fifo() = default;
    virtual ~fifo() = default;
    using super::empty;
    using super::pop;
    using iterator = super::iterator_base<node_type>;
    using const_iterator = super::iterator_base<const node_type>;
    iterator begin() { return iterator(super::begin());}
    iterator end() { return iterator();}
    const_iterator end() const { return const_iterator();}
    const_iterator cend() const { return const_iterator();}
    const_iterator begin() const { return const_iterator(super::begin());}
    const_iterator cbegin() const { return const_iterator(super::cbegin());}
    T &front() { return static_cast<T&>(super::front());}
    const T &front() const { return static_cast<const T&>(super::front());}
    T *take() { return static_cast<T*>(super::take());}
    void push(T *node) { super::push(static_cast<super::node_type*>(node));}
};
}
#endif
