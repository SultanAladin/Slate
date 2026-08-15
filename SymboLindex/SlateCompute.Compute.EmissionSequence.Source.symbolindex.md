//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 `50` §5 — the band walk that resolves an emitted image out of the domain, one texel centre at a time.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/EmissionSequence/Source
%layer      SlateCompute
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S EmissionSequence.cpp | 271 lines | 55a897c0 | 8 sym | `50` §5 — the band walk that resolves an emitted image out of the domain, one texel centre at a time.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ARRANGEMENT
//------------------------------------------------------------------------------------------------------------------------

F ProjectPlacements                | EmissionSequence.cpp | 22-44   | - | - | ?
    in    Arranged  const EmittedImage&            [-]  ?
    out   -         std::vector<ChannelPlacement>  [-]  ?
    by    Api/EmissionSequence.h

//------------------------------------------------------------------------------------------------------------------------
//                                                     WHAT IT READS
//------------------------------------------------------------------------------------------------------------------------

F EmissionSequence::Construct      | EmissionSequence.cpp | 50-65   | - | - | ?
    in    Supplied  const EmissionSources&  [-]  ?
    out   -         Deliver<bool>           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   OPENING ONE IMAGE
//------------------------------------------------------------------------------------------------------------------------

F EmissionSequence::Open           | EmissionSequence.cpp | 71-125  | - | - | ?
    in    Declaring     const EmissionSpecification&  [-]  ?
    in    Materials     const MaterialIndex&          [-]  ?
    in    ImageOrdinal  std::uint32_t                 [-]  ?
    out   -             Deliver<bool>                 [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    ONE BAND OF ROWS
//------------------------------------------------------------------------------------------------------------------------

F EmissionSequence::ResolveBand    | EmissionSequence.cpp | 131-216 | - | - | ?
    in    Content  const SurfaceLayerSequence&  [-]  ?
    out   -        Deliver<std::uint32_t>       [-]  ?

F EmissionSequence::ResolutionOwed | EmissionSequence.cpp | 218-221 | - | - | ?
    out   -  bool  [-]  ?

F EmissionSequence::ResolvedRows   | EmissionSequence.cpp | 223-226 | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    HANDING IT OVER
//------------------------------------------------------------------------------------------------------------------------

F EmissionSequence::Seal           | EmissionSequence.cpp | 232-253 | - | - | ?
    out   -  Deliver<EmittedTexels>  [-]  ?

F EmissionSequence::Reclaim        | EmissionSequence.cpp | 255-269 | - | - | ?
    out   -  void  [-]  ?
