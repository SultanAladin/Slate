//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 A repeating pattern as plane symmetry plus cell content — declared, deterministic, and resolving nothing.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/TilingSpecification/Api
%layer      SlateDocument
%sources    1
%symbols    25
%annotated  20/25
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S TilingSpecification.h | 266 lines | 8e5e0860 | 25 sym | A repeating pattern as plane symmetry plus cell content — declared, deterministic, and resolving nothing.

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE LATTICE
//------------------------------------------------------------------------------------------------------------------------

T LatticeSpecification                     | TilingSpecification.h | 31-52   | nonallocating,nonthrowing     | -  | The five plane symmetries `54` §2 declares, held as one specification. an offset progression; twill is a skew and an offset progression; basket weave is reflection on both. None of them is a special case, and a design that enumerated named patterns could express exactly the patterns somebody had already thought of.
    has   CellExtentAlong          double         [-]  ?
    has   CellExtentAcross         double         [-]  ?
    has   OffsetProgressionAlong   double         [-]  ?
    has   OffsetProgressionAcross  double         [-]  ?
    has   SkewAlong                double         [-]  ?
    has   SkewAcross               double         [-]  ?
    has   ReflectionMask           std::uint32_t  [-]  ?
    has   RotationIncrement        std::uint32_t  [-]  ?
    by    Source/TilingSpecification.cpp
    note  🔴 These five **compose**. Herringbone is reflection on one axis combined with a rotation increment and

F LatticeSpecification::Validate           | TilingSpecification.h | 51      | api,nonallocating,nonthrowing | ✔️ | Whether the lattice can be classified at all. one texel of the maximum working extent, and for both offset progressions at once displacement depending on the column and a column displacement depending on the row have no consistent inverse, and a lattice that cannot be inverted cannot be sampled — which is exactly what `70` does at every promotion.
    out   -  Outcome  [-]  refuses with ContentUnsupported for a non-positive extent, for an extent finer than
    by    Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/IlluminantPopulation.h, Api/PropertySpecification.h, Source/AssetInterchange.cpp, Source/AtmosphereIntegrator.cpp, (+3 more)
    note  🔴 Both progressions together are refused rather than resolved in a declared order. A row

//------------------------------------------------------------------------------------------------------------------------
//                                                      CELL CONTENT
//------------------------------------------------------------------------------------------------------------------------

E CellContentSource                        | TilingSpecification.h | 60-67   | contract                      | -  | Where one cell element's content comes from — `54` §3's four sources.
    has   VectorOutline   CellContentSource  [-]  ?
    has   Imagery         CellContentSource  [-]  ?
    has   NestedTiling    CellContentSource  [-]  ?
    has   DeclaredColour  CellContentSource  [-]  ?
    has   SourceCount     CellContentSource  [-]  ?
    by    Source/AnalyticProjection.cpp, Source/TilingSpecification.cpp

T CellContent                              | TilingSpecification.h | 76-86   | nonallocating,nonthrowing     | -  | One element placed within the repeating cell. would have to be re-derived whenever the cell extents changed, and changing the cell extents is the first thing an artist does to a pattern. to the outline or the image — it needs to say which one, and `70` fetches it.
    has   Source          CellContentSource     [-]  ?
    has   SourceOrdinal   std::uint32_t         [-]  ?
    has   PlacedAlong     double                [-]  ?
    has   PlacedAcross    double                [-]  ?
    has   PlacedScale     double                [-]  ?
    has   PlacedRotation  double                [-]  ?
    has   DeclaredColour  ColourSpecification   [-]  ?
    has   Combination     CombineSpecification  [-]  ?
    by    Source/AnalyticProjection.cpp, Source/TilingSpecification.cpp
    note  🔴 The placing transform is **within the cell**, in the cell's own unit square. Held in domain units it
    note  📝 Sources are carried by ordinal rather than by value. `54` §4 resolves nothing, so it needs no access

//------------------------------------------------------------------------------------------------------------------------
//                                                       VARIATION
//------------------------------------------------------------------------------------------------------------------------

E VariationSubject                         | TilingSpecification.h | 98-104  | contract                      | -  | How cells differ from one another, if at all — `54` §1's three rows. third row is the one most likely to be implemented as noise and is the one that must not be: a permutation of the cell ordinal is reproducible, and sampled noise makes the same document reopen looking different.
    has   Uniform         VariationSubject  [-]  ?
    has   Progressive     VariationSubject  [-]  ?
    has   Permuted        VariationSubject  [-]  ?
    has   VariationCount  VariationSubject  [-]  ?
    by    Source/TilingSpecification.cpp
    note  🔴 `00` §5 declares continuous stochastic sources absent, and this enumeration is the substitution. The

