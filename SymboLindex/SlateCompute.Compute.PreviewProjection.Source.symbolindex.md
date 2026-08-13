//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `82` — the four previews resolved on the host, each one discarded and re-resolved every rotation.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/PreviewProjection/Source
%layer      SlateCompute
%sources    1
%symbols    17
%annotated  0/17
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S PreviewProjection.cpp | 267 lines | 40f7ed31 | 17 sym | `82` — the four previews resolved on the host, each one discarded and re-resolved every rotation.

//------------------------------------------------------------------------------------------------------------------------
//                                                WHAT A DRAG RESOLVES AT
//------------------------------------------------------------------------------------------------------------------------

V CoarseDragLevel                       | PreviewProjection.cpp | 21      | - | - | ?

V FinestLevel                           | PreviewProjection.cpp | 25      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IT READS
//------------------------------------------------------------------------------------------------------------------------

F PreviewProjection::Construct          | PreviewProjection.cpp | 33-48   | - | - | ?
    in    Supplied  const PreviewSources&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE BRUSH PREVIEW
//------------------------------------------------------------------------------------------------------------------------

F PreviewProjection::OpenImpression     | PreviewProjection.cpp | 54-77   | - | - | ?
    in    Declaring  const StrokeDeclaration&   [-]  ?
    in    Brushed    const BrushSpecification&  [-]  ?
    out   -          Outcome<bool>              [-]  ?

F PreviewProjection::AmendImpression    | PreviewProjection.cpp | 79-98   | - | - | ?
    in    Arriving  const StrokeArrival&  [-]  ?
    out   -         Outcome<bool>         [-]  ?

F PreviewProjection::ResolveImpression  | PreviewProjection.cpp | 100-115 | - | - | ?
    in    Residency        SurfaceTileSpace&     [-]  ?
    in    Requesting       RequestQueue&         [-]  ?
    in    RotationOrdinal  std::uint64_t         [-]  ?
    out   -                Outcome<ResolvedRun>  [-]  ?

F PreviewProjection::CloseImpression    | PreviewProjection.cpp | 117-121 | - | - | ?
    in    Residency  SurfaceTileSpace&  [-]  ?
    out   -          void               [-]  ?

F PreviewProjection::ImpressionCoverage | PreviewProjection.cpp | 123-126 | - | - | ?
    out   -  const StrokeSpace&  [-]  ?

F PreviewProjection::ImpressionStanding | PreviewProjection.cpp | 128-131 | - | - | ?
    out   -  bool  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE CONTENT PREVIEW
//------------------------------------------------------------------------------------------------------------------------

F PreviewProjection::ProjectContentAt   | PreviewProjection.cpp | 137-165 | - | - | ?
    in    Content         const SurfaceLayerSequence&           [-]  ?
    in    Placements      const std::vector<ChannelPlacement>&  [-]  ?
    in    PositionAlong   double                                [-]  ?
    in    PositionAcross  double                                [-]  ?
    in    Level           std::uint32_t                         [-]  ?
    in    ComponentCount  std::uint32_t                         [-]  ?
    out   -               Outcome<ResolvedSample>               [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE PLACEMENT PREVIEW
//------------------------------------------------------------------------------------------------------------------------

F PreviewProjection::ProjectPlacementAt | PreviewProjection.cpp | 171-184 | - | - | ?
    in    Content         const SurfaceLayerSequence&           [-]  ?
    in    Placements      const std::vector<ChannelPlacement>&  [-]  ?
    in    PositionAlong   double                                [-]  ?
    in    PositionAcross  double                                [-]  ?
    in    CoarseDeclared  bool                                  [-]  ?
    in    ComponentCount  std::uint32_t                         [-]  ?
    out   -               Outcome<ResolvedSample>               [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE PARAMETER PREVIEW
//------------------------------------------------------------------------------------------------------------------------

F PreviewProjection::AmendParameter     | PreviewProjection.cpp | 190-205 | - | - | ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                Outcome<bool>  [-]  ?

F PreviewProjection::AmendmentCount     | PreviewProjection.cpp | 207-210 | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE STANDING EXTENT
//------------------------------------------------------------------------------------------------------------------------

F PreviewProjection::DeclareExtent      | PreviewProjection.cpp | 216-246 | - | - | ?
    in    Previewed        SpeculativeSubject  [-]  ?
    in    SurfaceOrdinal   std::uint32_t       [-]  ?
    in    RequestedLevel   std::uint32_t       [-]  ?
    in    RotationOrdinal  std::uint64_t       [-]  ?
    out   -                Outcome<bool>       [-]  ?

F PreviewProjection::Standing           | PreviewProjection.cpp | 248-251 | - | - | ?
    out   -  const SpeculativeExtent&  [-]  ?

F PreviewProjection::ExtentCurrent      | PreviewProjection.cpp | 253-259 | - | - | ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    out   -                bool           [-]  ?

F PreviewProjection::ReclaimExtent      | PreviewProjection.cpp | 261-265 | - | - | ?
    out   -  void  [-]  ?
