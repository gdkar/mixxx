#include "ReassignedFFT.hpp"
#include "KaiserWindow.hpp"
using namespace RBMixxxVamp ;
namespace detail {
struct _wisdom_reg {
    _wisdom_reg(){
        fftwf_init_threads();
        fftwf_make_planner_thread_safe();
        fftw_init_threads();
        fftw_make_planner_thread_safe();

        wisdom(false);
    }
   ~_wisdom_reg() {
        wisdom(true);
    }
    void wisdom(bool save) {
        if(auto home = getenv("HOME")){
            char fn[256];
            snprintf(fn, sizeof(fn), "%s/%s.%c", home, ".rubberband.wisdom", 'f');
            if(auto f = ::fopen(fn, save ? "wb" : "rb")){
                if (save)
                    fftwf_export_wisdom_to_file(f);
                else
                    fftwf_import_wisdom_from_file(f);
                fclose(f);
            }
            snprintf(fn, sizeof(fn), "%s/%s.%c", home, ".rubberband.wisdom", 'd');
            if(auto f = ::fopen(fn, save ? "wb" : "rb")){
                if (save)
                    fftw_export_wisdom_to_file(f);
                else
                    fftw_import_wisdom_from_file(f);
                fclose(f);
            }
        }
    }
};
_wisdom_reg the_registrar{};
}
RMFFT::RMFFT(int _size)
: m_size(_size)
{
    if(_size) {
        auto dims = fftwf_iodim{ m_size, 1, 1};
        auto _real = &m_split[0]; auto _imag = &m_split[m_spacing]; auto _time = &m_flat[0];

        m_plan_r2c = fftwf_plan_guru_split_dft_r2c(
            1, &dims, 0, nullptr, _time, _real, _imag, FFTW_ESTIMATE);
        dims = fftwf_iodim{ m_size, 1, 1};
        m_plan_c2r = fftwf_plan_guru_split_dft_c2r(
            1, &dims, 0, nullptr, _real, _imag, _time, FFTW_ESTIMATE);
    }
}
/*static*/ RMFFT RMFFT::Kaiser(int _size, float alpha)
{
    auto win    = vector_type(_size, 0.0f);
    auto win_dt = vector_type(_size, 0.0f);
    make_kaiser_window(win.begin(),win.end(), alpha);
    return RMFFT(win.cbegin(),win.cend());
}

RMFFT& RMFFT::operator=(RMFFT && o ) noexcept
{
    swap(o);
    return *this;
}
void RMFFT::swap(RMFFT &o) noexcept
{
    using std::swap;
    swap(m_size,o.m_size);
    swap(m_coef,o.m_coef);
    swap(m_spacing,o.m_spacing);
    swap(m_h,o.m_h);
    swap(m_Dh,o.m_Dh);
    swap(m_Th,o.m_Th);
    swap(m_TDh,o.m_TDh);
    swap(m_flat,o.m_flat);
    swap(m_split,o.m_split);
    swap(m_X,o.m_X);
    swap(m_X_Dh,o.m_X_Dh);
    swap(m_X_Th,o.m_X_Th);
    swap(m_X_TDh,o.m_X_TDh);
    swap(m_plan_r2c,o.m_plan_r2c);
    swap(m_plan_c2r,o.m_plan_c2r);
}
RMFFT::RMFFT(RMFFT && o ) noexcept
: RMFFT(0)
{
    swap(o);
}
RMFFT::~RMFFT()
{
    if(m_size) {
        if(m_plan_r2c) {
            fftwf_destroy_plan(m_plan_r2c);
            m_plan_r2c = 0;
        }
        if(m_plan_c2r) {
            fftwf_destroy_plan(m_plan_c2r);
            m_plan_c2r = 0;
        }
    }
}

void RMFFT::_finish_process( RMSpectrum & dst, int64_t _when )
{
    using reg = simd_reg<float>;
    constexpr auto w = int(simd_width<float>);

    const auto _real = &m_X[0], _imag    = &m_X[m_spacing]
        ,_real_Dh = &m_X_Dh[0], _imag_Dh = &m_X_Dh[m_spacing]
        ,_real_Th = &m_X_Th[0], _imag_Th = &m_X_Th[m_spacing]
        ;
    auto _cmul = [](auto r0, auto i0, auto r1, auto i1) {
        return std::make_pair(r0 * r1 - i0 * i1, r0 * i1 + r1 * i0);
    };
    auto _cinv = [](auto r, auto i) {
        auto n = bs::rec(bs::sqr(r) + bs::sqr(i) + 1e-6f);//bs::Eps<float>());
        return std::make_pair(r * n , -i * n);
    };
    dst.reset(m_size, _when);
    std::copy(_real, _real + m_coef, dst.X_real());
    std::copy(_imag, _imag + m_coef, dst.X_imag());

    for(auto i = 0; i < m_coef; i += w ) {
        auto _X_r = reg(_real + i), _X_i = reg(_imag + i);
        {
        auto _X_mag = bs::sqr(_X_i) + bs::sqr(_X_r);
//            auto _X_mag = bs::hypot(_X_i,_X_r);
            bs::store(bs::sqrt(_X_mag), dst.mag_data() + i);
            bs::store(bs::log(_X_mag), dst.M_data() + i);
            bs::store(bs::atan2(_X_i,_X_r), dst.Phi_data() + i);
        }

        std::tie(_X_r, _X_i) = _cinv(_X_r,_X_i);

        auto _Dh_over_X = _cmul( reg(_real_Dh + i),reg(_imag_Dh + i) ,_X_r, _X_i );

        bs::store(std::get<0>(_Dh_over_X), &dst.dM_dt  [0] + i);
        bs::store(std::get<1>(_Dh_over_X), &dst.dPhi_dt[0] + i);

        auto _Th_over_X = _cmul( reg(_real_Th + i),reg(_imag_Th + i) ,_X_r, _X_i );

        bs::store(-std::get<1>(_Th_over_X), &dst.dM_dw  [0] + i);
        bs::store( std::get<0>(_Th_over_X), &dst.dPhi_dw[0] + i);
    }
    for(auto i = 0; i < m_coef; ++i) {
        auto _X_r = *(_real + i), _X_i = *(_imag + i);
        {
//            auto _X_mag = bs::hypot(_X_i,_X_r);
            auto _X_mag = bs::sqr(_X_i) + bs::sqr(_X_r);
            bs::store(bs::sqrt(_X_mag), dst.mag_data() + i);
            bs::store(bs::log(_X_mag), dst.M_data() + i);
            bs::store(bs::atan2(_X_i,_X_r), dst.Phi_data() + i);
        }
        std::tie(_X_r, _X_i) = _cinv(_X_r,_X_i);

        auto _Dh_over_X = _cmul( *(_real_Dh + i),*(_imag_Dh + i) ,_X_r, _X_i );

        bs::store(std::get<0>(_Dh_over_X), &dst.dM_dt  [0] + i);
        bs::store(std::get<1>(_Dh_over_X), &dst.dPhi_dt[0] + i);

        auto _Th_over_X = _cmul( *(_real_Th + i),*(_imag_Th + i) ,_X_r, _X_i );

        bs::store(-std::get<1>(_Th_over_X), &dst.dM_dw  [0] + i);
        bs::store( std::get<0>(_Th_over_X), &dst.dPhi_dw[0] + i);
    }
}
int RMFFT::spacing() const
{
    return m_spacing;
}
int RMFFT::size() const
{
    return m_size;
}
int RMFFT::coefficients() const
{
    return m_coef;
}
