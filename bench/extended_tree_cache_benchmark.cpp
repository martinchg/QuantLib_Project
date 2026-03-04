#include <ql/qldefines.hpp>
#ifdef BOOST_MSVC
#    include <ql/auto_link.hpp>
#endif

#include <ql/errors.hpp>
#include <ql/experimental/lattices/extendedbinomialtree.hpp>
#include <ql/instruments/vanillaoption.hpp>
#include <ql/methods/lattices/binomialtree.hpp>
#include <ql/pricingengines/vanilla/binomialengine.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/time/calendars/target.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace QuantLib;

namespace {

class CachedExtendedCoxRossRubinstein : public Tree<CachedExtendedCoxRossRubinstein> {
  public:
    enum Branches { branches = 2 };

    CachedExtendedCoxRossRubinstein(const ext::shared_ptr<StochasticProcess1D>& process,
                                    Time end,
                                    Size steps,
                                    Real)
    : Tree<CachedExtendedCoxRossRubinstein>(steps + 1),
      x0_(process->x0()),
      dt_(end / steps),
      treeProcess_(process),
      driftStepCache_(steps + 1, 0.0),
      dxStepCache_(steps + 1, 0.0),
      puCache_(steps + 1, 0.0),
      cached_(steps + 1, false) {}

    Size size(Size i) const {
        return i + 1;
    }

    Size descendant(Size, Size index, Size branch) const {
        return index + branch;
    }

    Real underlying(Size i, Size index) const {
        ensureStepCached(i);
        BigInteger j = 2 * BigInteger(index) - BigInteger(i);
        return x0_ * std::exp(j * dxStepCache_[i]);
    }

    Real probability(Size i, Size, Size branch) const {
        ensureStepCached(i);
        const Real upProb = puCache_[i];
        const Real downProb = 1.0 - upProb;
        return (branch == 1 ? upProb : downProb);
    }

  private:
    void ensureStepCached(Size i) const {
        if (cached_[i]) {
            return;
        }

        const Time stepTime = i * dt_;
        const Real driftStep = treeProcess_->drift(stepTime, x0_) * dt_;
        const Real dxStep = treeProcess_->stdDeviation(stepTime, x0_, dt_);
        const Real pu = 0.5 + 0.5 * driftStep / dxStep;

        QL_REQUIRE(pu <= 1.0, "negative probability");
        QL_REQUIRE(pu >= 0.0, "negative probability");

        driftStepCache_[i] = driftStep;
        dxStepCache_[i] = dxStep;
        puCache_[i] = pu;
        cached_[i] = true;
    }

    Real x0_;
    Time dt_;
    ext::shared_ptr<StochasticProcess1D> treeProcess_;

    mutable std::vector<Real> driftStepCache_;
    mutable std::vector<Real> dxStepCache_;
    mutable std::vector<Real> puCache_;
    mutable std::vector<bool> cached_;
};

template <class EngineBuilder>
std::pair<Real, double> timedNpv(const VanillaOption& baseOption,
                                 EngineBuilder&& builder,
                                 Size repeats) {
    double totalMs = 0.0;
    Real npv = Null<Real>();

    for (Size n = 0; n < repeats; ++n) {
        VanillaOption option = baseOption;
        option.setPricingEngine(builder());

        const auto start = std::chrono::steady_clock::now();
        npv = option.NPV();
        const auto end = std::chrono::steady_clock::now();

        totalMs += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() /
                   1000.0;
    }

    return {npv, totalMs / static_cast<double>(repeats)};
}

} // namespace

