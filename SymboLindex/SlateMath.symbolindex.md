//============================================================================================================================================
//                                                           SLATEMATH.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Symbol roll for SlateMath — A coordinate and the space it is a coordinate in — never a bare triple, and never an assumed encoding.

%format   symbolindex 1.0
%scope    layer
%path     Engine/SlateMath
%folders  22
%symbols  235

//------------------------------------------------------------------------------------------------------------------------
//                                                     FOLDER INDEXES
//------------------------------------------------------------------------------------------------------------------------

I Api    | Api/Api.symbolindex       | 23 sym | A coordinate and the space it is a coordinate in — never a bare triple, and never an assumed encoding.
I Source | Source/Source.symbolindex | 10 sym | Primaries derived from chromaticities, the transfers, von Kries adaptation, and the Planckian locus.
I Api    | Api/Api.symbolindex       | 7 sym  | Planar path evaluation, flattening to a tolerance, and stroke offsetting — the mechanism `52` resolves with.
I Source | Source/Source.symbolindex | 8 sym  | Adaptive subdivision, endpoint arc parameterisation, and bevelled offsetting.
I Api    | Api/Api.symbolindex       | 9 sym  | Definite integral approximation over a declared domain — derived abscissae, ordered accumulation, no transcribed set.
I Source | Source/Source.symbolindex | 6 sym  | Newton on the Legendre recurrence, solved over half the interval and mirrored onto the other.
I Api    | Api/Api.symbolindex       | 17 sym | The session's reports and its sampled measures — one appended once, one overwritten, and never confused.
I Source | Source/Source.symbolindex | 14 sym | Coalescing, the bounded cyclic retention, and the overwriting measure index beside it.
I Api    | Api/Api.symbolindex       | 8 sym  | Wavelength domain to tristimulus — the colour-matching functions, analytic, never three sampled wavelengths.
I Source | Source/Source.symbolindex | 3 sym  | The nine-lobe fit, and the normalisation derived from it rather than beside it.
I Api    | Api/Api.symbolindex       | 13 sym | Decomposed transforms, their composition, and the rebasing that precedes every narrowing to 32-bit.
I Source | Source/Source.symbolindex | 4 sym  | Quaternion composition, matrix derivation, and the 64-bit rebasing subtraction.
I Api    | Api/Api.symbolindex       | 6 sym  | Boundary-first parameterisation — Convergent, and held to reporting which criterion terminated it.
I Source | Source/Source.symbolindex | 7 sym  | Chord-length boundary mapping, mean-value interior weights, and relaxation against a declared criterion.
I Api    | Api/Api.symbolindex       | 39 sym | The only thread creation in the repository — declared work, immutable inputs, results applied on the tick.
I Source | Source/Source.symbolindex | 23 sym | The reserved interactive worker, cooperative cancellation, and conclusions ordered by declaration.
I Api    | Api/Api.symbolindex       | 7 sym  | Timestamped device samples crossing in, with absent axes distinguishable from zero-valued ones.
I Source | Source/Source.symbolindex | 4 sym  | Bounded cyclic arrival ordering over pointer samples.
I Api    | Api/Api.symbolindex       | 5 sym  | Monotonically increasing ordering points, stamped at arrival and never derived at consumption.
I Source | Source/Source.symbolindex | 5 sym  | Host timeline over the operating system's monotonic counter.
I Api    | Api/Api.symbolindex       | 8 sym  | One window surface over three window systems — surrenders the native handle and nothing else.
I Source | Source/Source.symbolindex | 9 sym  | Windowing over GLFW, linked dynamically through glfw3dll.lib against glfw3.dll.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

