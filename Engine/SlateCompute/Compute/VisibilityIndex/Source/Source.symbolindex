//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Halving a display extent into a level chain, and the integer logarithm that picks the level one partition is tested at.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/VisibilityIndex/Source
%layer      SlateCompute
%sources    6
%symbols    76
%annotated  2/76
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S DepthReduction.cpp      | 145 lines | 2f71b1b9 | 9 sym  | Halving a display extent into a level chain, and the integer logarithm that picks the level one partition is tested at.
S OcclusionScheduler.cpp  | 997 lines | b9ff8e31 | 20 sym | The claimed chain, the per-level reduction it is filled by, and the two culling dispatches that compact survivors out of it.
S PartitionClassifier.cpp | 257 lines | c62bbfe2 | 3 sym  | Eight corners through a projective transform, one cone against one bounding sphere, and the standing the two amount to.
S PartitionStructure.cpp  | 470 lines | 3e10c419 | 15 sym | The growth front that walks `38`'s adjacency, the cone it accumulates, and the identities `42` issues against the result.
S VisibilityIndex.cpp     | 213 lines | 30ba87b3 | 12 sym | The enrolment run, the recording `08` §3 ② is contributed as, and the two indexed hops one written pixel resolves through.
S VisibilityRaster.cpp    | 849 lines | 09b31257 | 17 sym | The composition, the fan, the residency it becomes, and the one recording that writes `16` §4's targets.

//------------------------------------------------------------------------------------------------------------------------
//                                                      CONSTRUCTION
//------------------------------------------------------------------------------------------------------------------------

F DepthReduction::Construct                | DepthReduction.cpp      | 15-61   | - | - | ?
    in    DisplayAlong   std::uint32_t  [-]  ?
    in    DisplayAcross  std::uint32_t  [-]  ?
    out   -              Outcome<bool>  [-]  ?

F DepthReduction::Reclaim                  | DepthReduction.cpp      | 63-70   | - | - | ?
    out   -  void  [-]  ?

F OcclusionScheduler::Construct            | OcclusionScheduler.cpp  | 46-151  | - | - | ?
    in    Spans        SpanSpace&          [-]  ?
    in    Images       ImageSpace&         [-]  ?
    in    Claimed      const TargetSpace&  [-]  ?
    in    Modules      ShaderCodec&        [-]  ?
    in    Descriptors  DescriptorIndex&    [-]  ?
    in    Programs     ProgramIndex&       [-]  ?
    out   -            Outcome<bool>       [-]  ?

V VisibilityRecordingIdentity              | VisibilityIndex.cpp     | 22      | - | - | ?

V RecordingSubstitution                    | VisibilityIndex.cpp     | 29      | - | - | ?

F VisibilityIndex::Construct               | VisibilityIndex.cpp     | 33-38   | - | - | ?
    in    DisplayAlong   std::uint32_t  [-]  ?
    in    DisplayAcross  std::uint32_t  [-]  ?
    out   -              Outcome<bool>  [-]  ?

F VisibilityIndex::Reclaim                 | VisibilityIndex.cpp     | 40-48   | - | - | ?
    out   -  void  [-]  ?