int main() {
    try {
        const Calendar calendar = TARGET();
        const Date today(4, March, 2026);
        Settings::instance().evaluationDate() = today;

        const DayCounter dc = Actual365Fixed();
        const Real spotValue = 100.0;
        const Real strike = 100.0;
        const Date maturity = today + 2 * Years;

        const Handle<Quote> spot(ext::make_shared<SimpleQuote>(spotValue));

        const std::vector<Date> rDates = {today, today + 6 * Months, today + 1 * Years, maturity};
        const std::vector<Rate> rRates = {0.005, 0.02, 0.045, 0.08};
        const Handle<YieldTermStructure> riskFree(
            ext::make_shared<ZeroCurve>(rDates, rRates, dc));

        const std::vector<Date> qDates = {today, today + 1 * Years, maturity};
        const std::vector<Rate> qRates = {0.0, 0.01, 0.025};
        const Handle<YieldTermStructure> dividend(
            ext::make_shared<ZeroCurve>(qDates, qRates, dc));

        const std::vector<Date> volDates = {
            today + 3 * Months, today + 9 * Months, today + 15 * Months, maturity};
        const std::vector<Volatility> vols = {0.12, 0.20, 0.26, 0.30};
        const Handle<BlackVolTermStructure> vol(
            ext::make_shared<BlackVarianceCurve>(today, volDates, vols, dc));

        const auto process =
            ext::make_shared<BlackScholesMertonProcess>(spot, dividend, riskFree, vol);

        const auto payoff = ext::make_shared<PlainVanillaPayoff>(Option::Put, strike);
        const auto europeanExercise = ext::make_shared<EuropeanExercise>(maturity);
        const auto americanExercise = ext::make_shared<AmericanExercise>(today, maturity);

        const VanillaOption europeanOption(payoff, europeanExercise);
        const VanillaOption americanOption(payoff, americanExercise);

        std::cout << "European option benchmark (non-constant curves/surface)\n";
        std::cout << std::left << std::setw(8) << "steps" << std::setw(16) << "ext NPV"
                  << std::setw(14) << "ext ms" << std::setw(16) << "cached NPV"
                  << std::setw(14) << "cached ms" << "speedup\n";

        const std::vector<Size> stepsGrid = {100, 250, 500, 1000, 2000, 4000};
        for (Size steps : stepsGrid) {
            const auto extRes = timedNpv(
                europeanOption,
                [&]() {
                    return ext::make_shared<BinomialVanillaEngine<ExtendedCoxRossRubinstein>>(
                        process, steps);
                },
                5);

            const auto cachedRes = timedNpv(
                europeanOption,
                [&]() {
                    return ext::make_shared<BinomialVanillaEngine<CachedExtendedCoxRossRubinstein>>(
                        process, steps);
                },
                5);

            const double speedup = extRes.second / cachedRes.second;

            std::cout << std::left << std::setw(8) << steps << std::setw(16) << extRes.first
                      << std::setw(14) << extRes.second << std::setw(16) << cachedRes.first
                      << std::setw(14) << cachedRes.second << speedup << "x\n";
        }

        std::cout << "\nConstant vs time-dependent trees (same non-constant process)\n";
        std::cout << std::left << std::setw(8) << "steps" << std::setw(18) << "Euro diff"
                  << "Amer diff\n";

        for (Size compareSteps : std::vector<Size>{100, 500, 1000, 2000, 4000}) {
            const auto euroConst = timedNpv(
                europeanOption,
                [&]() {
                    return ext::make_shared<BinomialVanillaEngine<CoxRossRubinstein>>(process,
                                                                                       compareSteps);
                },
                5);

            const auto euroTimeDep = timedNpv(
                europeanOption,
                [&]() {
                    return ext::make_shared<BinomialVanillaEngine<ExtendedCoxRossRubinstein>>(
                        process, compareSteps);
                },
                5);

            const auto amerConst = timedNpv(
                americanOption,
                [&]() {
                    return ext::make_shared<BinomialVanillaEngine<CoxRossRubinstein>>(process,
                                                                                       compareSteps);
                },
                5);

            const auto amerTimeDep = timedNpv(
                americanOption,
                [&]() {
                    return ext::make_shared<BinomialVanillaEngine<ExtendedCoxRossRubinstein>>(
                        process, compareSteps);
                },
                5);

            std::cout << std::left << std::setw(8) << compareSteps
                      << std::setw(18) << (euroTimeDep.first - euroConst.first)
                      << (amerTimeDep.first - amerConst.first) << "\n";
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
