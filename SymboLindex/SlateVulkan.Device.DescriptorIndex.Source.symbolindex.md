//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 The layout declaration that closes at bring-up, the extent it is sized against, and the per-rotation write.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateVulkan/Device/DescriptorIndex/Source
%layer      SlateVulkan
%sources    1
%symbols    12
%annotated  0/12
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S DescriptorIndex.cpp | 390 lines | 66ce1782 | 12 sym | The layout declaration that closes at bring-up, the extent it is sized against, and the per-rotation write.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F DescriptorIndex::Construct        | DescriptorIndex.cpp | 15-23   | -          | - | ?
    in    Exchange  const VulkanExchange&  [-]  ?
    out   -         Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F DescriptorIndex::Declare          | DescriptorIndex.cpp | 29-93   | -          | - | ?
    in    Declared  const std::vector<DescriptorSlot>&  [-]  ?
    out   -         Outcome<std::uint32_t>              [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE ONE EXTENT
//------------------------------------------------------------------------------------------------------------------------

F DescriptorIndex::Fix              | DescriptorIndex.cpp | 99-163  | -          | - | ?
    in    ConcurrentSets  std::uint32_t  [-]  ?
    out   -               Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CLAIM
//------------------------------------------------------------------------------------------------------------------------

F DescriptorIndex::Claim            | DescriptorIndex.cpp | 169-201 | -          | - | ?
    in    LayoutOrdinal  std::uint32_t           [-]  ?
    out   -              Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE WRITE
//------------------------------------------------------------------------------------------------------------------------

F DescriptorIndex::SlotOf           | DescriptorIndex.cpp | 207-216 | -          | - | ?
    in    Holding      const DeclaredLayout&  [-]  ?
    in    SlotOrdinal  std::uint32_t          [-]  ?
    out   -            const DescriptorSlot*  [-]  ?

F DescriptorIndex::Amend            | DescriptorIndex.cpp | 218-311 | -          | - | ?
    in    ClaimOrdinal  std::uint32_t                          [-]  ?
    in    RotationSlot  std::uint32_t                          [-]  ?
    in    Amended       const std::vector<DescriptorContent>&  [-]  ?
    out   -             Outcome<bool>                          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    WHAT IS DECLARED
//------------------------------------------------------------------------------------------------------------------------

F DescriptorIndex::Resolve          | DescriptorIndex.cpp | 317-329 | -          | - | ?
    in    ClaimOrdinal  std::uint32_t             [-]  ?
    in    RotationSlot  std::uint32_t             [-]  ?
    out   -             Outcome<VkDescriptorSet>  [-]  ?

F DescriptorIndex::Layout           | DescriptorIndex.cpp | 331-340 | -          | - | ?
    in    LayoutOrdinal  std::uint32_t                   [-]  ?
    out   -              Outcome<VkDescriptorSetLayout>  [-]  ?

F DescriptorIndex::DeclaredCount    | DescriptorIndex.cpp | 342-345 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

F DescriptorIndex::ClaimedCount     | DescriptorIndex.cpp | 347-350 | -          | - | ?
    out   -  std::uint32_t  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F DescriptorIndex::Reclaim          | DescriptorIndex.cpp | 356-383 | -          | - | ?
    out   -  void  [-]  ?

F DescriptorIndex::~DescriptorIndex | DescriptorIndex.cpp | 385-388 | destructor | - | ?
