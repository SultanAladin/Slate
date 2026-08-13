//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 One stream surface over three file systems — paths, whole streams, and the write-verify-replace sequence.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateMath/Platform/FileInterchange/Api
%layer      SlateMath
%sources    1
%symbols    9
%annotated  9/9
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S FileInterchange.h | 124 lines | 10119f91 | 9 sym | One stream surface over three file systems — paths, whole streams, and the write-verify-replace sequence.

//------------------------------------------------------------------------------------------------------------------------
//                                                   WHAT A PATH NAMES
//------------------------------------------------------------------------------------------------------------------------

E PathContent                       | FileInterchange.h | 26-32  | contract                  | -  | What the file system reports a path currently names. caller asked for and acts on. A refusal names a file system that declined to answer at all, which is a different fact and is reported through `Outcome` instead.
    has   Absent     PathContent  [-]  ?
    has   Stream     PathContent  [-]  ?
    has   Directory  PathContent  [-]  ?
    has   Foreign    PathContent  [-]  ?
    by    Source/FileInterchange.cpp
    note  🔴 Absent is one of the four rather than a refusal, because "no file is there" is an answer the

T PathReport                        | FileInterchange.h | 36-41  | nonallocating,nonthrowing | -  | What one path names, and the extent behind it.
    has   Content       PathContent    [-]  ?
    has   SpannedBytes  std::uint64_t  [-]  ?
    has   Revised       std::uint64_t  [-]  ?
    by    Source/FileInterchange.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE TRANSLATION
//------------------------------------------------------------------------------------------------------------------------

T FileInterchange                   | FileInterchange.h | 55-122 | owning                    | -  | The path and whole-stream surface, translated once over three file systems. many bytes it is — a component here that recognised a format would put format knowledge in `Layer0_Platform` and give every codec a second place to disagree with itself. A path narrowed at the call site is a path that is wrong for exactly the artists whose documents are not spelled in ASCII, and it is wrong invisibly until one of them opens a file.
    has   StreamCeiling  static constexpr std::uint64_t  [-]  ?
    by    Source/FileInterchange.cpp, Source/PersistenceSequence.cpp, Source/ShaderCodec.cpp
    note  🔴 Nothing here interprets content. `10`'s codecs know what a stream holds and this knows only how
    note  ⚠️ Paths cross this surface as UTF-8 and are widened inside it where the host requires wide text.

F FileInterchange::Resolve          | FileInterchange.h | 66     | api,nonthrowing           | ✔️ | What a path currently names. question "is this here" indistinguishable from a file system that failed to answer it.
    in    Path  const std::string&  [-]  UTF-8
    out   -     Outcome             [-]  refuses with HostDenied when the file system declined to answer
    by    Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/DocumentSession.h, (+94 more)
    note  📝 An absent path is delivered as Absent rather than refused. Refusing would make the ordinary

F FileInterchange::ReadStream       | FileInterchange.h | 76     | api,nonthrowing           | 🔴 | Reads a whole stream into an extent the caller then owns. ExtentExhausted when it spans more than the declared ceiling by range arrival reads through that one rather than through this.
    in    Path  const std::string&  [-]  UTF-8
    out   -     Outcome             [-]  refuses with HostDenied when the stream cannot be opened, and with
    by    Api/ShaderCodec.h, Source/FileInterchange.cpp, Source/ShaderCodec.cpp
    note  ⚠️ Whole-stream. `StorageExchange` is the surface for a stream read by range, and a codec driven

F FileInterchange::WriteStream      | FileInterchange.h | 93     | api,nonthrowing           | 🔴 | Writes a whole stream, verifies what landed, and only then replaces what was there. ExtentExhausted when what landed does not match what was written written beside the target, read back and compared, and only a verified stream replaces the original. Writing over the original directly means a host that dies mid-write has destroyed the artist's document to produce a partial one — and the moment it is most likely to die is a long write of a large document, which is exactly the document worth keeping. is not across two. The staged stream is written **beside the target** for that reason and not
    in    Path     const std::string&                                   [-]  UTF-8; what the caller wants to end up holding the content
    in    Content  const std::vector<std::uint8_t>&                     [-]  the bytes to write
    in    a        temporary directory that may sit on another volume.  [-]  ?
    out   -        Outcome                                              [-]  refuses with HostDenied when the file system declined, and with
    by    Source/FileInterchange.cpp, Source/PersistenceSequence.cpp
    note  🔴 `48` §3's sequence, and the reason this is one routine rather than three. The content is
    note  ⚠️ The replacement is the file system's own rename, which is atomic within one file system and

F FileInterchange::DeclareDirectory | FileInterchange.h | 102    | api,nonthrowing           | 🚩 | Creates a directory and every absent directory above it. be there, and it is.
    in    Path  const std::string&  [-]  UTF-8
    out   -     Outcome             [-]  refuses with HostDenied when the file system declined
    by    Source/FileInterchange.cpp
    note  📝 A directory that already exists is delivered rather than refused — the caller asked for it to

F FileInterchange::Reclaim          | FileInterchange.h | 108    | api,nonthrowing           | ✔️ | Removes what a path names, when it names a stream.
    in    Path  const std::string&  [-]  ?
    out   -     Outcome             [-]  refuses with HostDenied when the file system declined; delivers for an absent path
    by    Api/AttachmentIndex.h, Api/ByteSpace.h, Api/CodeInterchange.h, Api/CommandSequence.h, Api/CycleScheduler.h, Api/DepthReduction.h, (+75 more)

F FileInterchange::Append           | FileInterchange.h | 116    | api,nonthrowing           | ✔️ | Appends one path component to another, with exactly one separator between them.
    in    Leading   const std::string&  [-]  UTF-8; a trailing separator is neither required nor doubled
    in    Trailing  const std::string&  [-]  UTF-8
    out   -         Path                [-]  UTF-8
    by    Api/RecoverySequence.h, Api/ReportSequence.h, Api/SurfaceLayerSequence.h, Source/AssetInterchange.cpp, Source/BrushSpecification.cpp, Source/ChartPartition.cpp, (+12 more)
