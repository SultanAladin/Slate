//============================================================================================================================================
//                                                              API.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Proves the host form and the shader form of a Shared/ entry point agree at the declared guarantee.

%format     symbolindex 1.0
%scope      folder
%path       Engine/SlateCompute/Compute/ParityRunner/Api
%layer      SlateCompute
%sources    1
%symbols    6
%annotated  6/6
%cost       ✔️ low · 🚩 medium · 🔴 high (cost rises left to right)

//------------------------------------------------------------------------------------------------------------------------
//                                                        SOURCES
//------------------------------------------------------------------------------------------------------------------------

S ParityRunner.h | 87 lines | bece8957 | 6 sym | Proves the host form and the shader form of a Shared/ entry point agree at the declared guarantee.

//------------------------------------------------------------------------------------------------------------------------
//                                                      REGISTRATION
//------------------------------------------------------------------------------------------------------------------------

T ParityRegistration          | ParityRunner.h | 24-29 | owning                        | -  | One registered `Shared/` entry point and the guarantee it claims.
    has   EntryName    const char*         [-]  ?
    has   Claimed      PrecisionGuarantee  [-]  ?
    has   SampleCount  std::uint32_t       [-]  ?
    by    Source/ConsoleHost.cpp, Source/ParityRunner.cpp
    note  An entry point in `Shared/` with no registration is duplicated source that has not diverged yet.

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE REPORT
//------------------------------------------------------------------------------------------------------------------------

T ParityReport                | ParityRunner.h | 37-44 | owning                        | -  | What one entry point's parity comparison found.
    has   EntryName         const char*    [-]  ?
    has   SampleCount       std::uint32_t  [-]  ?
    has   DisagreeingCount  std::uint32_t  [-]  ?
    has   LargestDeviation  double         [-]  ?
    has   AgreementHeld     bool           [-]  ?
    by    Source/ConsoleHost.cpp, Source/ParityRunner.cpp

//------------------------------------------------------------------------------------------------------------------------
//                                                       THE RUNNER
//------------------------------------------------------------------------------------------------------------------------

T ParityRunner                | ParityRunner.h | 55-85 | owning                        | -  | Classifies registered entry points on both toolchains over a common sample set. declared bound in units in the last place. Convergent entry points are compared within their own convergence criterion. Perceptual entry points are not compared — that is what Perceptual means.
    has   Registered         std::vector<ParityRegistration>  [-]  ?
    has   Reported           std::vector<ParityReport>        [-]  ?
    has   AgreementDeclared  bool                             [-]  ?
    by    Source/ConsoleHost.cpp, Source/ParityRunner.cpp
    note  🔴 Exact entry points are compared bit for bit. Bounded entry points are compared against a

F ParityRunner::Register      | ParityRunner.h | 64    | api,nonthrowing               | ✔️ | Registers one `Shared/` entry point for comparison.
    in    Arriving  const ParityRegistration&  [-]  the entry point and the guarantee it claims
    out   -         Outcome                    [-]  refuses when the entry point is already registered
    by    Api/DiagnosticExtension.h, Source/ConsoleHost.cpp, Source/DiagnosticExtension.cpp, Source/ParityRunner.cpp

F ParityRunner::Compare       | ParityRunner.h | 73    | api,nonthrowing               | 🔴 | Compares every registered entry point and reports each one. Until it is, the runner compares the host form against itself and reports the sample counts, which is honest about what was proven rather than reporting an agreement nothing established.
    out   -  Reports  [-]  one report per registration, in registration order
    by    Source/ConsoleHost.cpp, Source/ParityRunner.cpp
    note  ⏱️ 🚧 The shader-side comparison requires a device and is contributed by `06`'s bring-up.

F ParityRunner::AgreementHeld | ParityRunner.h | 78    | api,nonallocating,nonthrowing | ✔️ | Whether every registered entry point met its claimed guarantee in the last comparison.
    out   -  bool  [-]  ?
    by    Source/ConsoleHost.cpp, Source/ParityRunner.cpp
