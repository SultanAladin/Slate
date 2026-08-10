//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Domain resampling from arrival stamps, the deferral that never coarsens, and the accumulation applied once.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/ImpressionSequence/Source
%layer      SlateCompute
%sources    1
%symbols    25
%annotated  0/25
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ImpressionSequence.cpp | 834 lines | 78f921ab | 25 sym | Domain resampling from arrival stamps, the deferral that never coarsens, and the accumulation applied once.

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE PAINTING LEVEL
//------------------------------------------------------------------------------------------------------------------------

F PaintingLevelOf                         | ImpressionSequence.cpp | 17-27   | - | - | ?
    in    WorkingExtent  std::uint32_t           [-]  ?
    out   -              Outcome<std::uint32_t>  [-]  ?
    by    Api/ImpressionSequence.h, Source/ConsoleHost.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE SHAPE PROFILE
//------------------------------------------------------------------------------------------------------------------------

V SpeedReference                          | ImpressionSequence.cpp | 39      | - | - | ?

V PathDistanceReference                   | ImpressionSequence.cpp | 40      | - | - | ?

V TiltReference                           | ImpressionSequence.cpp | 41      | - | - | ?

V RotationReference                       | ImpressionSequence.cpp | 42      | - | - | ?

F ProfileCoverage                         | ImpressionSequence.cpp | 47-67   | - | - | ?
    in    Declared          ProfileSubject  [-]  ?
    in    NormalisedRadius  double          [-]  ?
    out   -                 double          [-]  ?

F BoundedUnit                             | ImpressionSequence.cpp | 69-72   | - | - | ?
    in    Magnitude  double  [-]  ?
    out   -          double  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        OPENING
//------------------------------------------------------------------------------------------------------------------------

F ImpressionSequence::Open                | ImpressionSequence.cpp | 80-170  | - | - | ?
    in    Declaring  const StrokeDeclaration&   [-]  ?
    in    Brushed    const BrushSpecification&  [-]  ?
    out   -          Outcome<bool>              [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESAMPLING
//------------------------------------------------------------------------------------------------------------------------

F ImpressionSequence::ProjectAxes         | ImpressionSequence.cpp | 176-217 | - | - | ?
    in    Arriving       const PointerSample&  [-]  ?
    in    TangentAlong   double                [-]  ?
    in    TangentAcross  double                [-]  ?
    in    Speed          double                [-]  ?
    in    PathDistance   double                [-]  ?
    out   -              ResolvedAxes          [-]  ?

F ImpressionSequence::Emit                | ImpressionSequence.cpp | 219-254 | - | - | ?
    in    PositionAlong   double               [-]  ?
    in    PositionAcross  double               [-]  ?
    in    TangentAlong    double               [-]  ?
    in    TangentAcross   double               [-]  ?
    in    Axes            const ResolvedAxes&  [-]  ?
    in    PathDistance    double               [-]  ?
    out   -               void                 [-]  ?

F ImpressionSequence::Amend               | ImpressionSequence.cpp | 256-355 | - | - | ?
    in    Arriving  const StrokeArrival&  [-]  ?
    out   -         Outcome<bool>         [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 IMPRESSION RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F ImpressionSequence::ResolveOne          | ImpressionSequence.cpp | 361-508 | - | - | ?
    in    Impressing       ImpressionSample&  [-]  ?
    in    Residency        SurfaceTileSpace&  [-]  ?
    in    Requesting       RequestQueue&      [-]  ?
    in    RotationOrdinal  std::uint64_t      [-]  ?
    out   -                Outcome<bool>      [-]  ?

F ImpressionSequence::Resolve             | ImpressionSequence.cpp | 510-545 | - | - | ?
    in    Residency        SurfaceTileSpace&     [-]  ?
    in    Requesting       RequestQueue&         [-]  ?
    in    RotationOrdinal  std::uint64_t         [-]  ?
    out   -                Outcome<ResolvedRun>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  ABANDON AND RECLAIM
//------------------------------------------------------------------------------------------------------------------------

F ImpressionSequence::Abandon             | ImpressionSequence.cpp | 551-568 | - | - | ?
    in    Residency  SurfaceTileSpace&  [-]  ?
    out   -          void               [-]  ?

F ImpressionSequence::ReclaimSpeculative  | ImpressionSequence.cpp | 570-587 | - | - | ?
    out   -  Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        SEALING
//------------------------------------------------------------------------------------------------------------------------

F ImpressionSequence::Seal                | ImpressionSequence.cpp | 593-743 | - | - | ?
    in    Content    SurfaceLayerSequence&  [-]  ?
    in    Revised    RevisionSequence&      [-]  ?
    in    Residency  SurfaceTileSpace&      [-]  ?
    in    SealedAt   std::uint64_t          [-]  ?
    out   -          Outcome<SealedStroke>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE INVERSE
//------------------------------------------------------------------------------------------------------------------------

F Restore                                 | ImpressionSequence.cpp | 749-802 | - | - | ?
    in    Sealed   const SealedStroke&    [-]  ?
    in    Content  SurfaceLayerSequence&  [-]  ?
    out   -        Outcome<bool>          [-]  ?
    by    Api/ImpressionSequence.h, Source/ConsoleHost.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F ImpressionSequence::Impressions         | ImpressionSequence.cpp | 808     | - | - | ?
    out   -  const std::vector<ImpressionSample>&  [-]  ?

F ImpressionSequence::Accumulation        | ImpressionSequence.cpp | 809     | - | - | ?
    out   -  const StrokeSpace&  [-]  ?

F ImpressionSequence::ImpressionCount     | ImpressionSequence.cpp | 811-814 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F ImpressionSequence::PendingCount        | ImpressionSequence.cpp | 816-827 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F ImpressionSequence::PaintingLevel       | ImpressionSequence.cpp | 829     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F ImpressionSequence::PathLength          | ImpressionSequence.cpp | 830     | - | - | ?
    out   -  double  [-]  ?

F ImpressionSequence::StrokeOpen          | ImpressionSequence.cpp | 831     | - | - | ?
    out   -  bool  [-]  ?

F ImpressionSequence::SpeculativeDeclared | ImpressionSequence.cpp | 832     | - | - | ?
    out   -  bool  [-]  ?
