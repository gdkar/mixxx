_Pragma("once")

#include <utility>
#include <iterator>
#include <algorithm>
#include <type_traits>
#include <numeric>

namespace mixxx {

template<class Iter, class Func, class K = std::result_of_t<Func(decltype(*Iter{}))> >
std::enable_if_t<
    std::is_base_of<
        std::bidirectional_iterator_tag
      , typename std::iterator_traits<Iter>::iterator_category
        >::value
 && std::is_arithmetic<K>::value
  , Iter
    > proportional_search(Iter _beg, Iter _end, const K& value, Func func)
{
    if(_beg == _end)
        return _end;
    auto lval = func(*_beg);
    if(value < lval)
        return _end;
    --_end;
    auto idx_dist = _end - _beg;
    auto hval     = func(*_end);

    if(value >= hval)
        return _end;

    auto bail     = false;

    while(_beg != _end && _beg + 1 != _end) {
        auto val_dist  = hval - lval;
        auto tgt_dist  = value - lval;
        auto mid_dist  = decltype(idx_dist)((idx_dist * tgt_dist) / val_dist);
        auto half_dist = idx_dist >> 1;
        if(mid_dist >= idx_dist) {
            if(bail) {
                mid_dist = half_dist;
            }else{
                mid_dist = idx_dist - 1;
                bail = true;
            }
        }else if(mid_dist <= 0) {
            if(bail) {
                mid_dist = half_dist;
            }else{
                mid_dist = 1;
                bail = true;
            }
        }else{
            bail = false;
        }
        auto _mid =  std::next(_beg, mid_dist);
        auto mval = func(*_mid);
        if(mval > value) {
            hval = mval;
            idx_dist = mid_dist;
            _end = _mid;
        }else if(mval == value) {
            return _mid;
        }else{
            lval = mval;
            idx_dist -= mid_dist;
            _beg = _mid;
        }
    }
    return _beg;
}

};
