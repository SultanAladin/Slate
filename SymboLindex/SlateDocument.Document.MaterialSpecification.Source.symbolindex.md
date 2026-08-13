//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The channel inventory per reflectance, declaration validation, and the partition resolution.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateDocument/Document/MaterialSpecification/Source
%layer      SlateDocument
%sources    1
%symbols    23
%annotated  0/23
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S MaterialSpecification.cpp | 279 lines | dd247b6e | 23 sym | The channel inventory per reflectance, declaration validation, and the partition resolution.

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE CHANNEL INVENTORY
//------------------------------------------------------------------------------------------------------------------------

F ChannelBit                                    | MaterialSpecification.cpp | 22-25   | - | - | ?
    in    Subject  ChannelSubject           [-]  ?
    out   -        constexpr std::uint32_t  [-]  ?
    by    Source/UvSurfaceDepot.cpp

F ChannelBit                                    | MaterialSpecification.cpp | 34      | - | - | ?
    in    Opacity  ChannelSubject::  [-]  ?
    out   -        /                 [-]  ?
    by    Source/UvSurfaceDepot.cpp

F ChannelConsumed                               | MaterialSpecification.cpp | 65-76   | - | - | ?
    in    Selected  ReflectanceSelection  [-]  ?
    in    Channel   ChannelSubject        [-]  ?
    out   -         bool                  [-]  ?
    by    Api/MaterialSpecification.h, Source/ReflectanceIntegrator.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE MATERIAL
//------------------------------------------------------------------------------------------------------------------------

F MaterialSpecification::DeclareReflectance     | MaterialSpecification.cpp | 82-87   | - | - | ?
    in    Selecting  ReflectanceSelection  [-]  ?
    out   -          void                  [-]  ?

F MaterialSpecification::DeclareChannel         | MaterialSpecification.cpp | 89-129  | - | - | ?
    in    Channel    ChannelSubject               [-]  ?
    in    Declaring  const ChannelSpecification&  [-]  ?
    out   -          Outcome<bool>                [-]  ?

F MaterialSpecification::DeclareCutoutThreshold | MaterialSpecification.cpp | 131-134 | - | - | ?
    in    Threshold  double  [-]  ?
    out   -          void    [-]  ?

F MaterialSpecification::DeclareCutoutEnrolment | MaterialSpecification.cpp | 136-139 | - | - | ?
    in    CutoutEnabled  bool  [-]  ?
    out   -              void  [-]  ?

F MaterialSpecification::Reflectance            | MaterialSpecification.cpp | 141     | - | - | ?
    out   -  ReflectanceSelection  [-]  ?

F MaterialSpecification::CutoutThreshold        | MaterialSpecification.cpp | 142     | - | - | ?
    out   -  double  [-]  ?

F MaterialSpecification::CutoutEnrolled         | MaterialSpecification.cpp | 143     | - | - | ?
    out   -  bool  [-]  ?

F MaterialSpecification::Channel                | MaterialSpecification.cpp | 145-150 | - | - | ?
    in    Subject  ChannelSubject               [-]  ?
    out   -        const ChannelSpecification&  [-]  ?

F MaterialSpecification::ChannelSampled         | MaterialSpecification.cpp | 152-165 | - | - | ?
    in    Subject  ChannelSubject  [-]  ?
    out   -        bool            [-]  ?

F MaterialSpecification::ChannelConverted       | MaterialSpecification.cpp | 167-174 | - | - | ?
    in    Subject  ChannelSubject  [-]  ?
    out   -        bool            [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE MATERIALS
//------------------------------------------------------------------------------------------------------------------------

F MaterialIndex::Declare                        | MaterialSpecification.cpp | 180-194 | - | - | ?
    in    Named  const std::string&      [-]  ?
    out   -      Outcome<std::uint32_t>  [-]  ?

F MaterialIndex::Resolve                        | MaterialSpecification.cpp | 196-205 | - | - | ?
    in    MaterialOrdinal  std::uint32_t                          [-]  ?
    out   -                Outcome<const MaterialSpecification*>  [-]  ?

F MaterialIndex::Amend                          | MaterialSpecification.cpp | 207-213 | - | - | ?
    in    MaterialOrdinal  std::uint32_t                    [-]  ?
    out   -                Outcome<MaterialSpecification*>  [-]  ?

F MaterialIndex::DeclaredName                   | MaterialSpecification.cpp | 215-218 | - | - | ?
    in    MaterialOrdinal  std::uint32_t       [-]  ?
    out   -                const std::string&  [-]  ?

F MaterialIndex::DeclaredCount                  | MaterialSpecification.cpp | 220-223 | - | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  PARTITION RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F PartitionResolutionIndex::Reclaim             | MaterialSpecification.cpp | 229-237 | - | - | ?
    out   -  void  [-]  ?

F PartitionResolutionIndex::Declare             | MaterialSpecification.cpp | 239-260 | - | - | ?
    in    Resolving  const ResolvedPartition&    [-]  ?
    out   -          Outcome<PartitionIdentity>  [-]  ?

F PartitionResolutionIndex::Resolve             | MaterialSpecification.cpp | 262-274 | - | - | ?
    in    Subject  PartitionIdentity           [-]  ?
    out   -        Outcome<ResolvedPartition>  [-]  ?

F PartitionResolutionIndex::Revision            | MaterialSpecification.cpp | 276     | - | - | ?
    out   -  std::uint64_t  [-]  ?

F PartitionResolutionIndex::DeclaredCount       | MaterialSpecification.cpp | 277     | - | - | ?
    out   -  std::uint32_t  [-]  ?
