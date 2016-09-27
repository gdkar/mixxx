#include "intrusive_fifo.hpp"

namespace impl {
    intrusive_fifo_base::intrusive_fifo_base() = default;
    intrusive_fifo_base::~intrusive_fifo_base()
    {
        while(!empty())
            delete take();
    }
    intrusive_node *intrusive_fifo_base::skip_stub(void)
    {
        if(m_tail == &m_stub) {
            if(auto node = m_tail->next(std::memory_order_acquire)) {
                return m_tail = node;
            }else{
                return nullptr;
            }
        }else{
            return m_tail;
        }
    }
    bool intrusive_fifo_base::empty() const
    {
        return !(m_tail == &m_stub || m_tail->next(std::memory_order_acquire));
    }
    bool intrusive_fifo_base::empty()
    {
        return !(skip_stub());
    }
    intrusive_node *intrusive_fifo_base::begin() const
    {
        if(m_tail == &m_stub) {
            return m_tail->next(std::memory_order_relaxed);
        }else{
            return m_tail;
        }
    }
    intrusive_node *intrusive_fifo_base::begin()
    {
        return skip_stub();
    }
    intrusive_node &intrusive_fifo_base::front() const
    {
        return *begin();
    }
    intrusive_node &intrusive_fifo_base::front()
    {
        return *begin();
    }
    bool intrusive_fifo_base::pop()
    {
        if(!skip_stub())
            return false;
        if(auto next = m_tail->next(std::memory_order_acquire)) {
            m_tail = next;
            return true;
        }
        auto lhead = m_head.load(std::memory_order_acquire);
        if(lhead != m_tail) {
            return false;
        }
        push(&m_stub);
        if(auto next = m_tail->next(std::memory_order_acquire)) {
            return (m_tail = next);
        }
        return false;
    }
    intrusive_node *intrusive_fifo_base::take()
    {
        auto node = begin();
        if(pop())
            return node;
        return nullptr;
    }
    void intrusive_fifo_base::push(intrusive_node *node)
    {
        if(node) {
            node->set_next(nullptr,std::memory_order_release);
            auto prev = m_head.exchange(node,std::memory_order_acq_rel);
            prev->set_next(node,std::memory_order_relaxed);
        }
    }
}
