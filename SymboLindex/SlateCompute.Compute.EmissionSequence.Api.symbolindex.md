//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `50` §5 — an export resolved from the domain at its declared extent, band by band, never read back from residency.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/EmissionSequence/Api
%layer      SlateCompute
%sources    1
%symbols    17
%annotated  12/17
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S EmissionSequence.h | 209 lines | 98bd0bdb | 17 sym | `50` §5 — an export resolved from the domain at its declared extent, band by band, never read back from residency.

//------------------------------------------------------------------------------------------------------------------------
//                                                WHAT BOUNDS AN EMISSION
//------------------------------------------------------------------------------------------------------------------------

V EmissionExtentCeiling            | EmissionSequence.h | 28      | -                             | -  | ?
    by    Source/EmissionSequence.cpp

V EmissionBandRows                 | EmissionSequence.h | 33      | -                             | -  | ?
    by    Source/EmissionSequence.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                 WHAT AN EMISSION MAKES
//------------------------------------------------------------------------------------------------------------------------

T EmittedTexels                    | EmissionSequence.h | 48-54   | owning                        | -  | One emitted image's texels, resolved and packed into the arrangement the specification declared. **not** a readback of `20`'s resident tiles. Residency is a display decision bounded by device memory, and an export bounded by what happened to be resident is an export whose content depends on where the artist last looked — which is a defect that reproduces only on the machine that made it. quantising here would quantise twice: once into this and once into the file, and the second one would be quantising an already-quantised value.
    has   Texels          std::vector<float>  [-]  ?
    has   ExtentTexels    std::uint32_t       [-]  ?
    has   ComponentCount  std::uint32_t       [-]  ?
    has   SpaceIdentity   std::uint32_t       [-]  ?
    by    Source/EmissionSequence.cpp
    note  🔴 `50` §5: these were resolved from `56`'s layers through `70`, at the emission's own extent. They are
    note  📝 Single precision, not the emitted depth. `50` §7 charges the quantisation to the codec at Tier B and

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHERE IT RESOLVES
//------------------------------------------------------------------------------------------------------------------------

T EmissionSources                  | EmissionSequence.h | 65-68   | nonallocating,nonthrowing     | -  | What an emission resolves through, borrowed and never owned. resolver: an export that resolved by a second implementation would ship an asset that disagrees with what the artist was shown while painting it, and they would have no way to tell which one was wrong.
    has   Resolution  const AnalyticProjection*  [-]  ?
    by    Source/EmissionSequence.cpp
    note  🔴 The same `70` a promotion reads and the same one `82` previews through. Three consumers, one

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE CHANNEL PLACEMENTS
//------------------------------------------------------------------------------------------------------------------------

F ProjectPlacements                | EmissionSequence.h | 90      | api,nonthrowing               | 🚩 | Derives the placements one emitted image's arrangement amounts to. densely — component zero of the resolution is the first channel the run names — and the walk scatters its 𝑘th component into the 𝑘th occupied slot. An unordered run would scatter into the wrong slots and produce an image that is plausible and wrong, which is `50` §5.1's whole warning. so nothing declares which components a channel occupies within a resolved texel. The scatter therefore lives in the band walk today. The day `00` §12 is answered and `70` places by the run, the scatter becomes the identity and is deleted; the run is handed over either way, so nothing else moves. entry that silently claimed the two components after it is an entry they cannot see the extent of.
    in    Arranged  const EmittedImage&                                                                          [-]  the image, with its occupied components declared
    in    the       arrangement, because `50` §5.1 requires the arrangement be *presented to the artist* and an  [-]  ?
    out   -         Placements                                                                                   [-]  one entry per occupied component, in ascending component order
    by    Source/EmissionSequence.cpp
    note  🔴 The entries are in **ascending component order**, and the band walk depends on it: `70` resolves
    note  🚧 `70` accepts the run and does not yet read it — `00` §12 carries the channel packing layout as open,
    note  🔴 Every placement spans one component. A colour channel occupying three is declared as three entries

F SLATE_DECLARES_PRECISION         | EmissionSequence.h | 91      | -                             | -  | ?
    in    Exact  PrecisionGuarantee::  [-]  ?
    in    Exact  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

F ToleranceAtExtent                | EmissionSequence.h | 102-105 | api,nonallocating,nonthrowing | ✔️ | The flattening tolerance an emission at one extent resolves at. emission's extent is the artist's declaration and need not be a reduction level at all, so the tolerance is derived from the extent rather than looked up — one texel of what is being written, which is the deviation below which a chord cannot move a sample.
    in    ExtentTexels  std::uint32_t  [px]  per edge of the emitted image
    out   -             Tolerance      [-]   one texel of the emitted extent, in domain units
    by    Source/EmissionSequence.cpp
    note  📐 The same rule `70`'s `ToleranceAtLevel` states, at an extent `20` does not have a level for. An

F SLATE_DECLARES_PRECISION         | EmissionSequence.h | 106     | -                             | -  | ?
    in    Exact  PrecisionGuarantee::  [-]  ?
    in    Exact  PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE EMISSION
//------------------------------------------------------------------------------------------------------------------------

