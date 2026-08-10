//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Ordering by position alone, amendments bounded by what they touched, and the one resampling that is reported.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/SurfaceLayerSequence/Source
%layer      SlateDocument
%sources    1
%symbols    24
%annotated  0/24
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SurfaceLayerSequence.cpp | 503 lines | c7cb4b01 | 24 sym | Ordering by position alone, amendments bounded by what they touched, and the one resampling that is reported.

//------------------------------------------------------------------------------------------------------------------------
//                                                        LOCATION
//------------------------------------------------------------------------------------------------------------------------

F SurfaceLayerSequence::Located             | SurfaceLayerSequence.cpp | 15-27   | - | - | ?
    in    Subject  LayerIdentity  [-]  ?
    out   -        std::size_t    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F SurfaceLayerSequence::Append              | SurfaceLayerSequence.cpp | 33-73   | - | - | ?
    in    Declaring  const LayerSpecification&  [-]  ?
    out   -          Outcome<LayerIdentity>     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       AMENDMENTS
//------------------------------------------------------------------------------------------------------------------------

F SurfaceLayerSequence::Reorder             | SurfaceLayerSequence.cpp | 79-97   | - | - | ?
    in    Subject   LayerIdentity           [-]  ?
    in    Position  std::uint32_t           [-]  ?
    out   -         Outcome<std::uint32_t>  [-]  ?

F SurfaceLayerSequence::DeclarePresence     | SurfaceLayerSequence.cpp | 99-111  | - | - | ?
    in    Subject          LayerIdentity  [-]  ?
    in    PresenceEnabled  bool           [-]  ?
    out   -                Outcome<bool>  [-]  ?

F SurfaceLayerSequence::DeclareCombination  | SurfaceLayerSequence.cpp | 113-135 | - | - | ?
    in    Subject    LayerIdentity                  [-]  ?
    in    Declaring  CombineSpecification           [-]  ?
    out   -          Outcome<CombineSpecification>  [-]  ?

F SurfaceLayerSequence::Withdraw            | SurfaceLayerSequence.cpp | 137-164 | - | - | ?
    in    Subject  LayerIdentity                [-]  ?
    out   -        Outcome<LayerSpecification>  [-]  ?

F SurfaceLayerSequence::Nest                | SurfaceLayerSequence.cpp | 166-180 | - | - | ?
    out   -  Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESAMPLING
//------------------------------------------------------------------------------------------------------------------------

F SampleBilinear                            | SurfaceLayerSequence.cpp | 192-228 | - | - | ?
    in    Held            const PaintedContent&  [-]  ?
    in    PositionAlong   double                 [-]  ?
    in    PositionAcross  double                 [-]  ?
    in    Component       std::uint32_t          [-]  ?
    out   -               float                  [-]  ?

F ResampleContent                           | SurfaceLayerSequence.cpp | 230-265 | - | - | ?
    in    Held       PaintedContent&                                               [-]  ?
    in    Remapping  const std::function<bool(double, double, double&, double&)>&  [-]  ?
    out   -          void                                                          [-]  ?

F SurfaceLayerSequence::Resample            | SurfaceLayerSequence.cpp | 269-327 | - | - | ?
    in    ArrivingRevision  std::uint64_t                                                 [-]  ?
    in    Remapping         const std::function<bool(double, double, double&, double&)>&  [-]  ?
    in    Reporting         ReportSequence&                                               [-]  ?
    in    Sampled           TickPoint                                                     [-]  ?
    out   -                 Outcome<bool>                                                 [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

F SurfaceLayerSequence::Resolve             | SurfaceLayerSequence.cpp | 333-344 | - | - | ?
    in    Subject  LayerIdentity                       [-]  ?
    out   -        Outcome<const LayerSpecification*>  [-]  ?

F SurfaceLayerSequence::AmendPainted        | SurfaceLayerSequence.cpp | 346-366 | - | - | ?
    in    Subject  LayerIdentity             [-]  ?
    out   -        Outcome<PaintedContent*>  [-]  ?

F SurfaceLayerSequence::Entries             | SurfaceLayerSequence.cpp | 368-371 | - | - | ?
    out   -  const std::vector<LayerSpecification>&  [-]  ?

F SurfaceLayerSequence::Nested              | SurfaceLayerSequence.cpp | 373-382 | - | - | ?
    in    NestedOrdinal  std::uint32_t                         [-]  ?
    out   -              Outcome<const SurfaceLayerSequence*>  [-]  ?

F SurfaceLayerSequence::AmendNested         | SurfaceLayerSequence.cpp | 384-393 | - | - | ?
    in    NestedOrdinal  std::uint32_t                   [-]  ?
    out   -              Outcome<SurfaceLayerSequence*>  [-]  ?

F SurfaceLayerSequence::PositionOf          | SurfaceLayerSequence.cpp | 395-403 | - | - | ?
    in    Subject  LayerIdentity           [-]  ?
    out   -        Outcome<std::uint32_t>  [-]  ?

F SurfaceLayerSequence::WrittenChannels     | SurfaceLayerSequence.cpp | 405-422 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F SurfaceLayerSequence::AuthoredContentHeld | SurfaceLayerSequence.cpp | 424-445 | - | - | ?
    out   -  bool  [-]  ?

F SurfaceLayerSequence::NestingDepth        | SurfaceLayerSequence.cpp | 447     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F SurfaceLayerSequence::AddressedRevision   | SurfaceLayerSequence.cpp | 448     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F SurfaceLayerSequence::EntryCount          | SurfaceLayerSequence.cpp | 450-453 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F SurfaceLayerSequence::NestedCount         | SurfaceLayerSequence.cpp | 455-458 | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ENTRIES
//------------------------------------------------------------------------------------------------------------------------

F LayerIndex::Locate                        | SurfaceLayerSequence.cpp | 464-486 | - | - | ?
    in    Sequence  const SurfaceLayerSequence&         [-]  ?
    in    Subject   LayerIdentity                       [-]  ?
    out   -         Outcome<const LayerSpecification*>  [-]  ?

F LayerIndex::SpannedCount                  | SurfaceLayerSequence.cpp | 488-501 | - | - | ?
    in    Sequence  const SurfaceLayerSequence&  [-]  ?
    out   -         std::uint32_t                [-]  ?
