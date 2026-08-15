//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Versioned document stream layout and its declared migrations — never a conditional inside a reader.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Format/FormatCodec/Api
%layer      SlateDocument
%sources    1
%symbols    4
%annotated  4/4
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S FormatCodec.h | 59 lines | 65ad81c3 | 4 sym | Versioned document stream layout and its declared migrations — never a conditional inside a reader.

//------------------------------------------------------------------------------------------------------------------------
//                                                     STREAM VERSION
//------------------------------------------------------------------------------------------------------------------------

V CurrentStreamVersion | FormatCodec.h | 20    | -                             | -  | The document stream layout this build writes.
    by    Api/DocumentSession.h, Api/PersistenceSequence.h, Source/ConsoleHost.cpp, Source/DocumentSession.cpp, Source/FormatCodec.cpp

T StreamHeading        | FormatCodec.h | 24-30 | nonallocating,nonthrowing     | -  | The leading declaration of every document stream.
    has   Signature       std::uint32_t  [-]  ?
    has   StreamVersion   std::uint32_t  [-]  ?
    has   OccupantCount   std::uint64_t  [-]  ?
    has   ContentOrdinal  std::uint64_t  [-]  ?
    by    Source/ConsoleHost.cpp, Source/FormatCodec.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       MIGRATION
//------------------------------------------------------------------------------------------------------------------------

T DeclaredMigration    | FormatCodec.h | 40-44 | nonallocating,nonthrowing     | -  | One declared transformation from a stream version to the one above it. reader. Conditionals inside readers are how a format acquires cases nobody can enumerate.
    has   FromVersion  std::uint32_t  [-]  ?
    has   ToVersion    std::uint32_t  [-]  ?
    by    Source/FormatCodec.cpp
    note  🔴 Migration is a declared transformation between two versions, never a conditional inside a

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CODEC
//------------------------------------------------------------------------------------------------------------------------

F ResolveMigration     | FormatCodec.h | 57    | api,nonallocating,nonthrowing | ✔️ | Reads a document stream's heading and reports whether a migration path reaches the current version. not decide whether the result is fit to use.
    in    Heading  const StreamHeading&  [-]  the heading as the stream carried it
    out   -        Deliver               [-]  refuses with VersionUnmigratable when no declared chain reaches CurrentStreamVersion
    by    Source/ConsoleHost.cpp, Source/FormatCodec.cpp
    note  A codec translates a stream and does nothing else. It does not condition what it decoded and does
