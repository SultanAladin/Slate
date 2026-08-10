//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Declaration by either route, flattening at a supplied tolerance, and classification per declared rule.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/VectorInterchange/Source
%layer      SlateDocument
%sources    1
%symbols    16
%annotated  0/16
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S VectorInterchange.cpp | 215 lines | ad741152 | 16 sym | Declaration by either route, flattening at a supplied tolerance, and classification per declared rule.

//------------------------------------------------------------------------------------------------------------------------
//                                                      DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F VectorInterchange::DeclareFromFile     | VectorInterchange.cpp | 17-28   | - | - | ?
    in    Arriving    const OutlineSpecification&  [-]  ?
    in    OriginPath  const std::string&           [-]  ?
    out   -           Outcome<bool>                [-]  ?

F VectorInterchange::DeclareFromText     | VectorInterchange.cpp | 30-41   | - | - | ?
    in    Arriving    const OutlineSpecification&  [-]  ?
    in    SourceText  const std::string&           [-]  ?
    out   -           Outcome<bool>                [-]  ?

F VectorInterchange::Refuse              | VectorInterchange.cpp | 43-51   | - | - | ?
    in    Construct      const std::string&  [-]  ?
    in    SourceOrdinal  std::uint32_t       [-]  ?
    in    Declining      const Refusal&      [-]  ?
    out   -              void                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       FLATTENING
//------------------------------------------------------------------------------------------------------------------------

F VectorInterchange::Flatten             | VectorInterchange.cpp | 57-81   | - | - | ?
    in    Tolerance  double                                    [-]  ?
    out   -          std::vector<std::vector<PlanarPosition>>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

F VectorInterchange::Classify            | VectorInterchange.cpp | 87-131  | - | - | ?
    in    Flattened  const std::vector<std::vector<PlanarPosition>>&  [-]  ?
    in    PointX     double                                           [-]  ?
    in    PointY     double                                           [-]  ?
    out   -          std::int32_t                                     [-]  ?

F VectorInterchange::Declared            | VectorInterchange.cpp | 133     | - | - | ?
    out   -  const OutlineSpecification&  [-]  ?

F VectorInterchange::Refusals            | VectorInterchange.cpp | 134     | - | - | ?
    out   -  const std::vector<RefusedConstruct>&  [-]  ?

F VectorInterchange::TextRetained        | VectorInterchange.cpp | 135     | - | - | ?
    out   -  bool  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       TYPEFACES
//------------------------------------------------------------------------------------------------------------------------

F TypefaceInterchange::DeclareTypeface   | VectorInterchange.cpp | 141-145 | - | - | ?
    in    TypefaceIdentity_  std::uint32_t  [-]  ?
    in    UnitsPerEm_        double         [-]  ?
    out   -                  void           [-]  ?

F TypefaceInterchange::DeclareGlyph      | VectorInterchange.cpp | 147-161 | - | - | ?
    in    Declaring  const GlyphSpecification&  [-]  ?
    out   -          Outcome<bool>              [-]  ?

F TypefaceInterchange::DeclareAdjustment | VectorInterchange.cpp | 163-182 | - | - | ?
    in    EarlierGlyph  std::uint32_t  [-]  ?
    in    LaterGlyph    std::uint32_t  [-]  ?
    in    Adjustment_   double         [-]  ?
    out   -             void           [-]  ?

F TypefaceInterchange::ResolveGlyph      | VectorInterchange.cpp | 184-194 | - | - | ?
    in    GlyphIdentity  std::uint32_t                       [-]  ?
    out   -              Outcome<const GlyphSpecification*>  [-]  ?

F TypefaceInterchange::Adjustment        | VectorInterchange.cpp | 196-205 | - | - | ?
    in    EarlierGlyph  std::uint32_t  [-]  ?
    in    LaterGlyph    std::uint32_t  [-]  ?
    out   -             double         [-]  ?

F TypefaceInterchange::TypefaceIdentity  | VectorInterchange.cpp | 207     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F TypefaceInterchange::UnitsPerEm        | VectorInterchange.cpp | 208     | - | - | ?
    out   -  double  [-]  ?

F TypefaceInterchange::GlyphCount        | VectorInterchange.cpp | 210-213 | - | - | ?
    out   -  std::uint32_t  [-]  ?
