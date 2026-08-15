# Engine Folder Structure

Source: `Engine/` — 246 source files (`.h` / `.cpp` / `.slang` / `.slang.h`) across 6 modules.

```
Engine/
├── Application/
│   ├── Module.toml
│   ├── ConsoleHost/
│   │   └── Source/
│   │       └── ConsoleHost.cpp
│   ├── EditorHost/
│   │   └── Source/
│   │       └── EditorHost.cpp
│   └── PaintHost/
│       └── Source/
│           └── PaintHost.cpp
├── Contract/
│   ├── CombineContract.h
│   ├── IdentityContract.h
│   ├── OutcomeContract.h
│   ├── PrecisionContract.h
│   ├── ToleranceContract.h
│   └── ToolchainContract.h
├── Shared/
│   ├── AccumulationProjection.slang.h
│   ├── AtmosphereProjection.slang.h
│   ├── ContainmentClassifier.slang.h
│   ├── IncircleClassifier.slang.h
│   ├── IntersectionClassifier.slang.h
│   ├── LatticeProjection.slang.h
│   ├── OcclusionProjection.slang.h
│   ├── OrientationClassifier.slang.h
│   ├── PlanarClassifier.slang.h
│   ├── Prelude.slang.h
│   ├── ReflectanceProjection.slang.h
│   ├── ReflectionProjection.slang.h
│   ├── SampleProjection.slang.h
│   ├── ToneProjection.slang.h
│   └── TransmissionProjection.slang.h
├── SlateCompute/
│   └── Compute/
│       ├── AnalyticProjection/
│       │   ├── Api/
│       │   │   └── AnalyticProjection.h
│       │   └── Source/
│       │       └── AnalyticProjection.cpp
│       ├── AtmosphereIntegrator/
│       │   ├── Api/
│       │   │   └── AtmosphereIntegrator.h
│       │   ├── Shader/
│       │   │   ├── AtmosphereUniform.slang
│       │   │   ├── MultiScatterSurface.slang
│       │   │   ├── SkyRadiance.slang
│       │   │   ├── SkyViewSurface.slang
│       │   │   └── TransmittanceSurface.slang
│       │   └── Source/
│       │       └── AtmosphereIntegrator.cpp
│       ├── ChartPartition/
│       │   ├── Api/
│       │   │   └── ChartPartition.h
│       │   └── Source/
│       │       └── ChartPartition.cpp
│       ├── DisplayProjection/
│       │   ├── Api/
│       │   │   └── DisplayProjection.h
│       │   └── Source/
│       │       └── DisplayProjection.cpp
│       ├── DomainSpace/
│       │   ├── Api/
│       │   │   └── DomainSpace.h
│       │   └── Source/
│       │       └── DomainSpace.cpp
│       ├── EmissionSequence/
│       │   ├── Api/
│       │   │   └── EmissionSequence.h
│       │   └── Source/
│       │       └── EmissionSequence.cpp
│       ├── ImpressionSequence/
│       │   ├── Api/
│       │   │   └── ImpressionSequence.h
│       │   └── Source/
│       │       └── ImpressionSequence.cpp
│       ├── IntersectionOutline/
│       │   ├── Api/
│       │   │   └── IntersectionOutline.h
│       │   └── Source/
│       │       └── IntersectionOutline.cpp
│       ├── OcclusionProjection/
│       │   ├── Api/
│       │   │   └── OcclusionProjection.h
│       │   └── Source/
│       │       └── OcclusionProjection.cpp
│       ├── OverlayProjection/
│       │   ├── Api/
│       │   │   └── OverlayProjection.h
│       │   └── Source/
│       │       └── OverlayProjection.cpp
│       ├── ParityRunner/
│       │   ├── Api/
│       │   │   └── ParityRunner.h
│       │   └── Source/
│       │       └── ParityRunner.cpp
│       ├── PreviewProjection/
│       │   ├── Api/
│       │   │   └── PreviewProjection.h
│       │   └── Source/
│       │       └── PreviewProjection.cpp
│       ├── PromotionScheduler/
│       │   ├── Api/
│       │   │   └── PromotionScheduler.h
│       │   └── Source/
│       │       └── PromotionScheduler.cpp
│       ├── ReflectanceIntegrator/
│       │   ├── Api/
│       │   │   └── ReflectanceIntegrator.h
│       │   └── Source/
│       │       └── ReflectanceIntegrator.cpp
│       ├── RequestQueue/
│       │   ├── Api/
│       │   │   └── RequestQueue.h
│       │   └── Source/
│       │       └── RequestQueue.cpp
│       ├── SampleIntegrator/
│       │   ├── Api/
│       │   │   └── SampleIntegrator.h
│       │   └── Source/
│       │       └── SampleIntegrator.cpp
│       ├── SeamSpecification/
│       │   ├── Api/
│       │   │   └── SeamSpecification.h
│       │   └── Source/
│       │       └── SeamSpecification.cpp
│       ├── SpecularProjection/
│       │   ├── Api/
│       │   │   └── SpecularProjection.h
│       │   └── Source/
│       │       └── SpecularProjection.cpp
│       ├── StrokeSpace/
│       │   ├── Api/
│       │   │   └── StrokeSpace.h
│       │   └── Source/
│       │       └── StrokeSpace.cpp
│       ├── SurfaceDepot/
│       │   ├── Api/
│       │   │   └── SurfaceDepot.h
│       │   └── Source/
│       │       └── SurfaceDepot.cpp
│       ├── SurfaceTileSpace/
│       │   ├── Api/
│       │   │   └── SurfaceTileSpace.h
│       │   └── Source/
│       │       └── SurfaceTileSpace.cpp
│       ├── TileSpace/
│       │   ├── Api/
│       │   │   └── TileSpace.h
│       │   └── Source/
│       │       └── TileSpace.cpp
│       ├── TransmissionSequence/
│       │   ├── Api/
│       │   │   └── TransmissionSequence.h
│       │   └── Source/
│       │       └── TransmissionSequence.cpp
│       ├── UvSurfaceDepot/
│       │   ├── Api/
│       │   │   └── UvSurfaceDepot.h
│       │   └── Source/
│       │       └── UvSurfaceDepot.cpp
│       └── VisibilityIndex/
│           ├── Api/
│           │   ├── DepthReduction.h
│           │   ├── OcclusionScheduler.h
│           │   ├── PartitionClassifier.h
│           │   ├── PartitionStructure.h
│           │   ├── VisibilityIndex.h
│           │   └── VisibilityRaster.h
│           ├── Shader/
│           │   ├── DepthReduction.slang
│           │   ├── OcclusionCulling.slang
│           │   ├── OcclusionUniform.slang
│           │   ├── VisibilityCorner.slang
│           │   ├── VisibilitySurface.slang
│           │   └── VisibilityUniform.slang
│           └── Source/
│               ├── DepthReduction.cpp
│               ├── OcclusionScheduler.cpp
│               ├── PartitionClassifier.cpp
│               ├── PartitionStructure.cpp
│               ├── VisibilityIndex.cpp
│               └── VisibilityRaster.cpp
├── SlateDocument/
│   ├── Document/
│   │   ├── AssetInterchange/
│   │   │   ├── Api/
│   │   │   │   └── AssetInterchange.h
│   │   │   └── Source/
│   │   │       └── AssetInterchange.cpp
│   │   ├── BrushSpecification/
│   │   │   ├── Api/
│   │   │   │   └── BrushSpecification.h
│   │   │   └── Source/
│   │   │       └── BrushSpecification.cpp
│   │   ├── CameraProjection/
│   │   │   ├── Api/
│   │   │   │   └── CameraProjection.h
│   │   │   └── Source/
│   │   │       └── CameraProjection.cpp
│   │   ├── DecalProjection/
│   │   │   ├── Api/
│   │   │   │   └── DecalProjection.h
│   │   │   └── Source/
│   │   │       └── DecalProjection.cpp
│   │   ├── DocumentSession/
│   │   │   ├── Api/
│   │   │   │   └── DocumentSession.h
│   │   │   └── Source/
│   │   │       └── DocumentSession.cpp
│   │   ├── EnrollmentIndex/
│   │   │   ├── Api/
│   │   │   │   └── EnrollmentIndex.h
│   │   │   └── Source/
│   │   │       └── EnrollmentIndex.cpp
│   │   ├── IlluminantPopulation/
│   │   │   ├── Api/
│   │   │   │   └── IlluminantPopulation.h
│   │   │   └── Source/
│   │   │       └── IlluminantPopulation.cpp
│   │   ├── IntakeIndex/
│   │   │   ├── Api/
│   │   │   │   └── IntakeIndex.h
│   │   │   └── Source/
│   │   │       └── IntakeIndex.cpp
│   │   ├── MaterialSpecification/
│   │   │   ├── Api/
│   │   │   │   └── MaterialSpecification.h
│   │   │   └── Source/
│   │   │       └── MaterialSpecification.cpp
│   │   ├── OutlinerSequence/
│   │   │   ├── Api/
│   │   │   │   └── OutlinerSequence.h
│   │   │   └── Source/
│   │   │       └── OutlinerSequence.cpp
│   │   ├── PersistenceSequence/
│   │   │   ├── Api/
│   │   │   │   └── PersistenceSequence.h
│   │   │   └── Source/
│   │   │       └── PersistenceSequence.cpp
│   │   ├── PointerIntersection/
│   │   │   ├── Api/
│   │   │   │   └── PointerIntersection.h
│   │   │   └── Source/
│   │   │       └── PointerIntersection.cpp
│   │   ├── PopulationIndex/
│   │   │   ├── Api/
│   │   │   │   └── PopulationIndex.h
│   │   │   └── Source/
│   │   │       └── PopulationIndex.cpp
│   │   ├── PrimitiveStructure/
│   │   │   ├── Api/
│   │   │   │   └── PrimitiveStructure.h
│   │   │   └── Source/
│   │   │       └── PrimitiveStructure.cpp
│   │   ├── PropertySpecification/
│   │   │   ├── Api/
│   │   │   │   └── PropertySpecification.h
│   │   │   └── Source/
│   │   │       └── PropertySpecification.cpp
│   │   ├── RecoverySequence/
│   │   │   ├── Api/
│   │   │   │   └── RecoverySequence.h
│   │   │   └── Source/
│   │   │       └── RecoverySequence.cpp
│   │   ├── ReferenceIndex/
│   │   │   ├── Api/
│   │   │   │   └── ReferenceIndex.h
│   │   │   └── Source/
│   │   │       └── ReferenceIndex.cpp
│   │   ├── RevisionSequence/
│   │   │   ├── Api/
│   │   │   │   └── RevisionSequence.h
│   │   │   └── Source/
│   │   │       └── RevisionSequence.cpp
│   │   ├── RowSequence/
│   │   │   ├── Api/
│   │   │   │   └── RowSequence.h
│   │   │   └── Source/
│   │   │       └── RowSequence.cpp
│   │   ├── SceneStructure/
│   │   │   ├── Api/
│   │   │   │   └── SceneStructure.h
│   │   │   └── Source/
│   │   │       └── SceneStructure.cpp
│   │   ├── SelectionSequence/
│   │   │   ├── Api/
│   │   │   │   └── SelectionSequence.h
│   │   │   └── Source/
│   │   │       └── SelectionSequence.cpp
│   │   ├── SpatialManipulator/
│   │   │   ├── Api/
│   │   │   │   └── SpatialManipulator.h
│   │   │   └── Source/
│   │   │       └── SpatialManipulator.cpp
│   │   ├── SpatialSubdivision/
│   │   │   ├── Api/
│   │   │   │   └── SpatialSubdivision.h
│   │   │   └── Source/
│   │   │       └── SpatialSubdivision.cpp
│   │   ├── SurfaceLayerSequence/
│   │   │   ├── Api/
│   │   │   │   └── SurfaceLayerSequence.h
│   │   │   └── Source/
│   │   │       └── SurfaceLayerSequence.cpp
│   │   ├── TilingSpecification/
│   │   │   ├── Api/
│   │   │   │   └── TilingSpecification.h
│   │   │   └── Source/
│   │   │       └── TilingSpecification.cpp
│   │   ├── ToolSequence/
│   │   │   ├── Api/
│   │   │   │   └── ToolSequence.h
│   │   │   └── Source/
│   │   │       └── ToolSequence.cpp
│   │   ├── TopologyConditioning/
│   │   │   ├── Api/
│   │   │   │   └── TopologyConditioning.h
│   │   │   └── Source/
│   │   │       └── TopologyConditioning.cpp
│   │   ├── TopologyStructure/
│   │   │   ├── Api/
│   │   │   │   └── TopologyStructure.h
│   │   │   └── Source/
│   │   │       └── TopologyStructure.cpp
│   │   ├── TrigramIndex/
│   │   │   ├── Api/
│   │   │   │   └── TrigramIndex.h
│   │   │   └── Source/
│   │   │       └── TrigramIndex.cpp
│   │   └── VectorInterchange/
│   │       ├── Api/
│   │       │   └── VectorInterchange.h
│   │       └── Source/
│   │           └── VectorInterchange.cpp
│   └── Format/
│       ├── FormatCodec/
│       │   ├── Api/
│       │   │   └── FormatCodec.h
│       │   └── Source/
│       │       └── FormatCodec.cpp
│       ├── ImageCodec/
│       │   ├── Api/
│       │   │   └── ImageCodec.h
│       │   └── Source/
│       │       └── ImageCodec.cpp
│       ├── TopologyCodec/
│       │   ├── Api/
│       │   │   └── TopologyCodec.h
│       │   └── Source/
│       │       └── TopologyCodec.cpp
│       ├── TypefaceCodec/
│       │   ├── Api/
│       │   │   └── TypefaceCodec.h
│       │   └── Source/
│       │       └── TypefaceCodec.cpp
│       └── VectorCodec/
│           ├── Api/
│           │   └── VectorCodec.h
│           └── Source/
│               └── VectorCodec.cpp
├── SlateMath/
│   ├── Numeric/
│   │   ├── ColourProjection/
│   │   │   ├── Api/
│   │   │   │   └── ColourProjection.h
│   │   │   └── Source/
│   │   │       └── ColourProjection.cpp
│   │   ├── CurveSolver/
│   │   │   ├── Api/
│   │   │   │   └── CurveSolver.h
│   │   │   └── Source/
│   │   │       └── CurveSolver.cpp
│   │   ├── LinearSolver/
│   │   │   ├── Api/
│   │   │   │   └── LinearSolver.h
│   │   │   └── Source/
│   │   │       └── LinearSolver.cpp
│   │   ├── QuadratureIntegrator/
│   │   │   ├── Api/
│   │   │   │   └── QuadratureIntegrator.h
│   │   │   └── Source/
│   │   │       └── QuadratureIntegrator.cpp
│   │   ├── ReportSequence/
│   │   │   ├── Api/
│   │   │   │   └── ReportSequence.h
│   │   │   └── Source/
│   │   │       └── ReportSequence.cpp
│   │   ├── SpectralProjection/
│   │   │   ├── Api/
│   │   │   │   └── SpectralProjection.h
│   │   │   └── Source/
│   │   │       └── SpectralProjection.cpp
│   │   ├── TransformProjection/
│   │   │   ├── Api/
│   │   │   │   └── TransformProjection.h
│   │   │   └── Source/
│   │   │       └── TransformProjection.cpp
│   │   ├── UnwrapSolver/
│   │   │   ├── Api/
│   │   │   │   └── UnwrapSolver.h
│   │   │   └── Source/
│   │   │       └── UnwrapSolver.cpp
│   │   └── WorkSequence/
│   │       ├── Api/
│   │       │   └── WorkSequence.h
│   │       └── Source/
│   │           └── WorkSequence.cpp
│   └── Platform/
│       ├── ClipboardExchange/
│       │   ├── Api/
│       │   │   └── ClipboardExchange.h
│       │   └── Source/
│       │       └── ClipboardExchange.cpp
│       ├── CodeInterchange/
│       │   ├── Api/
│       │   │   └── CodeInterchange.h
│       │   └── Source/
│       │       └── CodeInterchange.cpp
│       ├── FileInterchange/
│       │   ├── Api/
│       │   │   └── FileInterchange.h
│       │   └── Source/
│       │       └── FileInterchange.cpp
│       ├── InputExchange/
│       │   ├── Api/
│       │   │   └── InputExchange.h
│       │   └── Source/
│       │       └── InputExchange.cpp
│       ├── InstructionExchange/
│       │   ├── Api/
│       │   │   └── InstructionExchange.h
│       │   └── Source/
│       │       └── InstructionExchange.cpp
│       ├── PlatformInterchange/
│       │   ├── Api/
│       │   │   └── PlatformInterchange.h
│       │   └── Source/
│       │       └── PlatformInterchange.cpp
│       ├── StorageExchange/
│       │   ├── Api/
│       │   │   └── StorageExchange.h
│       │   └── Source/
│       │       └── StorageExchange.cpp
│       ├── TickSequence/
│       │   ├── Api/
│       │   │   └── TickSequence.h
│       │   └── Source/
│       │       └── TickSequence.cpp
│       └── WindowInterchange/
│           ├── Api/
│           │   └── WindowInterchange.h
│           └── Source/
│               └── WindowInterchange.cpp
├── SlateUI/
│   └── Interface/
│       ├── AppearanceSpecification/
│       │   ├── Api/
│       │   │   └── AppearanceSpecification.h
│       │   └── Source/
│       │       └── AppearanceSpecification.cpp
│       ├── DrawerSpace/
│       │   ├── Api/
│       │   │   └── DrawerSpace.h
│       │   └── Source/
│       │       └── DrawerSpace.cpp
│       ├── InterfaceExchange/
│       │   ├── Api/
│       │   │   ├── InterfaceExchange.h
│       │   │   └── RecordingSurface.h
│       │   └── Source/
│       │       ├── InterfaceExchange.cpp
│       │       └── RecordingSurface.cpp
│       ├── MotionIntegrator/
│       │   └── Api/
│       │       └── MotionIntegrator.h
│       ├── RedrawScheduler/
│       │   ├── Api/
│       │   │   └── RedrawScheduler.h
│       │   └── Source/
│       │       └── RedrawScheduler.cpp
│       └── SymbolSpecification/
│           ├── Api/
│           │   └── SymbolSpecification.h
│           └── Source/
│               └── SymbolSpecification.cpp
└── SlateVulkan/
    └── Device/
        ├── AttachmentIndex/
        │   ├── Api/
        │   │   └── AttachmentIndex.h
        │   └── Source/
        │       └── AttachmentIndex.cpp
        ├── ByteSpace/
        │   ├── Api/
        │   │   └── ByteSpace.h
        │   └── Source/
        │       └── ByteSpace.cpp
        ├── CommandSequence/
        │   ├── Api/
        │   │   └── CommandSequence.h
        │   └── Source/
        │       └── CommandSequence.cpp
        ├── CycleScheduler/
        │   ├── Api/
        │   │   └── CycleScheduler.h
        │   └── Source/
        │       └── CycleScheduler.cpp
        ├── DescriptorIndex/
        │   ├── Api/
        │   │   └── DescriptorIndex.h
        │   └── Source/
        │       └── DescriptorIndex.cpp
        ├── DiagnosticExtension/
        │   ├── Api/
        │   │   └── DiagnosticExtension.h
        │   └── Source/
        │       └── DiagnosticExtension.cpp
        ├── DisplayScheduler/
        │   ├── Api/
        │   │   └── DisplayScheduler.h
        │   └── Source/
        │       └── DisplayScheduler.cpp
        ├── HardwareMetrics/
        │   ├── Api/
        │   │   └── HardwareMetrics.h
        │   └── Source/
        │       └── HardwareMetrics.cpp
        ├── ImageSpace/
        │   ├── Api/
        │   │   └── ImageSpace.h
        │   └── Source/
        │       └── ImageSpace.cpp
        ├── ProgramIndex/
        │   ├── Api/
        │   │   └── ProgramIndex.h
        │   └── Source/
        │       └── ProgramIndex.cpp
        ├── RenderSchedule/
        │   ├── Api/
        │   │   └── RenderSchedule.h
        │   └── Source/
        │       └── RenderSchedule.cpp
        ├── ShaderCodec/
        │   ├── Api/
        │   │   └── ShaderCodec.h
        │   └── Source/
        │       └── ShaderCodec.cpp
        ├── SpanSpace/
        │   ├── Api/
        │   │   └── SpanSpace.h
        │   └── Source/
        │       └── SpanSpace.cpp
        ├── VendorClassifier/
        │   ├── Api/
        │   │   └── VendorClassifier.h
        │   └── Source/
        │       └── VendorClassifier.cpp
        ├── VulkanExchange/
        │   ├── Api/
        │   │   └── VulkanExchange.h
        │   └── Source/
        │       └── VulkanExchange.cpp
        └── WindowExchange/
            ├── Api/
            │   └── WindowExchange.h
            └── Source/
                └── WindowExchange.cpp
```
