//============================================================================================================================================
//                                                           WORKSPACECODEX.H
//============================================================================================================================================
// 🧩 Typed workspace, environment, scene-entry, material-reference, and embedded-Codex section translation.

#pragma once

#include "Foundation/DeliveryOutcome.h"
#include "SlateDocument/Format/CodexInterchange/Api/CodexInterchange.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Slate
{

/// 🧩 Which real workspace entry a Codex scene declaration represents.
enum class CodexSceneSubject : std::uint32_t
{
    Sun         = 0u,
    Sky         = 1u,
    Atmosphere  = 2u,
    Geometry    = 3u
};

/// 🧩 One named scene entry, its authored placement, and its assigned material reference.
struct CodexSceneEntry
{
    CodexSceneSubject  Subject             = CodexSceneSubject::Geometry;   // [-]
    std::string        Naming              = {};                            // [-] - persisted artist-visible naming
    std::string        GeometryReference   = {};                            // [-] - section-local geometry identity
    std::string        MaterialReference   = {};                            // [-] - section-local material identity
    double             Position[3]         = {};                            // [m] - authored world placement
    double             Rotation[3]         = {};                            // [deg] - yaw, pitch, roll
    double             Scale[3]            = { 1.0, 1.0, 1.0 };             // [-] - per-axis authored scale
};

/// 🧩 The environment figures a workspace persists beside its scene entries.
struct CodexEnvironment
{
    double         SunElevation          = 35.0;    // [deg]
    double         SunAzimuth            = 120.0;   // [deg]
    double         SunIntensity          = 4.8;     // [lx]
    double         SunTemperature        = 5500.0;  // [K]
    double         SkyIntensity          = 1.0;     // [-]
    double         AtmosphereDensity     = 1.0;     // [-]
    double         AtmosphereScaleHeight = 1.0;     // [-]
};

/// 🧩 A complete workspace payload carried by a WorkspaceCodex document.
struct WorkspaceCodex
{
    std::string                     Naming       = {};   // [-] - document title
    CodexEnvironment                Environment  = {};   // [-] - Sun, Sky, and Atmosphere declaration
    std::vector<CodexSceneEntry>    Scene        = {};   // [-] - only persisted scene entries
    std::vector<CodexDocument>      Embedded     = {};   // [B] - whole specialized Codex documents
};

/// 🧩 Translates typed workspace content to and from preserved Codex sections.
class WorkspaceCodexInterchange
{
public:

    /// 🧩 Adds typed workspace sections to a WorkspaceCodex document.
    /// in    Workspace [-]  typed workspace content
    /// in    Identity  [-]  stable document identity
    /// in    Revision  [-]  committed revision introducing the sections
    /// out   Result    [-]  complete WorkspaceCodex document
    Outcome<CodexDocument> EncodeWorkspace(const WorkspaceCodex& Workspace,
                                           std::uint64_t          Identity,
                                           std::uint64_t          Revision) const;

    /// 🧩 Resolves typed workspace sections from a WorkspaceCodex document while leaving unknown sections retained.
    /// in    Document [-]  decoded WorkspaceCodex document
    /// out   Result   [-]  typed workspace content
    Outcome<WorkspaceCodex> DecodeWorkspace(const CodexDocument& Document) const;
};

}   // namespace Slate