F VisibilityRaster::Construct              | VisibilityRaster.cpp    | 68-182  | - | - | ?
    in    Spans        SpanSpace&        [-]  ?
    in    Modules      ShaderCodec&      [-]  ?
    in    Descriptors  DescriptorIndex&  [-]  ?
    in    Programs     ProgramIndex&     [-]  ?
    in    Attachments  AttachmentIndex&  [-]  ?
    out   -            Outcome<bool>     [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    LEVEL SELECTION
//------------------------------------------------------------------------------------------------------------------------

F DepthReduction::Level                    | DepthReduction.cpp      | 76-82   | - | - | ?
    in    LevelOrdinal  std::uint32_t            [-]  ?
    out   -             Outcome<ReductionLevel>  [-]  ?

F DepthReduction::LevelOfExtent            | DepthReduction.cpp      | 84-106  | - | - | ?
    in    ProjectedAlong   std::uint32_t           [-]  ?
    in    ProjectedAcross  std::uint32_t           [-]  ?
    out   -                Outcome<std::uint32_t>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE READS
//------------------------------------------------------------------------------------------------------------------------

F DepthReduction::ChainTexels              | DepthReduction.cpp      | 112-123 | - | - | ?
    out   -  std::uint64_t  [-]  ?

F DepthReduction::LevelCount               | DepthReduction.cpp      | 125-128 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F DepthReduction::DisplayAlong             | DepthReduction.cpp      | 130-133 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F DepthReduction::DisplayAcross            | DepthReduction.cpp      | 135-138 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F DepthReduction::ChainDerived             | DepthReduction.cpp      | 140-143 | - | - | ?
    out   -  bool  [-]  ?

F OcclusionScheduler::RecordOf             | OcclusionScheduler.cpp  | 910-927 | - | - | ?
    in    CullingOrdinal  std::uint32_t      [-]  ?
    in    RotationSlot    std::uint32_t      [-]  ?
    in    Phase           CullingPhase       [-]  ?
    out   -               Outcome<VkBuffer>  [-]  ?

F OcclusionScheduler::SurvivingOf          | OcclusionScheduler.cpp  | 929-946 | - | - | ?
    in    CullingOrdinal  std::uint32_t      [-]  ?
    in    RotationSlot    std::uint32_t      [-]  ?
    in    Phase           CullingPhase       [-]  ?
    out   -               Outcome<VkBuffer>  [-]  ?

F OcclusionScheduler::CulledCount          | OcclusionScheduler.cpp  | 948     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F OcclusionScheduler::LevelCount           | OcclusionScheduler.cpp  | 949     | - | - | ?
    out   -  std::uint32_t  [-]  ?

F OcclusionScheduler::ChainDerived         | OcclusionScheduler.cpp  | 950     | - | - | ?
    out   -  bool  [-]  ?

F OcclusionScheduler::ChainReduced         | OcclusionScheduler.cpp  | 951     | - | - | ?
    out   -  bool  [-]  ?

F OcclusionScheduler::ProgramsStanding     | OcclusionScheduler.cpp  | 953-956 | - | - | ?
    out   -  bool  [-]  ?

F PartitionStructure::Standing             | PartitionStructure.cpp  | 431-434 | - | - | ?
    out   -  const DerivedPartitioning&  [-]  ?

F PartitionStructure::IdentityOf           | PartitionStructure.cpp  | 436-448 | - | - | ?
    in    PartitionOrdinal  std::uint32_t               [-]  ?
    out   -                 Outcome<PartitionIdentity>  [-]  ?

F PartitionStructure::PartitioningStanding | PartitionStructure.cpp  | 450-453 | - | - | ?
    out   -  bool  [-]  ?

F PartitionStructure::Revision             | PartitionStructure.cpp  | 455-458 | - | - | ?
    out   -  std::uint64_t  [-]  ?

F PartitionStructure::DescribedRevision    | PartitionStructure.cpp  | 460-463 | - | - | ?
    out   -  std::uint64_t  [-]  ?

F PartitionStructure::PartitionCount       | PartitionStructure.cpp  | 465-468 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F VisibilityIndex::Enrolled                | VisibilityIndex.cpp     | 185-191 | - | - | ?
    in    EnrolmentOrdinal  std::uint32_t                       [-]  ?
    out   -                 Outcome<const PartitionStructure*>  [-]  ?

F VisibilityIndex::Reduction               | VisibilityIndex.cpp     | 193-196 | - | - | ?
    out   -  const DepthReduction&  [-]  ?

F VisibilityIndex::EnrolledCount           | VisibilityIndex.cpp     | 198-201 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F VisibilityIndex::DeclaredPartitionCount  | VisibilityIndex.cpp     | 203-206 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F VisibilityIndex::ChainDerived            | VisibilityIndex.cpp     | 208-211 | - | - | ?
    out   -  bool  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 WHAT THE DEVICE READS
//------------------------------------------------------------------------------------------------------------------------

V ReductionWorkgroupEdge                   | OcclusionScheduler.cpp  | 22      | - | - | ?

V OcclusionWorkgroupLanes                  | OcclusionScheduler.cpp  | 27      | - | - | ?

V OrdinalsPerLevel                         | OcclusionScheduler.cpp  | 33      | - | - | ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE ORDERING
//------------------------------------------------------------------------------------------------------------------------

F OcclusionScheduler::Order                | OcclusionScheduler.cpp  | 157-171 | - | - | ?
    in    Recorded    VkCommandBuffer       [-]  ?
    in    ReadStages  VkPipelineStageFlags  [-]  ?
    in    ReadAccess  VkAccessFlags         [-]  ?
    out   -           void                  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE CHAIN
//------------------------------------------------------------------------------------------------------------------------

F OcclusionScheduler::Derive               | OcclusionScheduler.cpp  | 177-378 | - | - | ?
    in    DisplayAlong   std::uint32_t  [-]  ?
    in    DisplayAcross  std::uint32_t  [-]  ?
    out   -              Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESIDENCY
//------------------------------------------------------------------------------------------------------------------------

F OcclusionScheduler::Abandon              | OcclusionScheduler.cpp  | 384-420 | - | - | ?
    in    Abandoned  CulledResidency&  [-]  ?
    out   -          void              [-]  ?

F OcclusionScheduler::Resolve              | OcclusionScheduler.cpp  | 422-626 | - | - | ?
    in    TriangleCeiling  std::uint32_t           [-]  ?
    in    PartitionCount   std::uint32_t           [-]  ?
    out   -                Outcome<std::uint32_t>  [-]  ?

F VisibilityRaster::Resolve                | VisibilityRaster.cpp    | 343-546 | - | - | ?
    in    Enrolled        const PartitionStructure&  [-]  ?
    in    Imported        const TopologyStructure&   [-]  ?
    in    EnrolmentBase   std::uint32_t              [-]  ?
    in    Culling         const OcclusionScheduler*  [-]  ?
    in    CullingOrdinal  std::uint32_t              [-]  ?
    in    Recorded        VkCommandBuffer            [-]  ?
    out   -               Outcome<std::uint32_t>     [-]  ?

F VisibilityRaster::Surrender              | VisibilityRaster.cpp    | 548-557 | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

F OcclusionScheduler::Amend                | OcclusionScheduler.cpp  | 632-684 | - | - | ?
    in    CullingOrdinal  std::uint32_t                            [-]  ?
    in    RotationSlot    std::uint32_t                            [-]  ?
    in    Classified      const std::vector<ClassifiedPartition>&  [-]  ?
    out   -               Outcome<bool>                            [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REDUCTION
//------------------------------------------------------------------------------------------------------------------------

F OcclusionScheduler::ReduceLevel          | OcclusionScheduler.cpp  | 690-758 | - | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    RotationSlot  std::uint32_t    [-]  ?
    in    LevelOrdinal  std::uint32_t    [-]  ?
    out   -             Outcome<bool>    [-]  ?

F OcclusionScheduler::Reduce               | OcclusionScheduler.cpp  | 760-804 | - | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    RotationSlot  std::uint32_t    [-]  ?
    out   -             Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE CULL
//------------------------------------------------------------------------------------------------------------------------

F OcclusionScheduler::Cull                 | OcclusionScheduler.cpp  | 810-904 | - | - | ?
    in    Recorded      VkCommandBuffer  [-]  ?
    in    RotationSlot  std::uint32_t    [-]  ?
    in    Phase         CullingPhase     [-]  ?
    out   -             Outcome<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE RECLAMATION
//------------------------------------------------------------------------------------------------------------------------

F OcclusionScheduler::Reclaim              | OcclusionScheduler.cpp  | 962-995 | - | - | ?
    out   -  void  [-]  ?

F VisibilityRaster::Reclaim                | VisibilityRaster.cpp    | 819-827 | - | - | ?
    out   -  void  [-]  ?

F VisibilityRaster::ResidentCount          | VisibilityRaster.cpp    | 829-832 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F VisibilityRaster::DrawnTriangleCount     | VisibilityRaster.cpp    | 834-842 | - | - | ?
    out   -  std::uint32_t  [-]  ?

F VisibilityRaster::ProgramStanding        | VisibilityRaster.cpp    | 844-847 | - | - | ?
    out   -  bool  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE PROJECTED EXTENT
//------------------------------------------------------------------------------------------------------------------------

F ProjectPartitionExtent                   | PartitionClassifier.cpp | 18-150  | - | - | ?
    in    Composed       const ProjectedTransform&  [-]  ?
    in    Bounded        const ConditionedExtent&   [-]  ?
    in    DisplayAlong   std::uint32_t              [-]  ?
    in    DisplayAcross  std::uint32_t              [-]  ?
    out   -              ClassifiedPartition        [-]  ?
    by    Api/PartitionClassifier.h

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE ORIENTATION CONE
//------------------------------------------------------------------------------------------------------------------------

F OrientationRejected                      | PartitionClassifier.cpp | 156-210 | - | - | ?
    in    Turned      const OrientationCone&    [-]  ?
    in    Bounded     const ConditionedExtent&  [-]  ?
    in    ViewOrigin  DocumentPosition          [-]  ?
    out   -           bool                      [-]  ?
    by    Api/PartitionClassifier.h

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE FIRST PHASE
//------------------------------------------------------------------------------------------------------------------------

F ClassifyPartition                        | PartitionClassifier.cpp | 216-255 | - | - | ?
    in    Partitioned       const MicroSurfacePartition&  [-]  ?
    in    Viewing           const ViewProjection&         [-]  ?
    in    Bounding          const FrustumSpace&           [-]  ?
    in    Composed          const ProjectedTransform&     [-]  ?
    in    PartitionOrdinal  std::uint32_t                 [-]  ?
    in    FirstTriangle     std::uint32_t                 [-]  ?
    in    DisplayAlong      std::uint32_t                 [-]  ?
    in    DisplayAcross     std::uint32_t                 [-]  ?
    out   -                 ClassifiedPartition           [-]  ?
    by    Api/PartitionClassifier.h

//------------------------------------------------------------------------------------------------------------------------
//                                                    FACE ORIENTATION
//------------------------------------------------------------------------------------------------------------------------

F MaterialOfFace                           | PartitionStructure.cpp  | 23-28   | - | - | ?
    in    Imported     const TopologyStructure&  [-]  ?
    in    FaceOrdinal  std::uint32_t             [-]  ?
    out   -            std::uint32_t             [-]  ?

F TrianglesOfFace                          | PartitionStructure.cpp  | 33-38   | - | - | ?
    in    Imported     const TopologyStructure&  [-]  ?
    in    FaceOrdinal  std::uint32_t             [-]  ?
    out   -            std::uint32_t             [-]  ?

F OrientationOfFace                        | PartitionStructure.cpp  | 48-89   | - | - | One face's own orientation, by Newell's summation over its corner run. may be collinear or may sit on a locally concave part of an otherwise well-behaved face. Newell reads every corner and produces the area-weighted orientation of the whole polygon, which is what the cone is supposed to enclose. `02` §3.2 keeps them at 64 bits precisely because differencing them at 32 bits is where a distant occupant's geometry turns to noise.
    in    Imported            const TopologyStructure&  [-]  ?
    in    FaceOrdinal         std::uint32_t             [-]  ?
    in    OrientationDerived  bool&                     [-]  ?
    out   -                   SurfaceDirection          [-]  ?
    by    Source/UvSurfaceDepot.cpp
    note  📐 Newell rather than a cross product of the first three corners, because an n-gon's first three corners
    note  📝 Accumulated at 64 bits and narrowed only at the end. The positions are `mm` in document space and

F AdmitExtent                              | PartitionStructure.cpp  | 94-108  | - | - | ?
    in    Running         ConditionedExtent&        [-]  ?
    in    Arriving        const ConditionedExtent&  [-]  ?
    in    FirstAdmission  bool                      [-]  ?
    out   -               void                      [-]  ?

F CloseCone                                | PartitionStructure.cpp  | 118-134 | - | - | Closes the cone over the orientations one partition accumulated. one of them takes against it. The sum is unweighted, so a partition of a thousand tiny faces and one large one is centred where the faces are rather than where the area is — which is the direction the aperture then has to enclose, so the two agree. face is back-facing. The cone is withheld rather than reported wide, because `16` §2 ① reads only the two numbers and cannot tell a wide cone from a meaningless one.
    in    SummedX            double           [-]  ?
    in    SummedY            double           [-]  ?
    in    SummedZ            double           [-]  ?
    in    Aperture           double           [-]  ?
    in    EveryFaceOriented  bool             [-]  ?
    out   -                  OrientationCone  [-]  ?
    note  📐 The axis is the normalised sum of the face orientations and the aperture is the least dot product any
    note  🔴 An aperture at or below zero spans a hemisphere or more, and no direction exists from which every

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE DERIVATION
//------------------------------------------------------------------------------------------------------------------------

F DerivePartitioning                       | PartitionStructure.cpp  | 142-358 | - | - | ?
    in    Imported     const TopologyStructure&      [-]  ?
    in    Conditioned  const TopologyConditioning&   [-]  ?
    out   -            Outcome<DerivedPartitioning>  [-]  ?
    by    Api/PartitionStructure.h, Source/VisibilityIndex.cpp

F VisibilityRaster::Derive                 | VisibilityRaster.cpp    | 563-569 | - | - | ?
    in    DisplayAlong   std::uint32_t  [-]  ?
    in    DisplayAcross  std::uint32_t  [-]  ?
    out   -              Outcome<bool>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                ADOPTION AND DECLARATION
//------------------------------------------------------------------------------------------------------------------------

F PartitionStructure::Adopt                | PartitionStructure.cpp  | 364-380 | - | - | ?
    in    Arriving  const DerivedPartitioning&  [-]  ?
    out   -         Outcome<bool>               [-]  ?

F PartitionStructure::Declare              | PartitionStructure.cpp  | 382-417 | - | - | ?
    in    Resolutions  PartitionResolutionIndex&  [-]  ?
    in    Occupant     OccupantIdentity           [-]  ?
    out   -            Outcome<bool>              [-]  ?

F PartitionStructure::Reclaim              | PartitionStructure.cpp  | 419-425 | - | - | ?
    out   -  void  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RECORDING
//------------------------------------------------------------------------------------------------------------------------

F VisibilityIndex::Contribute              | VisibilityIndex.cpp     | 54-79   | - | - | ?
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Outcome<bool>    [-]  ?

F VisibilityRaster::Open                   | VisibilityRaster.cpp    | 575-639 | - | - | ?
    in    Recorded     VkCommandBuffer           [-]  ?
    in    Constructed  ConstructedProgram&       [-]  ?
    out   -            Outcome<ConstructedSpan>  [-]  ?

F VisibilityRaster::Project                | VisibilityRaster.cpp    | 641-667 | - | - | ?
    in    Standing           const ResidentPartitioning&  [-]  ?
    in    RotationSlot       std::uint32_t                [-]  ?
    in    Viewing            const ViewProjection&        [-]  ?
    in    Covering           const ConstructedSpan&       [-]  ?
    in    SurvivingResolved  bool                         [-]  ?
    out   -                  Outcome<bool>                [-]  ?

F VisibilityRaster::Record                 | VisibilityRaster.cpp    | 669-727 | - | - | ?
    in    Recorded      VkCommandBuffer        [-]  ?
    in    RotationSlot  std::uint32_t          [-]  ?
    in    Viewing       const ViewProjection&  [-]  ?
    out   -             Outcome<bool>          [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       ENROLMENT
//------------------------------------------------------------------------------------------------------------------------

F VisibilityIndex::Enroll                  | VisibilityIndex.cpp     | 85-149  | - | - | ?
    in    Occupant     OccupantIdentity             [-]  ?
    in    Imported     const TopologyStructure&     [-]  ?
    in    Conditioned  const TopologyConditioning&  [-]  ?
    in    Resolutions  PartitionResolutionIndex&    [-]  ?
    out   -            Outcome<std::uint32_t>       [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE RESOLUTION
//------------------------------------------------------------------------------------------------------------------------

F VisibilityIndex::Resolve                 | VisibilityIndex.cpp     | 155-179 | - | - | ?
    in    Written      VisibilityWord                   [-]  ?
    in    Resolutions  const PartitionResolutionIndex&  [-]  ?
    out   -            Outcome<ResolvedPartition>       [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE COMPOSITION
//------------------------------------------------------------------------------------------------------------------------

F ComposeCoefficients                      | VisibilityRaster.cpp    | 23-41   | - | - | ?
    in    Outer  const ProjectedTransform&  [-]  ?
    in    Inner  const ProjectedTransform&  [-]  ?
    out   -      ProjectedTransform         [-]  ?

F ComposeVisibilityTransform               | VisibilityRaster.cpp    | 45-62   | - | - | ?
    in    Viewing       const ViewProjection&      [-]  ?
    in    Placement     const ProjectedTransform&  [-]  ?
    in    ObjectOrigin  DocumentPosition           [-]  ?
    out   -             ProjectedTransform         [-]  ?
    by    Api/VisibilityRaster.h

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE FAN
//------------------------------------------------------------------------------------------------------------------------

F VisibilityRaster::Fan                    | VisibilityRaster.cpp    | 188-264 | - | - | ?
    in    Enrolled       const PartitionStructure&               [-]  ?
    in    Imported       const TopologyStructure&                [-]  ?
    in    EnrolmentBase  std::uint32_t                           [-]  ?
    out   -              Outcome<std::vector<UploadedTriangle>>  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE STAGING
//------------------------------------------------------------------------------------------------------------------------

F VisibilityRaster::Stage                  | VisibilityRaster.cpp    | 270-317 | - | - | ?
    in    Arriving       const void*             [-]  ?
    in    ArrivingBytes  VkDeviceSize            [-]  ?
    in    Intent         SpanIntent              [-]  ?
    in    Recorded       VkCommandBuffer         [-]  ?
    out   -              Outcome<std::uint32_t>  [-]  ?

F VisibilityRaster::Abandon                | VisibilityRaster.cpp    | 319-337 | - | - | ?
    in    Abandoned  ResidentPartitioning&  [-]  ?
    out   -          void                   [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                 THE INDIRECT RECORDING
//------------------------------------------------------------------------------------------------------------------------

F VisibilityRaster::RecordIndirect         | VisibilityRaster.cpp    | 733-813 | - | - | ?
    in    Recorded      VkCommandBuffer            [-]  ?
    in    RotationSlot  std::uint32_t              [-]  ?
    in    Viewing       const ViewProjection&      [-]  ?
    in    Culling       const OcclusionScheduler&  [-]  ?
    in    Phase         CullingPhase               [-]  ?
    out   -             Outcome<bool>              [-]  ?
