//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Decomposed transforms, their composition, and the rebasing that precedes every narrowing to 32-bit.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Numeric/TransformProjection/Api
%layer      SlateMath
%sources    1
%symbols    13
%annotated  9/13
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TransformProjection.h | 125 lines | fd99169a | 13 sym | Decomposed transforms, their composition, and the rebasing that precedes every narrowing to 32-bit.

//------------------------------------------------------------------------------------------------------------------------
//                                                POSITIONS AND ROTATIONS
//------------------------------------------------------------------------------------------------------------------------

T DocumentPosition         | TransformProjection.h | 21-26 | nonallocating,nonthrowing     | -  | A position in document space or view-relative space, at 64-bit.
    has   PositionX  double  [-]  ?
    has   PositionY  double  [-]  ?
    has   PositionZ  double  [-]  ?
    by    Api/AssetInterchange.h, Api/CameraProjection.h, Api/IlluminantPopulation.h, Api/PartitionClassifier.h, Api/PointerIntersection.h, Api/SpatialSubdivision.h, (+19 more)
    note  Scene extents exceed 32-bit relative precision, which is why document space is never 32-bit.

T DevicePosition           | TransformProjection.h | 32-37 | nonallocating,nonthrowing     | -  | A position after rebasing, narrowed for the device. plausible-looking cause.
    has   PositionX  float  [-]  ?
    has   PositionY  float  [-]  ?
    has   PositionZ  float  [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/TransformProjection.cpp, Source/VisibilityRaster.cpp
    note  🔴 Only ever produced by Rebase. A 32-bit position that did not pass through it is jitter with a

T RotationQuaternion       | TransformProjection.h | 43-49 | nonallocating,nonthrowing     | -  | A rotation as a unit quaternion. containment relation in `12` compound to unbounded depth without drift.
    has   ImaginaryX  double  [-]  ?
    has   ImaginaryY  double  [-]  ?
    has   ImaginaryZ  double  [-]  ?
    has   Real        double  [-]  ?
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/DecalProjection.cpp, Source/IlluminantPopulation.cpp, Source/PointerIntersection.cpp, Source/SpatialSubdivision.cpp, (+1 more)
    note  📐 Compounding quaternions renormalises and multiplying matrices does not, which is what lets the

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE TRANSFORM
//------------------------------------------------------------------------------------------------------------------------

T DecomposedTransform      | TransformProjection.h | 59-66 | nonallocating,nonthrowing     | -  | A transform stored decomposed — never as a general matrix. caches it, because a cached matrix is a second representation that drifts from the first.
    has   Translation  DocumentPosition    [-]  ?
    has   Rotation     RotationQuaternion  [-]  ?
    has   ScaleX       double              [-]  ?
    has   ScaleY       double              [-]  ?
    has   ScaleZ       double              [-]  ?
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/IlluminantPopulation.h, Api/SceneStructure.h, Api/SpatialSubdivision.h, Source/CameraProjection.cpp, (+5 more)
    note  Matrix form is derived at the point of use and never stored back. `Project` derives it; nothing

T ProjectedTransform       | TransformProjection.h | 70-76 | nonallocating,nonthrowing     | -  | A transform as sixteen coefficients, column-major, derived for one use and discarded.
    has   Coefficient  double[16]  [-]  ?
    by    Api/CameraProjection.h, Api/PartitionClassifier.h, Api/VisibilityRaster.h, Source/CameraProjection.cpp, Source/PartitionClassifier.cpp, Source/TransformProjection.cpp, (+1 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                      COMPOUNDING
//------------------------------------------------------------------------------------------------------------------------

F Compound                 | TransformProjection.h | 88    | api,nonallocating,nonthrowing | ✔️ | Compounds two rotations and renormalises the result.
    in    OuterRotation  RotationQuaternion  [-]  applied second
    in    InnerRotation  RotationQuaternion  [-]  applied first
    out   -              Compounded          [-]  a unit quaternion
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/SceneStructure.cpp, Source/TransformProjection.cpp

F SLATE_DECLARES_PRECISION | TransformProjection.h | 89    | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChartPartition.h, (+24 more)

F Compound                 | TransformProjection.h | 97    | api,nonallocating,nonthrowing | ✔️ | Compounds two decomposed transforms without ever forming a matrix.
    in    OuterTransform  const DecomposedTransform&  [-]  applied second — the containing transform
    in    InnerTransform  const DecomposedTransform&  [-]  applied first — the contained transform
    out   -               Compounded                  [-]  decomposed, ready to compound again
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/SceneStructure.cpp, Source/TransformProjection.cpp

F SLATE_DECLARES_PRECISION | TransformProjection.h | 99    | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChartPartition.h, (+24 more)

F Project                  | TransformProjection.h | 106   | api,nonallocating,nonthrowing | ✔️ | Derives the matrix form of a decomposed transform at the point of use.
    in    Source  const DecomposedTransform&  [-]  the decomposed transform
    out   -       Projected                   [-]  column-major coefficients; never stored back into Source
    by    Api/ColourProjection.h, Api/QuadratureIntegrator.h, Api/SpectralProjection.h, Api/VisibilityRaster.h, Source/AtmosphereIntegrator.cpp, Source/CameraProjection.cpp, (+6 more)

F SLATE_DECLARES_PRECISION | TransformProjection.h | 107   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChartPartition.h, (+24 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                        REBASING
//------------------------------------------------------------------------------------------------------------------------

F Rebase                   | TransformProjection.h | 122   | api,nonallocating,nonthrowing | ✔️ | Rebases a document position to the view origin and narrows it for the device. `SlateCompute` passes through here; `02` §8 gates it and the failure it prevents reads as a driver defect rather than as the arithmetic it is.
    in    Subject     DocumentPosition  [mm]  a position in document space
    in    ViewOrigin  DocumentPosition  [mm]  the current camera position, in document space
    out   -           Rebased           [mm]  relative to the view origin, at 32-bit
    by    Source/CameraProjection.cpp, Source/ConsoleHost.cpp, Source/TransformProjection.cpp, Source/VisibilityRaster.cpp
    note  🔴 The subtraction happens in 64-bit, before the narrowing. Every position crossing into

F SLATE_DECLARES_PRECISION | TransformProjection.h | 123   | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChartPartition.h, (+24 more)