T EmissionSequence                 | EmissionSequence.h | 123-200 | owning                        | -  | One emitted image resolved a band at a time, so a `Background` export never holds a worker for its length. so an export started before an edit contains the state at the moment it started. That is discharged by the caller handing in the sequence it sealed, not by anything here: a component that reached for the live sequence would resolve half its bands from before an edit and half from after, and the seam between them would land somewhere down the middle of the image. both built and are the caller's to drive — an export that half-overwrites last week's export has destroyed a deliverable to produce nothing, and that guarantee lives in the component that owns the replacement rather than in the one that produced the bytes.
    has   Producing        EmittedTexels                  [-]  ?
    has   Arrangement      std::vector<ChannelPlacement>  [-]  ?
    has   Resolution       const AnalyticProjection*      [-]  ?
    has   RowsNext         std::uint32_t                  [-]  ?
    has   EmissionOpen     bool                           [-]  ?
    has   SourcesDeclared  bool                           [-]  ?
    by    Source/EmissionSequence.cpp
    note  🔴 `50` §5: the document remains editable while this runs, and it reads **sealed** state per `48` §3 —
    note  🔴 Nothing here writes a file. `48` §3's write-verify-replace sequence and `04`'s `StorageExchange` are

F EmissionSequence::Construct      | EmissionSequence.h | 132     | api,nonallocating,nonthrowing | ✔️ | Takes the resolver every band reads.
    in    Supplied  const EmissionSources&  [-]  borrowed; outlives this component
    out   -         Deliver                 [-]  refuses with ContentUnsupported for an absent resolver
    by    Api/AnalyticProjection.h, Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CameraProjection.h, Api/CommandSequence.h, (+62 more)

F EmissionSequence::Open           | EmissionSequence.h | 148     | api,nonthrowing               | 🔴 | Opens one image of a validated emission, ready for its first band. ContentUnsupported outside the image count and above the extent ceiling, and with whatever the specification's own validation refused specification is handed in by value and the two calls are separated by however long the artist spent between declaring an export and starting it; validating once and trusting thereafter is trusting a copy, and `50` §5.1's wrong arrangement is exactly what that copy would carry.
    in    Declaring     const EmissionSpecification&  [-]  the emission specification; validated here, again, and not assumed
    in    Materials     const MaterialIndex&          [-]  the declared materials, so a channel no material declares is refused
    in    ImageOrdinal  std::uint32_t                 [-]  which of the specification's images this emission produces
    out   -             Deliver                       [-]  refuses with HostDenied before Construct and while an emission stands, with
    post  🔴 the texel run is allocated once, whole, so no band reallocates mid-emission
    by    Api/CameraProjection.h, Api/CommandSequence.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/HardwareMetrics.h, Api/ImpressionSequence.h, (+20 more)
    note  🔴 `Validate` is asked here even though `AssetInterchange::DeclareEmission` asked it already. The

F EmissionSequence::ResolveBand    | EmissionSequence.h | 166     | api,nonthrowing               | 🔴 | Resolves the next band of rows, and no more than that. resolved, and with whatever `70` refused at the first position it refused at `50` §2's rule for a partial intake is the same rule from the other direction: an image that is resolved above a seam and zero below it is an asset the artist ships without noticing, whereas an export that refused is one they cannot miss. sample places the first texel exactly on the domain boundary, where a seam's two sides are equally near and the resolution picks one arbitrarily.
    in    Content  const SurfaceLayerSequence&  [-]  the sealed layer sequence the emission reads
    out   -        Deliver                      [-]  refuses with HostDenied before Open, with ExtentExhausted once every row is
    post  the delivered count is rows resolved this call; zero is never delivered
    by    Source/EmissionSequence.cpp
    note  🔴 A refusal from `70` abandons the **whole** emission rather than leaving the band half-written.
    note  📝 Texels are sampled at their **centres** — (Row + ½)/Extent — and not at their corners. A corner

F EmissionSequence::ResolutionOwed | EmissionSequence.h | 171     | api,nonallocating,nonthrowing | ✔️ | Whether rows remain to be resolved.
    out   -  bool  [-]  ?
    by    Api/ImpressionSequence.h, Source/EmissionSequence.cpp, Source/ImpressionSequence.cpp, Source/ReflectanceIntegrator.cpp

F EmissionSequence::ResolvedRows   | EmissionSequence.h | 176     | api,nonallocating,nonthrowing | ✔️ | How many rows have been resolved, for whoever presents the export's progress.
    out   -  std::uint32_t  [-]  ?
    by    Source/EmissionSequence.cpp

F EmissionSequence::Seal           | EmissionSequence.h | 185     | api,nonthrowing               | 🚩 | Hands over the completed image and closes the emission. to a codec is a file that opens, looks approximately right, and is wrong along one edge.
    out   -  Deliver  [-]  refuses with HostDenied before Open and with ExtentExhausted while rows remain
    post  🔴 the emission is closed; the texels are moved out and this holds none
    by    Api/CameraProjection.h, Api/DecalProjection.h, Api/DocumentSession.h, Api/ImpressionSequence.h, Api/InterfaceExchange.h, Api/RevisionSequence.h, (+19 more)
    note  🔴 Refuses while rows remain rather than delivering what stands. A partially resolved image handed

F EmissionSequence::Reclaim        | EmissionSequence.h | 190     | api,nonthrowing               | 🚩 | Abandons the standing emission and reclaims its texels.
    out   -  void  [-]  ?
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, (+75 more)

F SLATE_DECLARES_PRECISION         | EmissionSequence.h | 205     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Exact    PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChannelPanel.h, (+50 more)