T VariationSpecification                   | TilingSpecification.h | 108-115 | nonallocating,nonthrowing     | -  | What varies across cells, and over what interval.
    has   Declared      VariationSubject  [-]  ?
    has   DeclaredSpan  std::uint32_t     [-]  ?
    has   PatternSeed   std::uint32_t     [-]  ?
    has   LowerScale    double            [-]  ?
    has   UpperScale    double            [-]  ?
    by    Source/TilingSpecification.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                WHAT A CELL RESOLVES TO
//------------------------------------------------------------------------------------------------------------------------

T ClassifiedCell                           | TilingSpecification.h | 125-133 | nonallocating,nonthrowing     | -  | One classified domain position — which cell, where inside it, and which variation the cell carries. domain. `70` takes this and produces the texels, at whatever level a tile was promoted to.
    has   CellAlong         std::int32_t   [-]  ?
    has   CellAcross        std::int32_t   [-]  ?
    has   WithinAlong       double         [-]  ?
    has   WithinAcross      double         [-]  ?
    has   VariationOrdinal  std::uint32_t  [-]  ?
    has   VariationScale    double         [-]  ?
    by    Source/AnalyticProjection.cpp, Source/TilingSpecification.cpp
    note  🔴 Not texels. `54` §4 and §5's last gate: this document declares a pattern and resolves nothing into a

//------------------------------------------------------------------------------------------------------------------------
//                                                       ONE TILING
//------------------------------------------------------------------------------------------------------------------------

T TilingSpecification                      | TilingSpecification.h | 144-210 | owning                        | -  | One repeating pattern — a lattice, an ordered run of cell content, and how cells vary. content placed in one cell of it, and none of them is noise. The mechanism is periodic and deterministic, and saying so first is the point: pattern generation reaches for noise by habit.
    has   DeclaredLattice    LatticeSpecification      [-]  ?
    has   DeclaredContent    std::vector<CellContent>  [-]  ?
    has   DeclaredVariation  VariationSpecification    [-]  ?
    has   Depth              std::uint32_t             [-]  ?
    has   LatticeHeld        bool                      [-]  ?
    by    Source/AnalyticProjection.cpp, Source/TilingSpecification.cpp
    note  🔴 Textiles are this. Herringbone, twill, houndstooth and basket weave are all a plane symmetry with

F TilingSpecification::DeclareLattice      | TilingSpecification.h | 152     | api,nonthrowing               | ✔️ | Declares the lattice, validated before it is held.
    in    Declaring  const LatticeSpecification&  [-]  ?
    out   -          Outcome                      [-]  carries the lattice's own refusal
    by    Source/TilingSpecification.cpp

F TilingSpecification::DeclareContent      | TilingSpecification.h | 162     | api,nonthrowing               | 🚩 | Appends one content element to the cell, at the end of the ordering. space, and for a nested source in a tiling that is already nested where the complexity artists want lives; unbounded nesting makes resolution cost unbounded, and `20` §2.2's evaluation-cost budget cannot bound what it cannot predict.
    in    Declaring  const CellContent&  [-]  ?
    out   -          Outcome             [-]  refuses with ContentUnsupported for a non-positive scale, for a colour declaring no
    by    Source/TilingSpecification.cpp
    note  🔴 `54` §3: nesting is bounded at `TilingNestingCeiling`. A weave whose thread is itself a weave is

F TilingSpecification::DeclareVariation    | TilingSpecification.h | 169     | api,nonthrowing               | ✔️ | Declares how cells differ. variation interval
    in    Declaring  const VariationSpecification&  [-]  ?
    out   -          Outcome                        [-]  refuses with ContentUnsupported for a declared span of zero, and for an inverted
    by    Api/BrushSpecification.h, Source/BrushSpecification.cpp, Source/TilingSpecification.cpp

F TilingSpecification::DeclareNestingDepth | TilingSpecification.h | 176     | api,nonallocating,nonthrowing | ✔️ | Declares this tiling as nested inside another, which bars it from nesting one itself. nested element afterwards rather than only at the moment it is admitted.
    in    Depth  std::uint32_t  [-]  ?
    out   -      void           [-]  ?
    by    Source/TilingSpecification.cpp
    note  Recorded here rather than checked by the holder, so that a tiling admitted into a cell can refuse a

F TilingSpecification::Classify            | TilingSpecification.h | 187     | api,nonthrowing               | ✔️ | Classifies one domain position into its cell. through arithmetic written here. `54` §5's gate is that the host and the device agree about which cell a position falls in, and two implementations of one boundary are two that will disagree.
    in    PositionAlong   double   [-]  the domain's first axis
    in    PositionAcross  double   [-]  its second
    out   -               Outcome  [-]  refuses with ContentUnsupported while no valid lattice is declared
    by    Api/CameraProjection.h, Api/VectorInterchange.h, Api/VendorClassifier.h, Source/AnalyticProjection.cpp, Source/CameraProjection.cpp, Source/ConsoleHost.cpp, (+6 more)
    note  🔴 Classification goes through `02` §5's `LatticeProjection` in `Shared/`, at Tier A, and never

