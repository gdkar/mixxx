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
    using size_type       = size_t;//typename std::iterator_traits<Iter>::size_type;
    using difference_type = typename std::iterator_traits<Iter>::difference_type;
    using value_type      = typename std::iterator_traits<Iter>::value_type;
    using reference       = typename std::iterator_traits<Iter>::reference;
    using pointer         = typename std::iterator_traits<Iter>::pointer;
    Iter _begin{};
    Iter _end  {};
    constexpr range() = default;
    constexpr range(const Iter &_b, const Iter &_e)
    : _begin(_b),_end(_e){}
    constexpr range(const range &o)        = default;
    constexpr range(range &&o) noexcept    = default;
    range &operator = (const range &o)     = default;
    range &operator = (range &&o) noexcept = default;
    constexpr Iter begin() const { return _begin; }
    constexpr Iter end()   const { return _end; }
    constexpr Iter cbegin()const { return _begin; }
    constexpr Iter cend()  const { return _end; }
    constexpr size_type size() const { return std::distance(begin(),end());}
    constexpr reference operator[](difference_type idx) { return *std::next(begin(),idx);}
    constexpr const reference operator[](difference_type idx) const { return *std::next(begin(),idx);}
};

template<class Iter>
range<Iter> make_range(Iter _begin, Iter _end) { return range<Iter>(_begin,_end); }

template<class Container>
range<typename Container::iterator> iter_range(Container && c)
{
    using std::begin;
    using std::end;
    return make_range(begin(std::forward<Container>(c)),
                      end  (std::forward<Container>(c)));
}