E TransferSubject                         | Api/ColourProjection.h          | 26-32   | The encoding transfer a colour space applies between its linear light and its stored code. `Linear` here carries no transfer, which is what lets a working space be wide and linear while a display space is neither.
T ColourSpaceSpecification                | Api/ColourProjection.h          | 45-62   | One colour space: three primaries, a white point, and the transfer it stores its code through. rather than declared as a matrix. A stored matrix is a second representation of the primaries and it drifts from them the moment either is amended. differ are never treated as one because their chromaticities happened to agree to six places.
F ColourSpaceSpecification::SpaceDeclared | Api/ColourProjection.h          | 61      | Whether this specification names a space at all.
V WorkingSpaceIdentity                    | Api/ColourProjection.h          | 66      | ?
V DisplaySpaceIdentity                    | Api/ColourProjection.h          | 67      | ?
F DeclaredWorkingSpace                    | Api/ColourProjection.h          | 74-89   | The wide linear working space a document is created with. wide-gamut set; `36` §9 leaves which set open and this is a constant, not a shape.
F DeclaredDisplaySpace                    | Api/ColourProjection.h          | 96-103  | The display space, companded, as a build default until `36` §9's open row is answered. produces an image that is correct on exactly one monitor.
T ColourSpecification                     | Api/ColourProjection.h          | 115-125 | A coordinate together with the space it is expressed in. three subsystems will each interpret differently, and all three will look plausible. and clamping here would compress a radiance before `66` had a chance to project it.
F ColourSpecification::ColourDeclared     | Api/ColourProjection.h          | 124     | Whether this colour names the space it is a coordinate in.
F SpacesAgree                             | Api/ColourProjection.h          | 131-134 | Whether two colours are expressed in the same space.
F SLATE_DECLARES_PRECISION                | Api/ColourProjection.h          | 135     | ?
F Project                                 | Api/ColourProjection.h          | 150     | Projects one colour into a declared space, transfers and white point included. white point, encode the target transfer. Exposing the four apart invites a caller to omit one, and the omission that matters — the transfer — produces an image that is merely "a bit washed out".
F SLATE_DECLARES_PRECISION                | Api/ColourProjection.h          | 153     | ?
F ProjectTristimulus                      | Api/ColourProjection.h          | 170     | Projects one tristimulus coordinate into a declared space, encoding its transfer. own; whether it needs adapting is a fact about the spectrum that produced it, which this routine cannot see. `ProjectTemperature` adapts before calling here, because a locus coordinate **is** a white point — and that is the one case where the adaptation is knowable at this depth. space without re-deriving the primaries. `AdaptWhite` already crosses this seam in tristimulus, so nothing new is exposed by it.
F SLATE_DECLARES_PRECISION                | Api/ColourProjection.h          | 174     | ?
F Encode                                  | Api/ColourProjection.h          | 180     | Applies one space's encoding transfer to a linear coordinate.
F SLATE_DECLARES_PRECISION                | Api/ColourProjection.h          | 181     | ?
F Decode                                  | Api/ColourProjection.h          | 186     | Removes one space's encoding transfer, returning linear light.
F SLATE_DECLARES_PRECISION                | Api/ColourProjection.h          | 187     | ?
F AdaptWhite                              | Api/ColourProjection.h          | 194     | Adapts a tristimulus coordinate from one white point to another. shifts hue on every saturated colour, which is visible exactly where an artist notices it.
F SLATE_DECLARES_PRECISION                | Api/ColourProjection.h          | 197     | ?
F ProjectTemperature                      | Api/ColourProjection.h          | 206     | Derives a white point coordinate from a declared correlated colour temperature. 5600 expects to see 5600 when they return, and a coordinate cannot be inverted back to it exactly.
F SLATE_DECLARES_PRECISION                | Api/ColourProjection.h          | 208     | ?
T TristimulusProjection                   | Source/ColourProjection.cpp     | 23-29   | ?
F Invert                                  | Source/ColourProjection.cpp     | 31-56   | ?
F Apply                                   | Source/ColourProjection.cpp     | 58-68   | ?
F DeriveProjection                        | Source/ColourProjection.cpp     | 77-127  | ?
F Encode                                  | Source/ColourProjection.cpp     | 135-153 | ?
F Decode                                  | Source/ColourProjection.cpp     | 155-170 | ?
F AdaptWhite                              | Source/ColourProjection.cpp     | 176-252 | ?
F Project                                 | Source/ColourProjection.cpp     | 258-318 | ?
F ProjectTristimulus                      | Source/ColourProjection.cpp     | 324-355 | ?
F ProjectTemperature                      | Source/ColourProjection.cpp     | 361-427 | ?
T PlanarPosition                          | Api/CurveSolver.h               | 25-29   | One position in a planar path's own space. predicate over narrowed positions is exact about the wrong positions.
E SegmentSubject                          | Api/CurveSolver.h               | 39-46   | What a path segment's geometry is, which fixes which control positions below are read. intake with its position named, never approximated by the nearest member.
T PathSegment                             | Api/CurveSolver.h               | 52-63   | One segment of a planar path, continuing from wherever the preceding segment ended. two segments that the fill rule would then have to guess how to close.
F Flatten                                 | Api/CurveSolver.h               | 82      | Appends the flattened polyline of one segment, excluding its origin and including its terminus. an outline at whatever reduction level a tile was promoted to, so a fixed tolerance is either wasteful at coarse levels or visibly polygonal at fine ones — `52` §4. more than the tolerance. Uniform subdivision at a fixed count is either wrong on a tight curve or wasteful on a slack one, and a path carries both.
F Flatten                                 | Api/CurveSolver.h               | 93      | Flattens a whole ordered run of segments into one polyline.
F OffsetOutline                           | Api/CurveSolver.h               | 114     | Converts a flattened polyline and a half-width into the closed outline of its stroke. ExtentExhausted for a polyline of fewer than two positions the source's own space and a placement scales that space, so a stored width thins when the placement shrinks — correct for a drawing program and wrong for content placed onto a surface at a chosen size. so declaring one here would be inventing an authored property the artist cannot see.
F SLATE_DECLARES_PRECISION                | Api/CurveSolver.h               | 117     | ?
V SubdivisionCeiling                      | Source/CurveSolver.cpp          | 25      | ?
F ChordDeviation                          | Source/CurveSolver.cpp          | 27-44   | ?
F Interpolate                             | Source/CurveSolver.cpp          | 46-53   | ?
F SubdivideCubic                          | Source/CurveSolver.cpp          | 55-84   | ?
F FlattenArc                              | Source/CurveSolver.cpp          | 86-193  | ?
F Flatten                                 | Source/CurveSolver.cpp          | 201-241 | ?
F Flatten                                 | Source/CurveSolver.cpp          | 243-260 | ?
F OffsetOutline                           | Source/CurveSolver.cpp          | 266-319 | ?
T QuadratureRule                          | Api/QuadratureIntegrator.h      | 36-130  | A Gauss–Legendre rule over the reference interval, derived once and reused. somebody typed, and the same three defects `ColourProjection` met apply here: a sign, a place, and a normalisation, each individually plausible. Newton on the Legendre recurrence has one place to be wrong and converges to the last representable bit from the standard initial estimate. `28`'s three surface builds pay a root-finding solve at every one of a million cells, and would make the abscissae a cost rather than a constant. agreement with the true integral: given the abscissa magnitudes the weighted sum is accumulated in ordinal order and is therefore the same number on every machine and every run — `02` §5's ordered recombination. How well the rule approximates the integrand is fixed by the abscissa count the caller declares, and is the caller's declaration rather than this component's promise.
F QuadratureRule::Derive                  | Api/QuadratureIntegrator.h      | 54      | Derives the rule of a declared abscissa count. symmetric about the origin and the weights with them — solving both halves would be solving the same equation twice and would let the two halves disagree in their last bit.
F QuadratureRule::Abscissa                | Api/QuadratureIntegrator.h      | 60      | One abscissa of the reference interval, in ascending order.
F QuadratureRule::Weight                  | Api/QuadratureIntegrator.h      | 65      | The weight that abscissa carries.
F QuadratureRule::Project                 | Api/QuadratureIntegrator.h      | 80      | Projects one abscissa onto a declared interval, weight included. before the rule is derived one walk, in ordinal order. `28` integrates three extinction components along one ray, and three separate scalar integrations would evaluate the same density profile three times.
F QuadratureRule::IntegrateInterval       | Api/QuadratureIntegrator.h      | 98-113  | Integrates one scalar integrand over a declared interval. artist as an atmosphere whose horizon shifts between two runs of an unchanged document. what the definite integral means and what a caller reversing a ray direction relies on.
F QuadratureRule::DeclaredCount           | Api/QuadratureIntegrator.h      | 118     | How many abscissae the rule carries.
F QuadratureRule::Derived                 | Api/QuadratureIntegrator.h      | 123     | Whether the rule has been derived at all.
F SLATE_DECLARES_PRECISION                | Api/QuadratureIntegrator.h      | 134     | ?
F QuadratureRule::Derive                  | Source/QuadratureIntegrator.cpp | 17-84   | ?
F QuadratureRule::Abscissa                | Source/QuadratureIntegrator.cpp | 90-93   | ?
F QuadratureRule::Weight                  | Source/QuadratureIntegrator.cpp | 95-98   | ?
F QuadratureRule::Project                 | Source/QuadratureIntegrator.cpp | 100-119 | ?
F QuadratureRule::DeclaredCount           | Source/QuadratureIntegrator.cpp | 121-124 | ?
F QuadratureRule::Derived                 | Source/QuadratureIntegrator.cpp | 126-129 | ?
E ReportDisposition                       | Api/ReportSequence.h            | 31-41   | What the engine did, on the artist's behalf, that the artist could not otherwise see. same reason — it names the category rather than the mechanism. Each member below is instead the past participle of what happened, which is the discriminating fact. disposition is a presentation that disagrees with the document that made the promise — `86` §4.1. and a presenter that treats all seven as failures teaches the artist to ignore it.
T ReportSpecification                     | Api/ReportSequence.h            | 53-62   | One appended report — where it came from, what it applies to, and how many times it has happened. and a report that owned an allocation would allocate on a worker while the tick presents the register. disposition then presents as the most serious thing it could be, which is the direction that gets fixed.
T ReportSequence                          | Api/ReportSequence.h            | 78-138  | The session's appended reports, bounded, coalesced, and readable from the tick. every origin that must write here sits beneath `SlateUI`, so a register held in the interface could not be written by a single one of the mechanisms obliged to write it. admits one. A report about a failure has to survive the failure, and `34` §5's failed work produces no result to carry it back on. leaves that component nothing to do; the coalescing rule it would have carried is `Coalesces` below.
F ReportSequence::Append                  | Api/ReportSequence.h            | 100     | Appends one report, coalescing it into a recurrence of the same origin, disposition and subject. reconstructed later from a measure that changed is a report about the wrong instant. integer comparisons per append rather than four thousand string comparisons.
F ReportSequence::Retained                | Api/ReportSequence.h            | 108     | The retained reports, oldest first, as a copy taken under the register's own guard. hand back storage a worker may be writing while the presenter walks it.
F ReportSequence::RetainedCount           | Api/ReportSequence.h            | 113     | How many reports are retained now.
F ReportSequence::AppendedCount           | Api/ReportSequence.h            | 118     | How many occurrences have been appended across the whole session, coalesced ones included.
F ReportSequence::DiscardedCount          | Api/ReportSequence.h            | 123     | How many retained reports the ceiling has discarded — itself a fact worth presenting.
F ReportSequence::Reclaim                 | Api/ReportSequence.h            | 128     | Empties the register. Called at process teardown and by nothing else.
T SampledMeasure                          | Api/ReportSequence.h            | 149-157 | One sampled quantity with a current value — overwritten every time it is sampled. its totals every rotation; appended, that is thousands of entries a minute inside which the one refusal that mattered is unfindable — `86` §2.
T MeasureIndex                            | Api/ReportSequence.h            | 164-209 | The current reading of every sampled measure, keyed by origin and quantity. never pushed — a producer that pushed its own measure would write from inside a recording, contending with the tick for the state the tick is presenting.
F MeasureIndex::DeclareCount              | Api/ReportSequence.h            | 175     | Declares one integer measure, replacing whatever the same origin and quantity last read.
F MeasureIndex::DeclareMagnitude          | Api/ReportSequence.h            | 182     | Declares one real measure, replacing whatever the same origin and quantity last read. the real declaration and quietly change a count into a magnitude.
F MeasureIndex::Measures                  | Api/ReportSequence.h            | 187     | Every measure currently held, in declaration order.
F MeasureIndex::Resolve                   | Api/ReportSequence.h            | 197     | One measure's current reading. capability: a metric that reports zero when it could not be measured is confidently wrong.
F MeasureIndex::Reclaim                   | Api/ReportSequence.h            | 202     | Discards every held measure.
F MeasureIndex::Located                   | Api/ReportSequence.h            | 206     | ?
F TextAgrees                              | Source/ReportSequence.cpp       | 22-31   | ?
F Coalesces                               | Source/ReportSequence.cpp       | 36-45   | ?
F ReportSequence::Append                  | Source/ReportSequence.cpp       | 53-94   | ?
F ReportSequence::Retained                | Source/ReportSequence.cpp       | 100-111 | ?
F ReportSequence::RetainedCount           | Source/ReportSequence.cpp       | 113-117 | ?
F ReportSequence::AppendedCount           | Source/ReportSequence.cpp       | 119-123 | ?
F ReportSequence::DiscardedCount          | Source/ReportSequence.cpp       | 125-129 | ?
F ReportSequence::Reclaim                 | Source/ReportSequence.cpp       | 131-140 | ?
F MeasureIndex::Located                   | Source/ReportSequence.cpp       | 146-158 | ?
F MeasureIndex::DeclareCount              | Source/ReportSequence.cpp       | 160-178 | ?
F MeasureIndex::DeclareMagnitude          | Source/ReportSequence.cpp       | 180-198 | ?
F MeasureIndex::Measures                  | Source/ReportSequence.cpp       | 200-203 | ?
F MeasureIndex::Resolve                   | Source/ReportSequence.cpp       | 205-216 | ?
F MeasureIndex::Reclaim                   | Source/ReportSequence.cpp       | 218-221 | ?
V SpectralLowerWavelength                 | Api/SpectralProjection.h        | 22      | ?
V SpectralUpperWavelength                 | Api/SpectralProjection.h        | 23      | ?
T TristimulusCoordinate                   | Api/SpectralProjection.h        | 35-40   | One tristimulus coordinate, before any space's primaries are applied. coordinate in, and tristimulus is not a space's coordinate — it is what a projection into one starts from. Spelling it as a colour would let a caller store it and have `36`'s rule appear satisfied by a coordinate in no space at all.
F ProjectWavelength                       | Api/SpectralProjection.h        | 58      | The three colour-matching responses at one wavelength. four hundred and seventy entries per response and would be four hundred and seventy chances to mistype; the fit is nine lobes and reproduces the set to within a fraction of a percent everywhere, which is far inside the Bounded guarantee this component claims. three-point quadrature of an integral whose integrand has a sharp lobe structure, and it is why an atmosphere computed that way has a twilight of the wrong hue rather than of the wrong brightness.
F SLATE_DECLARES_PRECISION                | Api/SpectralProjection.h        | 59      | ?
F LuminanceNormalisation                  | Api/SpectralProjection.h        | 69      | The integral of the luminance response over the declared interval — the normalisation a projection divides by. primaries from chromaticities: a stored normalisation is a second representation of the matching functions and drifts from them the moment the fit is amended.
F SLATE_DECLARES_PRECISION                | Api/SpectralProjection.h        | 70      | ?
F ProjectSpectrum                         | Api/SpectralProjection.h        | 91-139  | Projects one spectral quantity onto tristimulus, normalised so a flat spectrum of unit magnitude has unit luminance. normalisation vanishes — which is a fit that no longer describes a luminance response integrations would evaluate the caller's spectrum three times, and a spectrum that reads a medium profile is not cheap enough for that to be a matter of taste. not the same as exponentiating per wavelength and projecting the result. `28` does the former, because the latter needs a spectral transmittance surface rather than a tristimulus one; the discrepancy grows with optical depth and is visible only at grazing angles through the whole atmosphere. Declared here so that whoever measures it later finds the reason rather than the symptom.
F Lobe                                    | Source/SpectralProjection.cpp   | 23-33   | ?
F ProjectWavelength                       | Source/SpectralProjection.cpp   | 41-64   | ?
F LuminanceNormalisation                  | Source/SpectralProjection.cpp   | 70-83   | ?
T DocumentPosition                        | Api/TransformProjection.h       | 21-26   | A position in document space or view-relative space, at 64-bit.
T DevicePosition                          | Api/TransformProjection.h       | 32-37   | A position after rebasing, narrowed for the device. plausible-looking cause.
T RotationQuaternion                      | Api/TransformProjection.h       | 43-49   | A rotation as a unit quaternion. containment relation in `12` compound to unbounded depth without drift.
T DecomposedTransform                     | Api/TransformProjection.h       | 59-66   | A transform stored decomposed — never as a general matrix. caches it, because a cached matrix is a second representation that drifts from the first.
T ProjectedTransform                      | Api/TransformProjection.h       | 70-76   | A transform as sixteen coefficients, column-major, derived for one use and discarded.
F Compound                                | Api/TransformProjection.h       | 88      | Compounds two rotations and renormalises the result.
F SLATE_DECLARES_PRECISION                | Api/TransformProjection.h       | 89      | ?
F Compound                                | Api/TransformProjection.h       | 97      | Compounds two decomposed transforms without ever forming a matrix.
F SLATE_DECLARES_PRECISION                | Api/TransformProjection.h       | 99      | ?
F Project                                 | Api/TransformProjection.h       | 106     | Derives the matrix form of a decomposed transform at the point of use.
F SLATE_DECLARES_PRECISION                | Api/TransformProjection.h       | 107     | ?
F Rebase                                  | Api/TransformProjection.h       | 122     | Rebases a document position to the view origin and narrows it for the device. `SlateCompute` passes through here; `02` §8 gates it and the failure it prevents reads as a driver defect rather than as the arithmetic it is.
F SLATE_DECLARES_PRECISION                | Api/TransformProjection.h       | 123     | ?
F Compound                                | Source/TransformProjection.cpp  | 17-58   | ?
F Compound                                | Source/TransformProjection.cpp  | 64-98   | ?
F Project                                 | Source/TransformProjection.cpp  | 104-134 | ?
F Rebase                                  | Source/TransformProjection.cpp  | 140-154 | ?
T UnwrapSpecification                     | Api/UnwrapSolver.h              | 31-38   | One chart handed to the solver — its positions, its triangulation, and its boundary loop. property in object space, so a chart flattens identically wherever the occupant sits and an artist moving a scene re-derives nothing here. and cannot be flattened at all; the caller resolves that by subdividing, which is `68` §4.1's response to a fold and is deliberately the same mechanism.
T DistortionMeasure                       | Api/UnwrapSolver.h              | 51-58   | Area and angle distortion, measured apart from each other. and no single number expresses both: a surface flattened to preserve angles stretches in area, and an artist painting a repeating pattern cares about area while one placing a decal cares about angle. raw ratio would report the packing scale as though it were a defect of the flattening.
F Solve                                   | Api/UnwrapSolver.h              | 80      | Flattens one chart, boundary first, and reports which condition terminated it. an out-of-range corner, a boundary loop shorter than three, or a boundary of no extent cause, because a solver that returns its last iterate at the ceiling is indistinguishable from one that converged — and `68` §4's specific consequence is an artist painting on a domain whose distortion nobody measured. weighted average of its neighbours. Mean-value weights are strictly positive, so a convex boundary gives a fold-free embedding by construction rather than by inspection — which is the whole reason the boundary is mapped to a circle rather than to the chart's own silhouette. `68` §4.1 tests for them anyway. Construction narrows the failure; it does not remove it.
F SLATE_DECLARES_PRECISION                | Api/UnwrapSolver.h              | 81      | ?
F Measure                                 | Api/UnwrapSolver.h              | 92      | Measures the area and angle distortion of a flattening against the topology it came from. rather than removing them, so they arrive here and would otherwise report an unbounded ratio.
F SLATE_DECLARES_PRECISION                | Api/UnwrapSolver.h              | 95      | ?
F SpatialDistance                         | Source/UnwrapSolver.cpp         | 22-29   | ?
F CornerAngle                             | Source/UnwrapSolver.cpp         | 34-55   | ?
F PlanarAngle                             | Source/UnwrapSolver.cpp         | 57-75   | ?
F SpatialArea                             | Source/UnwrapSolver.cpp         | 77-92   | ?
F PlanarArea                              | Source/UnwrapSolver.cpp         | 94-100  | ?
F Solve                                   | Source/UnwrapSolver.cpp         | 108-311 | ?
F Measure                                 | Source/UnwrapSolver.cpp         | 317-395 | ?
V AbsentWork                              | Api/WorkSequence.h              | 27      | ?
E WorkPriority                            | Api/WorkSequence.h              | 37-43   | How urgently declared work is wanted, and therefore what it may starve. whole-document export are both long solves, and `34` §4 forbids the export occupying every worker.
E WorkConclusion                          | Api/WorkSequence.h              | 49-55   | How one declaration ended. superseded cancellation ordinary operation and a withdrawn one the requester's own decision.
T WorkCancellation                        | Api/WorkSequence.h              | 66-77   | What a resolution reads at each of its declared cancellation points. cancelled declaration still runs to its next declared point and releases what it holds — a worker that is simply never joined leaks its inputs, proportional to how often the artist changes their mind.
F WorkCancellation::WithdrawalDeclared    | Api/WorkSequence.h              | 73-76   | Whether the requester has withdrawn this declaration.
T WorkProgress                            | Api/WorkSequence.h              | 89-151  | What a long solve reports while it runs. own rate would contend with the tick for the very state the tick is presenting. and read once per tick, so the contention it could suffer never arises.
F WorkProgress::DeclareFraction           | Api/WorkSequence.h              | 101-105 | Declares the resolved fraction, clamped to the closed unit interval.
F WorkProgress::DeclareCount              | Api/WorkSequence.h              | 112-119 | Declares the resolved count out of the spanned count, and the fraction they imply.
F WorkProgress::Fraction                  | Api/WorkSequence.h              | 124     | The resolved fraction as last declared.
F WorkProgress::ResolvedCount             | Api/WorkSequence.h              | 129     | The resolved count as last declared.
F WorkProgress::SpannedCount              | Api/WorkSequence.h              | 134     | The spanned count as last declared; zero declares the span unknown.
F WorkProgress::Reclaim                   | Api/WorkSequence.h              | 139-144 | Returns every reading to its beginning, for a reused record.
T WorkDeclaration                         | Api/WorkSequence.h              | 166-174 | One declaration of work to be resolved off the tick. document, the tick's state, or anything in `76`. The requester captures what the work needs at declaration and hands it over, which is the rule that makes every lock here unnecessary. it on the tick after `Drain` delivers it — `34` §3. bounded worker count, waiting is a deadlock that appears only under load, on someone else's machine.
T WorkCompletion                          | Api/WorkSequence.h              | 178-186 | One concluded declaration, crossing back to the tick.
T WorkQueue                               | Api/WorkSequence.h              | 196-226 | Pending declarations at one priority level, claimed in declaration order. withdrawal costs a write instead of a shift of everything behind it.
F WorkQueue::Admit                        | Api/WorkSequence.h              | 203     | Admits one record ordinal at the end of the order.
F WorkQueue::Claim                        | Api/WorkSequence.h              | 209     | Claims the earliest pending record ordinal.
F WorkQueue::Withdraw                     | Api/WorkSequence.h              | 214     | Strikes one record ordinal from the order without claiming it.
F WorkQueue::PendingCount                 | Api/WorkSequence.h              | 219     | How many declarations are pending here.
T WorkSequence                            | Api/WorkSequence.h              | 239-365 | The workers, their lifetime, and the dispatch order over three priority levels. `50`, `68`, `20`, `24`, `70` and `82` is declared into this and applied by its requester on the tick. delivers completions ordered by declaration ordinal for exactly that reason — an application order that followed completion order would make the same inputs produce two documents on two machines.
F WorkSequence::WorkSequence              | Api/WorkSequence.h              | 245     | ?
F WorkSequence::~WorkSequence             | Api/WorkSequence.h              | 248     | ?
F WorkSequence::Construct                 | Api/WorkSequence.h              | 260     | Constructs the workers, once, at bring-up. the host report and the call moves there when it exists.
F WorkSequence::Declare                   | Api/WorkSequence.h              | 270     | Declares one unit of work, to be resolved by a worker. declaration carries no resolution claims it; nothing about the calling thread decides when.
F WorkSequence::Withdraw                  | Api/WorkSequence.h              | 278     | Withdraws one declaration, because the requester no longer wants it.
F WorkSequence::Supersede                 | Api/WorkSequence.h              | 286     | Withdraws one declaration because a newer one replaces it. superseded cancellation ordinary operation, so nothing is appended to the register for it.
F WorkSequence::Drain                     | Api/WorkSequence.h              | 302     | Delivers every conclusion recorded since the last drain, in declaration order. delivered as soon as it is recorded, so an earlier declaration still resolving does not hold a later one back. Holding it back would make a `Background` export block every `Interactive` promotion declared after it, which is the starvation `34` §4 forbids outright. index, never by completion. Two independent declarations read disjoint immutable inputs, so the order their results are applied in carries no information and cannot make two machines differ. a worker applying its own result would linearise against `RevisionSequence` from a thread that does not observe the tick's ordering, which `12` invariant 10 forbids.
F WorkSequence::Progress                  | Api/WorkSequence.h              | 308     | One declaration's resolved fraction.
F WorkSequence::ProgressCount             | Api/WorkSequence.h              | 314     | One declaration's resolved count.
F WorkSequence::WorkerCount               | Api/WorkSequence.h              | 319     | How many workers stand.
F WorkSequence::OccupiedWorkers           | Api/WorkSequence.h              | 324     | How many workers are resolving something now.
F WorkSequence::PendingCount              | Api/WorkSequence.h              | 329     | How many declarations are pending across every priority.
F WorkSequence::Reclaim                   | Api/WorkSequence.h              | 335     | Withdraws everything pending, joins every worker, and returns the sequence to its unconstructed state.
F WorkSequence::Serve                     | Api/WorkSequence.h              | 344     | ?
F WorkSequence::Claimable                 | Api/WorkSequence.h              | 345     | ?
F WorkSequence::Claim                     | Api/WorkSequence.h              | 346     | ?
F WorkSequence::Seal                      | Api/WorkSequence.h              | 347     | ?
F WorkSequence::Cancel                    | Api/WorkSequence.h              | 348     | ?
F WorkSequence::Resolved                  | Api/WorkSequence.h              | 349     | ?
T WorkSequence                            | Source/WorkSequence.cpp         | 19-29   | ?
F WorkQueue::Admit                        | Source/WorkSequence.cpp         | 35-39   | ?
F WorkQueue::Claim                        | Source/WorkSequence.cpp         | 41-68   | ?
F WorkQueue::Withdraw                     | Source/WorkSequence.cpp         | 70-84   | ?
F WorkQueue::PendingCount                 | Source/WorkSequence.cpp         | 86-89   | ?
F WorkSequence::Construct                 | Source/WorkSequence.cpp         | 100-135 | ?
F WorkSequence::~WorkSequence             | Source/WorkSequence.cpp         | 137-140 | ?
F WorkSequence::Claimable                 | Source/WorkSequence.cpp         | 146-163 | ?
F WorkSequence::Claim                     | Source/WorkSequence.cpp         | 165-181 | ?
F WorkSequence::Serve                     | Source/WorkSequence.cpp         | 183-229 | ?
F WorkSequence::Declare                   | Source/WorkSequence.cpp         | 235-286 | ?
F WorkSequence::Resolved                  | Source/WorkSequence.cpp         | 292-303 | ?
F WorkSequence::Cancel                    | Source/WorkSequence.cpp         | 305-329 | ?
F WorkSequence::Withdraw                  | Source/WorkSequence.cpp         | 331-334 | ?
F WorkSequence::Supersede                 | Source/WorkSequence.cpp         | 336-339 | ?
F WorkSequence::Seal                      | Source/WorkSequence.cpp         | 345-397 | ?
F WorkSequence::Drain                     | Source/WorkSequence.cpp         | 403-421 | ?
F WorkSequence::Progress                  | Source/WorkSequence.cpp         | 427-437 | ?
F WorkSequence::ProgressCount             | Source/WorkSequence.cpp         | 439-452 | ?
F WorkSequence::WorkerCount               | Source/WorkSequence.cpp         | 454-458 | ?
F WorkSequence::OccupiedWorkers           | Source/WorkSequence.cpp         | 460-464 | ?
F WorkSequence::PendingCount              | Source/WorkSequence.cpp         | 466-476 | ?
F WorkSequence::Reclaim                   | Source/WorkSequence.cpp         | 482-531 | ?
T AxisPresence                            | Api/InputExchange.h             | 23-28   | Which optional axes the reporting device supplied on one sample. are different facts, and `22` treats them differently.
T PointerSample                           | Api/InputExchange.h             | 38-49   | One pointer sample, stamped at arrival by `TickSequence`. rate reconstructs only if the arrival stamps survive.
T InputExchange                           | Api/InputExchange.h             | 60-94   | The bounded arrival ordering of pointer samples, drained once per tick by the consumer. that outruns the drain loses its oldest samples, which is visible, rather than allocating during an interaction, which is not.
F InputExchange::Record                   | Api/InputExchange.h             | 70      | Records one arriving sample against the supplied timeline.
F InputExchange::Sample                   | Api/InputExchange.h             | 77      | Reads one held sample in arrival order.
F InputExchange::HeldCount                | Api/InputExchange.h             | 82      | How many samples are held.
F InputExchange::Reclaim                  | Api/InputExchange.h             | 87      | Discards every held sample. Called by the consumer once it has read them.
F InputExchange::Record                   | Source/InputExchange.cpp        | 15-30   | ?
F InputExchange::Sample                   | Source/InputExchange.cpp        | 36-39   | ?
F InputExchange::HeldCount                | Source/InputExchange.cpp        | 41-44   | ?
F InputExchange::Reclaim                  | Source/InputExchange.cpp        | 46-50   | ?
T TickPoint                               | Api/TickSequence.h              | 21-24   | One ordering point on the monotonic host timeline. path the artist drew from arrival stamps, which is the only reason arrival stamps exist.
T TickSequence                            | Api/TickSequence.h              | 28-55   | The monotonic host timeline. One instance per process, constructed at bring-up.
F TickSequence::TickSequence              | Api/TickSequence.h              | 34      | Fixes the timeline origin against the host performance counter.
F TickSequence::Advance                   | Api/TickSequence.h              | 41      | Reads the current ordering point.
F TickSequence::Span                      | Api/TickSequence.h              | 49      | Duration between two ordering points.
K WIN32_LEAN_AND_MEAN                     | Source/TickSequence.cpp         | 14      | ?
K NOMINMAX                                | Source/TickSequence.cpp         | 17      | ?
F TickSequence::TickSequence              | Source/TickSequence.cpp         | 31-53   | ?
F TickSequence::Advance                   | Source/TickSequence.cpp         | 59-83   | ?
F TickSequence::Span                      | Source/TickSequence.cpp         | 89-95   | ?
T DisplayExtent                           | Api/WindowInterchange.h         | 23-27   | The extent of a window's drawable area. the intermediates; nothing here queues them.
T WindowInterchange                       | Api/WindowInterchange.h         | 38-81   | A native window over the host window system. is, includes no Vulkan header, and names no presentation chain. `06`'s `WindowExchange` converts the handle; the split is what keeps `SlateMath` device-free.
F WindowInterchange::~WindowInterchange   | Api/WindowInterchange.h         | 45      | ?
F WindowInterchange::Open                 | Api/WindowInterchange.h         | 53      | Opens a window of the requested extent and surrenders nothing until it succeeds.
F WindowInterchange::Drain                | Api/WindowInterchange.h         | 58      | Drains the window system's pending messages into this window's recorded condition.
F WindowInterchange::NativeHandle         | Api/WindowInterchange.h         | 64      | The opaque native handle, for `06`'s `WindowExchange` and for nothing else.
F WindowInterchange::CurrentExtent        | Api/WindowInterchange.h         | 69      | The current drawable extent.
F WindowInterchange::ClosureRequested     | Api/WindowInterchange.h         | 74      | Whether the artist has asked the window system to close this window.
V OpenWindowCount                         | Source/WindowInterchange.cpp    | 23      | ?
F AcquireWindowSystem                     | Source/WindowInterchange.cpp    | 25-32   | ?
F ReleaseWindowSystem                     | Source/WindowInterchange.cpp    | 34-43   | ?
F WindowInterchange::Open                 | Source/WindowInterchange.cpp    | 50-77   | ?
F WindowInterchange::~WindowInterchange   | Source/WindowInterchange.cpp    | 79-87   | ?
F WindowInterchange::Drain                | Source/WindowInterchange.cpp    | 93-109  | ?
F WindowInterchange::NativeHandle         | Source/WindowInterchange.cpp    | 111-114 | ?
F WindowInterchange::CurrentExtent        | Source/WindowInterchange.cpp    | 116-119 | ?
F WindowInterchange::ClosureRequested     | Source/WindowInterchange.cpp    | 121-124 | ?
