//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Trigram folding and entry, the rarest-run narrowing, and the exact confirmation over it.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/TrigramIndex/Source
%layer      SlateDocument
%sources    1
%symbols    8
%annotated  0/8
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TrigramIndex.cpp | 237 lines | 012d0e2d | 8 sym | Trigram folding and entry, the rarest-run narrowing, and the exact confirmation over it.

//------------------------------------------------------------------------------------------------------------------------
//                                                      FOLDED NAMES
//------------------------------------------------------------------------------------------------------------------------

F FoldedTrigrams             | TrigramIndex.cpp | 20-51   | - | - | ?
    in    Declared  const std::string&          [-]  ?
    out   -         std::vector<std::uint32_t>  [-]  ?

F NameContains               | TrigramIndex.cpp | 55-78   | - | - | ?
    in    Declared  const std::string&  [-]  ?
    in    Sought    const std::string&  [-]  ?
    out   -         bool                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F TrigramIndex::Enter        | TrigramIndex.cpp | 86-113  | - | - | ?
    in    SlotOrdinal  std::uint32_t       [-]  ?
    in    Declared     const std::string&  [-]  ?
    out   -            void                [-]  ?

F TrigramIndex::Declare      | TrigramIndex.cpp | 115-141 | - | - | ?
    in    Subject   OccupantIdentity    [-]  ?
    in    Declared  const std::string&  [-]  ?
    out   -         Deliver<bool>       [-]  ?

F TrigramIndex::Withdraw     | TrigramIndex.cpp | 143-170 | - | - | ?
    in    Subject  OccupantIdentity  [-]  ?
    out   -        void              [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE NARROWING
//------------------------------------------------------------------------------------------------------------------------

F TrigramIndex::Narrow       | TrigramIndex.cpp | 176-222 | - | - | ?
    in    Sought  const std::string&             [-]  ?
    out   -       std::vector<OccupantIdentity>  [-]  ?

F TrigramIndex::DeclaredName | TrigramIndex.cpp | 224-230 | - | - | ?
    in    Subject  OccupantIdentity    [-]  ?
    out   -        const std::string&  [-]  ?

F TrigramIndex::NamedCount   | TrigramIndex.cpp | 232-235 | - | - | ?
    out   -  std::uint32_t  [-]  ?
