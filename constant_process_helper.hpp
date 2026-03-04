#ifndef constant_process_helper_hpp
#define constant_process_helper_hpp

#include <ql/processes/blackscholesprocess.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>
#include <ql/quotes/simplequote.hpp>

namespace QuantLib {

struct ConstantBSParams {
    Real spot;
    Rate r;
    Rate q;
    Volatility sigma;
};

inline ConstantBSParams extractConstantBSParams(
    const ext::shared_ptr<GeneralizedBlackScholesProcess>& process,
    const Date& maturity,
    Real strike) {

    QL_REQUIRE(process, "null process");

    // 1) Spot
    const Real spot = process->stateVariable()->value();

    // 2) r(T) = zero-rate risk-free à maturité (continu)
    const auto& rCurve = process->riskFreeRate();
    QL_REQUIRE(!rCurve.empty(), "null risk-free curve");
    const DayCounter rDc = rCurve->dayCounter();
    const Rate r = rCurve->zeroRate(maturity, rDc, Continuous).rate();

    // 3) q(T) = zero-rate dividend à maturité (continu)
    const auto& qCurve = process->dividendYield();
    QL_REQUIRE(!qCurve.empty(), "null dividend curve");
    const DayCounter qDc = qCurve->dayCounter();
    const Rate q = qCurve->zeroRate(maturity, qDc, Continuous).rate();

    // 4) sigma(T,K) = Black vol à maturité & strike
    const auto& volSurf = process->blackVolatility();
    QL_REQUIRE(!volSurf.empty(), "null volatility");
    const Volatility sigma = volSurf->blackVol(maturity, strike);

    return ConstantBSParams{spot, r, q, sigma};
}

} // namespace QuantLib

#endif