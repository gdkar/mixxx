_Pragma("once")

#include <memory>
#include <climits>
#include <cfloat>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <cmath>
#include <cstring>
#include <atomic>
#include <utility>
#include <algorithm>
#include <limits>
#include <numeric>

template<class Iter>
struct range {
    using size_type = size_t;//typename std::iterator_traits<Iter>::size_type;
    using difference_type = typename std::iterator_traits<Iter>::difference_type;
    using value_type = typename std::iterator_traits<Iter>::value_type;
    using reference  = typename std::iterator_traits<Iter>::reference;
    using pointer    = typename std::iterator_traits<Iter>::pointer;
    Iter _begin{};
    Iter _end  {};
    constexpr range() = default;
    constexpr range(const Iter &_b, const Iter &_e)
    : _begin(_b),_end(_e){}
    constexpr range(const range &o) = default;
    constexpr range(range &&o) noexcept = default;
    range &operator = (const range &o) = default;
    range &operator = (range &&o) noexcept = default;
    constexpr Iter begin() const { return _begin; }
    constexpr Iter end()   const { return _end; }
    constexpr Iter cbegin() const { return _begin; }
    constexpr Iter cend()   const { return _end; }
    constexpr size_type size() const { return std::distance(begin(),end());}
    constexpr reference operator[](difference_type idx) { return *std::next(begin(),idx);}
    constexpr const reference operator[](difference_type idx) const { return *std::next(begin(),idx);}
};

template<class Iter>
range<Iter> make_range(Iter _begin, Iter _end) { return range<Iter>(_begin,_end); }

template<class T>
constexpr std::enable_if_t<std::is_integral<T>::value,T>
next_pow_2(T x)
{
    using U = std::make_unsigned_t<T>;
    auto u = U(x) - U(1);
    for(auto i = 1; i < (sizeof(U) * CHAR_BIT)/2; i<<=1)
        u |= (u>>i);
    return T(u+U(1));
}
template<class T>
class Ringbuffer {
    protected:
        size_t                 m_capacity{0};
        size_t                 m_mask    {0};
        std::atomic<int64_t>   m_ridx{0};
        std::atomic<int64_t>   m_widx{0};
        std::unique_ptr<T[]>   m_data{};
    public:
        using value_type = T;
        using size_type       = size_t;
        using difference_type = int64_t;
        using reference       = T&;
        using const_reference = const T&;
        using pointer         = T*;
        using const_pointer   = const T*;
        constexpr Ringbuffer() = default;
        constexpr Ringbuffer(size_type cap)
        : m_capacity(next_pow_2(cap))
        , m_mask(m_capacity - 1)
        , m_data(std::make_unique<T[]>(m_capacity)){}
        Ringbuffer(Ringbuffer &&o) noexcept
            : m_capacity(std::exchange(o.m_capacity,0))
            ,m_mask(std::exchange(o.m_mask,0))
            ,m_ridx(o.m_ridx.exchange(0))
            ,m_widx(o.m_widx.exchange(0))
        { m_data.swap(o.m_data);}
        Ringbuffer &operator =(Ringbuffer &&o) noexcept
        { 
            m_ridx.exchange(o.m_ridx.exchange(0));
            m_widx.exchange(o.m_widx.exchange(0));
            m_data.swap(o.m_data);
        }
       ~Ringbuffer() = default;
        constexpr size_type capacity() const { return m_capacity; }
        constexpr size_type mask() const { return m_mask;}        
        difference_type read_index() const { return m_ridx.load();}
        difference_type write_index() const { return m_widx.load();}
        size_type read_offset() const { return read_index() & mask();}
        size_type write_offset() const { return write_index() & mask();}
        size_type size() const { return write_index() - read_index();}
        size_type space()const { return (read_index() + capacity()) - write_index();}
        bool      full() const { return (read_index() + capacity()) == write_index();}
        bool      empty()const { return read_index() == write_index();}

        struct iterator {
            using iterator_category = std::random_access_iterator_tag;
            using value_type = T;
            using size_type       = size_t;
            using difference_type = int64_t;
            using reference       = T&;
            using const_reference = const T&;
            using pointer         = T*;
            using const_pointer   = const T*;

