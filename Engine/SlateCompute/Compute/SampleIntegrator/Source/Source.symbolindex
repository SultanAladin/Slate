//============================================================================================================================================
//                                                             SOURCE.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Motion-driven reprojection, the count-derived weight, and the reset that never decays.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/SampleIntegrator/Source
%layer      SlateCompute
%sources    1
%symbols    12
%annotated  0/12
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S SampleIntegrator.cpp | 197 lines | dcb0cb5e | 12 sym | Motion-driven reprojection, the count-derived weight, and the reset that never decays.

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE DECLARATION
//------------------------------------------------------------------------------------------------------------------------

V AccumulationRecordingIdentity     | SampleIntegrator.cpp | 18      | - | - | ?

F SampleIntegrator::Declare         | SampleIntegrator.cpp | 22-39   | - | - | ?
    in    Declaring  const RejectionSpecification&  [-]  ?
    out   -          Deliver<bool>                  [-]  ?

F SampleIntegrator::Contribute      | SampleIntegrator.cpp | 41-65   | - | - | ?
    in    Schedule  RenderSchedule&  [-]  ?
    out   -         Deliver<bool>    [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE OFFSET
//------------------------------------------------------------------------------------------------------------------------

F SampleIntegrator::OffsetOf        | SampleIntegrator.cpp | 71-77   | - | - | ?
    in    RotationOrdinal  std::uint64_t  [-]  ?
    in    OffsetX          double&        [-]  ?
    in    OffsetY          double&        [-]  ?
    out   -                void           [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE CLASSIFICATION
//------------------------------------------------------------------------------------------------------------------------

F SampleIntegrator::Classify        | SampleIntegrator.cpp | 83-105  | - | - | ?
    in    ReprojectedAlong   double            [-]  ?
    in    ReprojectedAcross  double            [-]  ?
    in    HeldOccupant       std::uint32_t     [-]  ?
    in    ArrivingOccupant   std::uint32_t     [-]  ?
    in    HeldDepth          double            [-]  ?
    in    ArrivingDepth      double            [-]  ?
    out   -                  RejectionSubject  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                    THE ACCUMULATION
//------------------------------------------------------------------------------------------------------------------------

F SampleIntegrator::Accumulate      | SampleIntegrator.cpp | 111-149 | - | - | ?
    in    Held      AccumulatedSample&  [-]  ?
    in    Arriving  const double        [-]  ?
    in    Refused   RejectionSubject    [-]  ?
    in    Least     const double        [-]  ?
    in    Greatest  const double        [-]  ?
    out   -         void                [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                   THE INVALIDATIONS
//------------------------------------------------------------------------------------------------------------------------

F SampleIntegrator::Invalidate      | SampleIntegrator.cpp | 155-161 | - | - | ?
    out   -  void  [-]  ?

F SampleIntegrator::HistoryReadable | SampleIntegrator.cpp | 163     | - | - | ?
    out   -  bool  [-]  ?

//------------------------------------------------------------------------------------------------------------------------
//                                                     THE REPORTING
//------------------------------------------------------------------------------------------------------------------------

F SampleIntegrator::DeclareRotation | SampleIntegrator.cpp | 169-184 | - | - | ?
    in    LeastSampleCount     std::uint32_t  [-]  ?
    in    GreatestSampleCount  std::uint32_t  [-]  ?
    in    RejectedCount        std::uint32_t  [-]  ?
    in    AccumulatedCount     std::uint32_t  [-]  ?
    out   -                    void           [-]  ?

F SampleIntegrator::Report          | SampleIntegrator.cpp | 186-192 | - | - | ?
    in    Measured  MeasureIndex&  [-]  ?
    in    Sampled   TickPoint      [-]  ?
    out   -         void           [-]  ?

F SampleIntegrator::Declared        | SampleIntegrator.cpp | 194     | - | - | ?
    out   -  const RejectionSpecification&  [-]  ?

F SampleIntegrator::Metrics         | SampleIntegrator.cpp | 195     | - | - | ?
    out   -  const ConvergenceMetrics&  [-]  ?
