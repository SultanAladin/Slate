//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Decomposed transforms, their composition, and the rebasing that precedes every narrowing to 32-bit.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/TransformProjection/Api
%layer      SlateMath
%sources    1
%symbols    18
%annotated  12/18
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TransformProjection.h | 171 lines | 4727d8e2 | 18 sym | Decomposed transforms, their composition, and the rebasing that precedes every narrowing to 32-bit.

//------------------------------------------------------------------------------------------------------------------------
//                                                POSITIONS AND ROTATIONS
//------------------------------------------------------------------------------------------------------------------------

T DocumentPosition         | TransformProjection.h | 21-26 | nonallocating,nonthrowing     | -  | A position in document space, at 64-bit. The document's own origin is what it is measured from.
    has   PositionX  double  [-]  ?
    has   PositionY  double  [-]  ?
    has   PositionZ  double  [-]  ?
    by    Api/AssetInterchange.h, Api/CameraProjection.h, Api/IlluminantPopulation.h, Api/PartitionClassifier.h, Api/PointerIntersection.h, Api/PrimitiveStructure.h, (+29 more)
    note  Scene extents exceed 32-bit relative precision, which is why document space is never 32-bit.

T ViewPosition             | TransformProjection.h | 37-42 | nonallocating,nonthrowing     | -  | A position in view-relative space — `02` §3's second space, at 64-bit and measured from the view origin. the whole of what it buys. The two carry identical members and mean different things, so one structure serving both makes a view-relative position passable wherever a document position is expected — which is `00` §10 conflict 15's defect exactly, and it survived the whole series the first time. narrowing are two decisions and a caller that wants the first without the second is `46` at every projection it derives. Fusing them is what makes the intermediate unnameable.
    has   PositionX  double  [-]  ?
    has   PositionY  double  [-]  ?
    has   PositionZ  double  [-]  ?
    by    Source/TransformProjection.cpp
    note  🔴 A **distinct** structure rather than a second reading of `DocumentPosition`, and the distinctness is
    note  📐 Still 64-bit. The narrowing is a separate act with its own routine, because the subtraction and the

