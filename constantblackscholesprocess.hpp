#ifndef constant_black_scholes_process_hpp
#define constant_black_scholes_process_hpp


#include <ql/stochasticprocess.hpp>

namespace QuantLib {

    class ConstantBlackScholesProcess : public StochasticProcess1D {

    public:

        ConstantBlackScholesProcess(Real s0,
                                    Rate r,
                                    Rate q,
                                    Volatility sigma);

        Real x0() const override;
        Real drift(Time t, Real x) const override;
        Real diffusion(Time t, Real x) const override;
        Real evolve(Time t0, Real x0, Time dt, Real dw) const override;

    private:

        Real s0_;
        Rate r_;
        Rate q_;
        Volatility sigma_;
    };

}

#endif