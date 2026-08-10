//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Definite integral approximation over a declared domain — derived abscissae, ordered accumulation, no transcribed set.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/QuadratureIntegrator/Api
%layer      SlateMath
%sources    1
%symbols    9
%annotated  8/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S QuadratureIntegrator.h | 136 lines | b192bb38 | 9 sym | Definite integral approximation over a declared domain — derived abscissae, ordered accumulation, no transcribed set.

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE RULE
//------------------------------------------------------------------------------------------------------------------------

T QuadratureRule                    | QuadratureIntegrator.h | 36-130 | owning                        | -  | A Gauss–Legendre rule over the reference interval, derived once and reused. somebody typed, and the same three defects `ColourProjection` met apply here: a sign, a place, and a normalisation, each individually plausible. Newton on the Legendre recurrence has one place to be wrong and converges to the last representable bit from the standard initial estimate. `28`'s three surface builds pay a root-finding solve at every one of a million cells, and would make the abscissae a cost rather than a constant. agreement with the true integral: given the abscissa magnitudes the weighted sum is accumulated in ordinal order and is therefore the same number on every machine and every run — `02` §5's ordered recombination. How well the rule approximates the integrand is fixed by the abscissa count the caller declares, and is the caller's declaration rather than this component's promise.
    has   AbscissaCeiling    static constexpr std::uint32_t  [-]  ?
    has   DeclaredAbscissae  std::vector<double>             [-]  ?
    has   DeclaredWeights    std::vector<double>             [-]  ?
    has   RuleDerived        bool                            [-]  ?
    by    Api/AtmosphereIntegrator.h, Api/SpectralProjection.h, Source/AtmosphereIntegrator.cpp, Source/ConsoleHost.cpp, Source/QuadratureIntegrator.cpp, Source/SpectralProjection.cpp
    note  🔴 The abscissae are **derived**, never transcribed. A transcribed set is accurate to the digits
    note  🔴 The rule is derived **once** and integrated against many times. Deriving it per integral would make
    note  📐 Declared Bounded, per `02` §5. The guarantee is over the **accumulation**, not over the rule's

F QuadratureRule::Derive            | QuadratureIntegrator.h | 54     | api,nonthrowing               | 🔴 | Derives the rule of a declared abscissa count. symmetric about the origin and the weights with them — solving both halves would be solving the same equation twice and would let the two halves disagree in their last bit.
    in    Requested  std::uint32_t  [-]  abscissae; a rule of n integrates a polynomial of degree 2n−1 exactly
    out   -          Outcome        [-]  refuses with ContentUnsupported for zero, and with ExtentExhausted above the ceiling
    post  the abscissae are in ascending order and the weights sum to the reference interval's width
    by    Api/AttachmentIndex.h, Api/CameraProjection.h, Api/ChartPartition.h, Api/IlluminantPopulation.h, Api/OcclusionScheduler.h, Api/VisibilityRaster.h, (+8 more)
    note  📐 Only half the roots are solved for. The Legendre polynomials are even or odd, so the roots are

F QuadratureRule::Abscissa          | QuadratureIntegrator.h | 60     | api,nonallocating,nonthrowing | ✔️ | One abscissa of the reference interval, in ascending order.
    in    Ordinal  std::uint32_t  [-]  ?
    out   -        double         [-]  ?
    pre   Ordinal is below DeclaredCount
    by    Source/QuadratureIntegrator.cpp

F QuadratureRule::Weight            | QuadratureIntegrator.h | 65     | api,nonallocating,nonthrowing | ✔️ | The weight that abscissa carries.
    in    Ordinal  std::uint32_t  [-]  ?
    out   -        double         [-]  ?
    by    Source/QuadratureIntegrator.cpp, Source/UnwrapSolver.cpp

F QuadratureRule::Project           | QuadratureIntegrator.h | 80     | api,nonthrowing               | ✔️ | Projects one abscissa onto a declared interval, weight included. before the rule is derived one walk, in ordinal order. `28` integrates three extinction components along one ray, and three separate scalar integrations would evaluate the same density profile three times.
    in    Ordinal    std::uint32_t  [-]  the abscissa
    in    Lower      double         [-]  the interval's lower bound
    in    Upper      double         [-]  its upper bound
    in    Position   double&        [-]  ?
    in    Weighting  double&        [-]  ?
    out   -          Position       [-]  where the abscissa lands
    out   -          Weighting      [-]  the weight scaled to the interval
    out   -          Outcome        [-]  refuses with ExtentExhausted outside the declared count, and with ContentUnsupported
    by    Api/ColourProjection.h, Api/SpectralProjection.h, Api/TransformProjection.h, Api/VisibilityRaster.h, Source/AtmosphereIntegrator.cpp, Source/CameraProjection.cpp, (+6 more)
    note  📝 Exposed so that a caller integrating several components at once accumulates them side by side in

F QuadratureRule::IntegrateInterval | QuadratureIntegrator.h | 98-113 | api,nonthrowing               | 🚩 | Integrates one scalar integrand over a declared interval. artist as an atmosphere whose horizon shifts between two runs of an unchanged document. what the definite integral means and what a caller reversing a ray direction relies on.
    in    Lower     double                                                                                [-]  the interval's lower bound
    in    Upper     double                                                                                [-]  its upper bound
    in    Evaluate  Integrand                                                                             [-]  the integrand; called once per abscissa, in ordinal order
    in    arrival   order is a different number each run at Bounded, and here the difference reaches the  [-]  ?
    out   -         Integral                                                                              [-]  zero for a degenerate interval, and zero before the rule is derived
    by    Source/SpectralProjection.cpp
    note  🔴 Accumulated in ordinal order, never in whatever order a compiler finds convenient. `02` §5: a sum
    note  ⚠️ An inverted interval integrates to the negation of the forward one rather than refusing, which is

F QuadratureRule::DeclaredCount     | QuadratureIntegrator.h | 118    | api,nonallocating,nonthrowing | ✔️ | How many abscissae the rule carries.
    out   -  std::uint32_t  [-]  ?
    by    Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/MaterialSpecification.h, Api/ProgramIndex.h, (+19 more)

F QuadratureRule::Derived           | QuadratureIntegrator.h | 123    | api,nonallocating,nonthrowing | ✔️ | Whether the rule has been derived at all.
    out   -  bool  [-]  ?
    by    Api/SeamSpecification.h, Api/SpectralProjection.h, Source/AtmosphereIntegrator.cpp, Source/CameraProjection.cpp, Source/ChartPartition.cpp, Source/ColourProjection.cpp, (+11 more)

F SLATE_DECLARES_PRECISION          | QuadratureIntegrator.h | 134    | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChartPartition.h, (+24 more)
