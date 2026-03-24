Results Analysis

The results are exactly as expected. First, we notice that the computation time has significantly decreased, dropping from 2.05 seconds to roughly 0.15 seconds. This was the principal aim of the project, making the computation time about 13 times faster.

As anticipated, there is a slight impact on the final price of the options. We notice minor differences between the standard QuantLib model and the constant model; for instance, the price of the European option goes from 4.170 to 4.166. This was expected because we approximated the dynamic yield and volatility curves as flat curves with fixed values. These differences remain within a hundredth or a thousandth for every type of option: we consider this an acceptable trade-off given the massive optimization in computation time. Moreover, we can conclude that the code architecture works, as there is zero difference in NPV between the old engine and the non constant mode.

Comparing the three engines, the impact of the approximation differs due to the nature of the options, specifically regarding their path-dependency:

For the European option: The only price that matters is the one at maturity (it is path-independent). Since we used the exact average rates for that specific horizon, the constant price is very close to the real price.

For the Asian option: The payoff depends on the average price throughout the life of the option. Using a constant average rate from the very first day ignores local fluctuations, which distorts the intermediate simulated prices and, in turn, automatically impacts the final average.

For the Lookback option: The payoff depends on the extreme (maximum or minimum) reached by the underlying asset over time, making it highly sensitive to local volatility. By replacing the actual volatility surface with a constant, "smoothed" average, we risk underestimating these local shocks and thus misjudging the peak reached by the trajectory.