T DevicePosition           | TransformProjection.h | 48-53 | nonallocating,nonthrowing     | -  | A position after rebasing, narrowed for the device. jitter with a plausible-looking cause.
    has   PositionX  float  [-]  ?
    has   PositionY  float  [-]  ?
    has   PositionZ  float  [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/TransformProjection.cpp, Source/VisibilityRaster.cpp
    note  🔴 Only ever produced by Narrow or Rebase. A 32-bit position that did not pass through one of them is

T RotationQuaternion       | TransformProjection.h | 59-65 | nonallocating,nonthrowing     | -  | A rotation as a unit quaternion. containment relation in `12` compound to unbounded depth without drift.
    has   ImaginaryX  double  [-]  ?
    has   ImaginaryY  double  [-]  ?
    has   ImaginaryZ  double  [-]  ?
    has   Real        double  [-]  ?
    by    Api/SpatialManipulator.h, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/DecalProjection.cpp, Source/IlluminantPopulation.cpp, Source/OcclusionProjection.cpp, (+4 more)
    note  📐 Compounding quaternions renormalises and multiplying matrices does not, which is what lets the

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TRANSFORM
//------------------------------------------------------------------------------------------------------------------------

T DecomposedTransform      | TransformProjection.h | 75-82 | nonallocating,nonthrowing     | -  | A transform stored decomposed — never as a general matrix. caches it, because a cached matrix is a second representation that drifts from the first.
    has   Translation  DocumentPosition    [-]  ?
    has   Rotation     RotationQuaternion  [-]  ?
    has   ScaleX       double              [-]  ?
    has   ScaleY       double              [-]  ?
    has   ScaleZ       double              [-]  ?
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/IlluminantPopulation.h, Api/SceneStructure.h, Api/SpatialManipulator.h, Api/SpatialSubdivision.h, (+6 more)
    note  Matrix form is derived at the point of use and never stored back. `Project` derives it; nothing

T ProjectedTransform       | TransformProjection.h | 86-92 | nonallocating,nonthrowing     | -  | A transform as sixteen coefficients, column-major, derived for one use and discarded.
    has   Coefficient  double[16]  [-]  ?
    by    Api/CameraProjection.h, Api/PartitionClassifier.h, Api/VisibilityRaster.h, Source/CameraProjection.cpp, Source/PartitionClassifier.cpp, Source/TransformProjection.cpp, (+1 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                      COMPOUNDING
//------------------------------------------------------------------------------------------------------------------------

F Compound                 | TransformProjection.h | 104   | api,nonallocating,nonthrowing | ✔️ | Compounds two rotations and renormalises the result.
    in    OuterRotation  RotationQuaternion  [-]  applied second
    in    InnerRotation  RotationQuaternion  [-]  applied first
    out   -              Compounded          [-]  a unit quaternion
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/SceneStructure.cpp, Source/TransformProjection.cpp

F SLATE_DECLARES_PRECISION | TransformProjection.h | 105   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F Compound                 | TransformProjection.h | 113   | api,nonallocating,nonthrowing | ✔️ | Compounds two decomposed transforms without ever forming a matrix.
    in    OuterTransform  const DecomposedTransform&  [-]  applied second — the containing transform
    in    InnerTransform  const DecomposedTransform&  [-]  applied first — the contained transform
    out   -               Compounded                  [-]  decomposed, ready to compound again
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/SceneStructure.cpp, Source/TransformProjection.cpp

F SLATE_DECLARES_PRECISION | TransformProjection.h | 115   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F Project                  | TransformProjection.h | 122   | api,nonallocating,nonthrowing | ✔️ | Derives the matrix form of a decomposed transform at the point of use.
    in    Source  const DecomposedTransform&  [-]  the decomposed transform
    out   -       Projected                   [-]  column-major coefficients; never stored back into Source
    by    Api/ColourProjection.h, Api/DisplayProjection.h, Api/QuadratureIntegrator.h, Api/SpectralProjection.h, Api/TickSequence.h, Api/VisibilityRaster.h, (+12 more)

F SLATE_DECLARES_PRECISION | TransformProjection.h | 123   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                        REBASING
//------------------------------------------------------------------------------------------------------------------------

F Relative                 | TransformProjection.h | 140   | api,nonallocating,nonthrowing | ✔️ | Carries a document position into view-relative space. The subtraction, and nothing else. quantities is exact to within one unit in the last place of the **larger operand's** exponent, so a position ten metres from a camera a kilometre from the document origin retains micrometre resolution here. Narrowing that difference afterwards costs the 32-bit relative precision of ten metres, which is far below one micrometre; narrowing before the subtraction costs the relative precision of a kilometre.
    in    Subject     DocumentPosition  [mm]  a position in document space
    in    ViewOrigin  DocumentPosition  [mm]  the current camera position, in document space
    out   -           Relative          [mm]  the same position, measured from the view origin, still at 64-bit
    by    Source/TransformProjection.cpp, Source/VectorCodec.cpp
    note  📐 This is the half of `02` §3.2 that carries the precision claim. The difference of two 64-bit

F SLATE_DECLARES_PRECISION | TransformProjection.h | 141   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F Narrow                   | TransformProjection.h | 152   | api,nonallocating,nonthrowing | ✔️ | Narrows a view-relative position for the device. The narrowing, and nothing else. position by mistake. `02` §8's gate — *every position narrowing to 32-bit is rebased in 64-bit first* — is discharged by this signature rather than by a review, because the only way to obtain the argument is to have called `Relative`.
    in    Subject  ViewPosition  [mm]  a position already measured from the view origin
    out   -        Narrowed      [mm]  the same position at 32-bit
    by    Api/OutlinerSequence.h, Api/TrigramIndex.h, Source/ClipboardExchange.cpp, Source/CodeInterchange.cpp, Source/FileInterchange.cpp, Source/OutlinerPanel.cpp, (+5 more)
    note  🔴 Takes a `ViewPosition` and not a `DocumentPosition`, so a caller cannot narrow an unrebased

F SLATE_DECLARES_PRECISION | TransformProjection.h | 153   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F Rebase                   | TransformProjection.h | 168   | api,nonallocating,nonthrowing | ✔️ | Rebases a document position to the view origin and narrows it for the device — both halves, at once. `SlateCompute` passes through here; `02` §8 gates it and the failure it prevents reads as a driver defect rather than as the arithmetic it is. as a third arithmetic. It is kept because the fused act is what most callers want and because every caller in the engine already spells it this way; the two halves exist for the callers — `46` deriving a projection, `78` measuring a grip — that want the difference before it is narrowed.
    in    Subject     DocumentPosition  [mm]  a position in document space
    in    ViewOrigin  DocumentPosition  [mm]  the current camera position, in document space
    out   -           Rebased           [mm]  relative to the view origin, at 32-bit
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/TransformProjection.cpp, Source/VisibilityRaster.cpp
    note  🔴 The subtraction happens in 64-bit, before the narrowing. Every position crossing into
    note  📝 Exactly `Narrow(Relative(Subject, ViewOrigin))` and is implemented as that composition rather than

F SLATE_DECLARES_PRECISION | TransformProjection.h | 169   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)
