#include "constantblackscholesprocess.hpp"
#include <cmath>

using namespace QuantLib;

ConstantBlackScholesProcess::ConstantBlackScholesProcess(
    Real s0,
    Rate r,
    Rate q,
    Volatility sigma)
: s0_(s0), r_(r), q_(q), sigma_(sigma) {}

Real ConstantBlackScholesProcess::x0() const {
    return s0_;
}

Real ConstantBlackScholesProcess::drift(Time,
                                        Real x) const {
    return (r_ - q_) * x;
}

Real ConstantBlackScholesProcess::diffusion(Time,
                                            Real x) const {
    return sigma_ * x;
}

Real ConstantBlackScholesProcess::evolve(Time,
                                         Real x0,
                                         Time dt,
                                         Real dw) const {

    // Euler discretization
    return x0
           + drift(0.0, x0) * dt
           + diffusion(0.0, x0) * std::sqrt(dt) * dw;
}