            pointer         m_ptr{nullptr};
            difference_type m_idx{0};
            size_type       m_mask{0};
            constexpr iterator() = default;
            constexpr iterator(pointer ptr, difference_type idx, size_type _mask)
            : m_ptr(ptr)
            , m_idx(idx)
            , m_mask(_mask){}
            constexpr iterator(Ringbuffer &fifo, difference_type idx = 0)
            : m_ptr(&fifo.m_data[0])
            , m_idx(idx)
            , m_mask(fifo.m_mask){}
            constexpr iterator(const iterator &o) = default;
            constexpr iterator(iterator &&o) noexcept = default;
            iterator &operator=(const iterator &o) = default;
            iterator &operator=(iterator &&)  = default;
            constexpr size_type mask() const { return m_mask;}
            constexpr size_type offset() const { return m_idx & mask();}
            constexpr bool operator ==(const iterator& o) const { return m_ptr == o.m_ptr && m_idx == o.m_idx;}
            constexpr bool operator !=(const iterator& o) const { return !(*this == o);}
            constexpr bool operator  <(const iterator& o) const { return m_ptr == o.m_ptr && (m_idx < o.m_idx);}
            constexpr bool operator  >(const iterator& o) const { return m_ptr == o.m_ptr && (m_idx > o.m_idx);}
            constexpr bool operator <=(const iterator& o) const { return m_ptr == o.m_ptr && (m_idx <= o.m_idx);}
            constexpr bool operator >=(const iterator& o) const { return m_ptr == o.m_ptr && (m_idx >= o.m_idx);}
            constexpr iterator operator +(difference_type diff) { return iterator(m_ptr, m_idx+diff, m_mask);}
            constexpr iterator operator -(difference_type diff) { return iterator(m_ptr, m_idx+diff, m_mask);}
            constexpr difference_type operator - (const iterator &o) { return (m_ptr == o.m_ptr) ? m_idx - o.m_idx : throw std::invalid_argument("iterators not to same object.");}
            void swap(iterator &o)  noexcept
            {
                using std::swap;
                swap(m_ptr,o.m_ptr);
                swap(m_idx,o.m_idx);
            }
            iterator &operator ++(){m_idx++;return *this;}
            iterator &operator ++(int){auto ret = *this;++*this;return ret;}
            iterator &operator --(){m_idx--;return *this;}
            iterator &operator --(int){auto ret = *this;--*this;return ret;}
            iterator &operator +=(difference_type diff) { m_idx += diff; return *this;}
            iterator &operator -=(difference_type diff) { m_idx -= diff; return *this;}
            constexpr pointer operator ->() { return m_ptr + offset();}
            constexpr const_pointer operator ->() const { return m_ptr + offset();}
            constexpr reference operator *() { return m_ptr[offset()];}
            constexpr const_reference operator *() const { return m_ptr[offset()];}
            constexpr reference operator[](difference_type diff) { return m_ptr[(m_idx + diff) & mask()];}
            constexpr const_reference operator[](difference_type diff) const { return m_ptr[(m_idx + diff) & mask()];}
        };
        using range_type      = range<iterator>;
        range_type read_range() 
        {
            return make_range(iterator(*this, read_index()),iterator(*this,write_index()));
        }
        range_type write_range() 
        {
            return make_range(iterator(*this, write_index()),iterator(*this,read_index() + capacity()));
        }
        range_type read_range(size_type _size, difference_type _diff = 0) 
        {
            _size = std::min<size_type>(_size,std::max<difference_type>(size() - _diff,0));
            return make_range(iterator(*this, read_index() + _size),iterator(*this,read_index() + _size + _diff));
        }
        range_type write_range(size_type _size) 
        {
            _size = std::min<size_type>(_size,space());
            return make_range(iterator(*this, write_index()),iterator(*this,write_index() + _size));
        }
        size_type write_advance(size_type count)
        {
            count = std::min(count,space());
            m_widx.fetch_add(count);
            return count;
        }
        size_type read_advance(size_type count)
        {
            count = std::min(count,size());
            m_ridx.fetch_add(count);
            return count;
        }
        template<class Iter>
        size_type peek_n(Iter start, size_type count, difference_type offset = 0)
        {
            auto r = read_range(count,offset);
            auto stop = std::copy_n(r.cbegin(), count, start);
            return std::distance(start,stop);
        }
        template<class Iter>
        Iter peek(Iter start, Iter stop, difference_type offset = 0)
        {
            auto count = std::distance(start,stop);
            auto r = read_range(count,offset);
            return std::copy(r.cbegin(), r.cend(), start);
        }
        template<class Iter>
        size_type read_n(Iter start, size_type count)
        {
            count = std::min(count, size());
            auto r = read_range(count);
            std::copy_n(r.begin(), r.size(), start);
            read_advance(count);
            return count;
        }
        template<class Iter>
        Iter read(Iter start, Iter stop)
        {
            auto r = read_range(std::distance(start,stop));
            stop = std::copy(r.cbegin(), r.cend(), start);
            m_ridx.fetch_add(r.size());
            return stop;
        }
        template<class Iter>
        size_type write_n(Iter start, size_type count)
        {
            count = std::min(count,space());
            auto r = write_range(count);
            std::copy_n(start,count,r.begin());
            write_advance(count);
            return count;
        }
        template<class Iter>
        Iter write(Iter start, Iter stop)
        {
            auto r = write_range(std::distance(start,stop));
            std::copy_n(start,r.size(),r.begin());
            m_widx.fetch_add(r.size());
            return std::next(start,r.size());
        }
        iterator begin() { return iterator(*this, read_index());}
        iterator end()   { return iterator(*this, write_index());}
        reference front(){ return m_data[read_index()];}
        reference back (){ return m_data[write_index()];}
        void push_back(const_reference item)
        {
            if(!full()) { back() = item; m_widx.fetch_add(1);}
            else { throw std::invalid_argument("push onto full fifo.");}
        }
        void pop_front()
        {
            if(!empty()) {m_ridx.fetch_add(1);}
            else { throw std::invalid_argument("pop from empty fifo.");}
        }
        bool try_push_back(const_reference item)
        {
            if(full())
                return false;
            back() = item;
            m_widx.fetch_add(1);
            return true;
        }
        bool try_pop_front(reference item)
        {
            if(empty())
                return false;
            item = front();
            m_ridx.fetch_add(1);
            return true;
        }
};