F TilingSpecification::Lattice             | TilingSpecification.h | 189     | -                             | -  | ?
    out   -  const LatticeSpecification&  [-]  ?
    by    Source/TilingSpecification.cpp

F TilingSpecification::Content             | TilingSpecification.h | 190     | -                             | -  | ?
    out   -  const std::vector<CellContent>&  [-]  ?
    by    Api/AnalyticProjection.h, Api/ImpressionSequence.h, Contract/OutcomeContract.h, Source/AnalyticProjection.cpp, Source/ConsoleHost.cpp, Source/DescriptorIndex.cpp, (+2 more)

F TilingSpecification::Variation           | TilingSpecification.h | 191     | -                             | -  | ?
    out   -  const VariationSpecification&  [-]  ?
    by    Api/BrushSpecification.h, Source/BrushSpecification.cpp, Source/TilingSpecification.cpp

F TilingSpecification::NestingDepth        | TilingSpecification.h | 196     | api,nonallocating,nonthrowing | ✔️ | How deeply this tiling is nested; zero for one applied directly.
    out   -  std::uint32_t  [-]  ?
    by    Api/AnalyticProjection.h, Api/SurfaceLayerSequence.h, Source/AnalyticProjection.cpp, Source/SurfaceLayerSequence.cpp, Source/TilingSpecification.cpp

F TilingSpecification::LatticeDeclared     | TilingSpecification.h | 201     | api,nonallocating,nonthrowing | ✔️ | Whether a lattice has been validly declared.
    out   -  bool  [-]  ?
    by    Source/TilingSpecification.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE TILINGS
//------------------------------------------------------------------------------------------------------------------------

T TilingIndex                              | TilingSpecification.h | 221-260 | owning                        | -  | Every declared tiling in the document, addressed by ordinal, with the nesting bound enforced across them. been nested inside. Admitting a nested reference is where the depth is known, so it is where the refusal belongs.
    has   TilingCeiling  static constexpr std::uint32_t    [-]  ?
    has   Declared       std::vector<TilingSpecification>  [-]  ?
    by    Api/AnalyticProjection.h, Source/TilingSpecification.cpp
    note  🔴 The bound is enforced **here** rather than inside a tiling, because a tiling cannot see what it has

F TilingIndex::Declare                     | TilingSpecification.h | 229     | api,nonthrowing               | 🚩 | Declares one tiling and issues its ordinal.
    out   -  Outcome  [-]  refuses with ExtentExhausted at the declared ceiling
    by    Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/IlluminantPopulation.h, (+30 more)

F TilingIndex::Resolve                     | TilingSpecification.h | 235     | api,nonthrowing               | ✔️ | One declared tiling, for reading.
    in    TilingOrdinal  std::uint32_t  [-]  ?
    out   -              Outcome        [-]  refuses with ContentUnsupported outside the declared count
    by    Api/AtmosphereIntegrator.h, Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/IlluminantPopulation.h, (+58 more)

F TilingIndex::Amend                       | TilingSpecification.h | 241     | api,nonthrowing               | ✔️ | One declared tiling, for amending.
    in    TilingOrdinal  std::uint32_t  [-]  ?
    out   -              Outcome        [-]  refuses with ContentUnsupported outside the declared count
    by    Api/BrushSpecification.h, Api/CameraProjection.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/IlluminantPopulation.h, Api/ImpressionSequence.h, (+20 more)

F TilingIndex::Nest                        | TilingSpecification.h | 251     | api,nonthrowing               | 🚩 | Nests one tiling inside a cell of another, at the declared bound. inside itself, and for a nesting that would exceed `TilingNestingCeiling`
    in    EnclosingOrdinal  std::uint32_t  [-]  the tiling whose cell carries it
    in    NestedOrdinal     std::uint32_t  [-]  the tiling being nested
    out   -                 Outcome        [-]  refuses with ContentUnsupported for an unknown ordinal, for a tiling nested
    post  the nested tiling refuses a nested element of its own from this point
    by    Api/SurfaceLayerSequence.h, Source/SurfaceLayerSequence.cpp, Source/TilingSpecification.cpp

F TilingIndex::DeclaredCount               | TilingSpecification.h | 253     | -                             | -  | ?
    out   -  std::uint32_t  [-]  ?
    by    Api/AttachmentIndex.h, Api/BrushSpecification.h, Api/DecalProjection.h, Api/DescriptorIndex.h, Api/MaterialSpecification.h, Api/ProgramIndex.h, (+19 more)

F SLATE_DECLARES_PRECISION                 | TilingSpecification.h | 264     | -                             | -  | ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Bounded  PrecisionGuarantee::  [-]  ?
    in    Exact    PrecisionGuarantee::  [-]  ?
    by    Api/AnalyticProjection.h, Api/AssetInterchange.h, Api/AtmosphereIntegrator.h, Api/BrushSpecification.h, Api/CameraProjection.h, Api/ChartPartition.h, (+24 more)
