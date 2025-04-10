
#include <ql/qldefines.hpp>
#ifdef BOOST_MSVC
#    include <ql/auto_link.hpp>
#endif
#include "constantblackscholesprocess.hpp"
#include "mc_discr_arith_av_strike.hpp"
#include "mclookbackengine.hpp"
#include "mceuropeanengine.hpp"
#include <ql/exercise.hpp>
#include <ql/instruments/asianoption.hpp>
#include <ql/instruments/europeanoption.hpp>
#include <ql/instruments/lookbackoption.hpp>
#include <ql/instruments/payoffs.hpp>
#include <ql/pricingengines/asian/mc_discr_arith_av_strike.hpp>
#include <ql/pricingengines/lookback/mclookbackengine.hpp>
#include <ql/pricingengines/vanilla/mceuropeanengine.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <chrono>
#include <iostream>

using namespace QuantLib;

int main() {

    try {

        // modify the sample code below to suit your project

        Calendar calendar = TARGET();
        Date today = Date(24, February, 2022);
        Settings::instance().evaluationDate() = today;

        Real underlying = 36;

        Handle<Quote> underlyingH(ext::make_shared<SimpleQuote>(underlying));

        DayCounter dayCounter = Actual365Fixed();
        Handle<YieldTermStructure> riskFreeRate(
            ext::make_shared<ZeroCurve>(std::vector<Date>{today, today + 6 * Months},
                                        std::vector<Rate>{0.01, 0.015},
                                        dayCounter));
        Handle<BlackVolTermStructure> volatility(ext::make_shared<BlackVarianceCurve>(
            today,
            std::vector<Date>{today + 3 * Months, today + 6 * Months},
            std::vector<Volatility>{0.20, 0.25},
            dayCounter));

        auto bsmProcess =
            ext::make_shared<BlackScholesProcess>(underlyingH, riskFreeRate, volatility);

        // options

        Real strike = 40;
        Date maturity(24, May, 2022);

        Option::Type type(Option::Put);
        auto exercise = ext::make_shared<EuropeanExercise>(maturity);
        auto payoff = ext::make_shared<PlainVanillaPayoff>(type, strike);

        EuropeanOption europeanOption(payoff, exercise);

        DiscreteAveragingAsianOption asianOption(Average::Arithmetic,
                                                 {Date(4, March, 2022),
                                                  Date(14, March, 2022),
                                                  Date(24, March, 2022),
                                                  Date(4, April, 2022),
                                                  Date(14, April, 2022),
                                                  Date(24, April, 2022),
                                                  Date(4, May, 2022),
                                                  Date(14, May, 2022),
                                                  Date(24, May, 2022)},
                                                 payoff,
                                                 exercise);

        ContinuousFixedLookbackOption lookbackOption(underlyingH->value(), payoff, exercise);

        // Monte Carlo pricing and timing

        // table heading

        Size width = 15;
        auto spacer = std::setw(width);
        std::cout << std::setw(40) << "old engine" << std::setw(30) << "non constant"
                  << std::setw(30) << "constant" << std::endl;
        std::cout << spacer << "kind" << spacer << "NPV" << spacer << "time [s]" << spacer << "NPV"
                  << spacer << "time [s]" << spacer << "NPV" << spacer << "time [s]" << std::endl;
        std::cout << std::string(5, ' ') << std::string(100, '-') << std::endl;

        Size timeSteps = 10;
        Size samples = 1000000;
        Size mcSeed = 42;

        // European, old engine

        europeanOption.setPricingEngine(MakeMCEuropeanEngine<PseudoRandom>(bsmProcess)
                                            .withSteps(timeSteps)
                                            .withSamples(samples)
                                            .withSeed(mcSeed));

        auto startTime = std::chrono::steady_clock::now();

        Real NPV = europeanOption.NPV();

        auto endTime = std::chrono::steady_clock::now();

        double us =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << "European" << spacer << NPV << spacer << us / 1000000;

        // European, new engine, non constant

        europeanOption.setPricingEngine(MakeMCEuropeanEngine_2<PseudoRandom>(bsmProcess)
                                            .withSteps(timeSteps)
                                            .withSamples(samples)
                                            .withSeed(mcSeed));

        startTime = std::chrono::steady_clock::now();

        NPV = europeanOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV << spacer << us / 1000000;

        // European, new engine, constant

        europeanOption.setPricingEngine(MakeMCEuropeanEngine_2<PseudoRandom>(bsmProcess)
                                            .withSteps(timeSteps)
                                            .withSamples(samples)
                                            .withSeed(mcSeed)
                                            .withConstantParameters());

        startTime = std::chrono::steady_clock::now();

        NPV = europeanOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV << spacer << us / 1000000 << std::endl;

        // Asian, old engine

        asianOption.setPricingEngine(MakeMCDiscreteArithmeticASEngine<PseudoRandom>(bsmProcess)
                                         .withSamples(samples)
                                         .withSeed(mcSeed));

        startTime = std::chrono::steady_clock::now();

        NPV = asianOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << "Asian" << spacer << NPV << spacer << us / 1000000;

        // Asian, non constant

        asianOption.setPricingEngine(MakeMCDiscreteArithmeticASEngine_2<PseudoRandom>(bsmProcess)
                                         .withSamples(samples)
                                         .withSeed(mcSeed));

        startTime = std::chrono::steady_clock::now();

        NPV = asianOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV << spacer << us / 1000000;

        // Asian, constant

        asianOption.setPricingEngine(MakeMCDiscreteArithmeticASEngine_2<PseudoRandom>(bsmProcess)
                                         .withSamples(samples)
                                         .withSeed(mcSeed)
                                         .withConstantParameters());

        startTime = std::chrono::steady_clock::now();

        NPV = asianOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV << spacer << us / 1000000 << std::endl;

        // Lookback, old engine

        lookbackOption.setPricingEngine(MakeMCLookbackEngine<ContinuousFixedLookbackOption, PseudoRandom>(bsmProcess)
                                           .withSteps(timeSteps)
                                           .withSamples(samples)
                                           .withSeed(mcSeed));

        startTime = std::chrono::steady_clock::now();

        NPV = lookbackOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << "Lookback" << spacer << NPV << spacer << us / 1000000;

        // Lookback, non constant

        lookbackOption.setPricingEngine(MakeMCFixedLookbackEngine_2<PseudoRandom>(bsmProcess)
                                           .withSteps(timeSteps)
                                           .withSamples(samples)
                                           .withSeed(mcSeed));

        startTime = std::chrono::steady_clock::now();

        NPV = lookbackOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV << spacer << us / 1000000;

        // Lookback, constant

        lookbackOption.setPricingEngine(MakeMCFixedLookbackEngine_2<PseudoRandom>(bsmProcess)
                                           .withSteps(timeSteps)
                                           .withSamples(samples)
                                           .withSeed(mcSeed)
                                           .withConstantParameters());

        startTime = std::chrono::steady_clock::now();

        NPV = lookbackOption.NPV();

        endTime = std::chrono::steady_clock::now();

        us = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

        std::cout << spacer << NPV << spacer << us / 1000000 << std::endl;
        std::cout << std::endl;

        // All done

        return 0;

    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "unknown error" << std::endl;
        return 1;
    }
}
