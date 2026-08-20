//============================================================================================================================================
//                                                           TOOLSEQUENCE.CPP
//============================================================================================================================================
// 🧩 Tool declaration, the arbitration both units ask, and the capture that persists for a whole drag.

#include "SlateDocument/Document/ToolSequence/Api/ToolSequence.h"

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                                        THE TOOLS
//------------------------------------------------------------------------------------------------------------------------

Outcome<std::uint32_t> ToolIndex::Declare(const ToolSpecification& Declaring)
{
    if (Declaring.Identity.empty())
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::ContentUnsupported, "a tool declares no identity" });

    if (Declaring.Reserved  == PointerPrecedence::PrecedenceCount
     || Declaring.Previewed == PreviewSubject::PreviewCount
     || Declaring.Recorded  == TransactionSubject::SubjectCount)
    {
        return Outcome<std::uint32_t>::Refuse(
            { RefusalReason::ContentUnsupported, "no such precedence, preview or transaction shape" });
    }

    // 📝 A repeated identity is rejected rather than replacing the standing declaration. `Located` below resolves
    //    by identity, and two tools sharing one would make the resolution answer whichever was declared first —
    //    which is a tool the artist activates and does not get.
    for (const ToolSpecification& Held : Declared)
    {
        if (Held.Identity == Declaring.Identity)
        {
            return Outcome<std::uint32_t>::Refuse(
                { RefusalReason::ContentUnsupported, "a tool already declares that identity" });
        }
    }

    if (Declared.size() >= ToolCeiling)
        return Outcome<std::uint32_t>::Refuse({ RefusalReason::ExtentExhausted, "the tool ceiling was reached" });

    const std::uint32_t ToolOrdinal = static_cast<std::uint32_t>(Declared.size());

    Declared.push_back(Declaring);

    return Outcome<std::uint32_t>::Result(ToolOrdinal);
}

Outcome<const ToolSpecification*> ToolIndex::Resolve(std::uint32_t ToolOrdinal) const
{
    if (ToolOrdinal >= Declared.size())
        return Outcome<const ToolSpecification*>::Refuse({ RefusalReason::ContentUnsupported, "no such tool" });

    return Outcome<const ToolSpecification*>::Result(&Declared[ToolOrdinal]);
}

Outcome<ToolSpecification*> ToolIndex::Amend(std::uint32_t ToolOrdinal)
{
    if (ToolOrdinal >= Declared.size())
        return Outcome<ToolSpecification*>::Refuse({ RefusalReason::ContentUnsupported, "no such tool" });

    return Outcome<ToolSpecification*>::Result(&Declared[ToolOrdinal]);
}

Outcome<std::uint32_t> ToolIndex::Located(const std::string& Identity) const
{
    for (std::size_t Ordinal = 0u; Ordinal < Declared.size(); ++Ordinal)
    {
        if (Declared[Ordinal].Identity == Identity)
            return Outcome<std::uint32_t>::Result(static_cast<std::uint32_t>(Ordinal));
    }

    return Outcome<std::uint32_t>::Refuse({ RefusalReason::ContentUnsupported, "nothing declares that tool" });
}

std::uint32_t ToolIndex::DeclaredCount() const
{
    return static_cast<std::uint32_t>(Declared.size());
}

//------------------------------------------------------------------------------------------------------------------------
//                                                      THE DECLARATIONS
//------------------------------------------------------------------------------------------------------------------------

ToolIndex&        ToolSequence::Tools()         { return DeclaredTools;   }
const ToolIndex&  ToolSequence::Tools() const   { return DeclaredTools;   }
BrushIndex&       ToolSequence::Brushes()       { return DeclaredBrushes; }
const BrushIndex& ToolSequence::Brushes() const { return DeclaredBrushes; }

Outcome<bool> ToolSequence::DeclareTool(std::uint32_t ToolOrdinal_)
{
    if (!DeclaredTools.Resolve(ToolOrdinal_).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such tool" });

    ToolOrdinal = ToolOrdinal_;

    return Outcome<bool>::Result(true);
}

Outcome<bool> ToolSequence::DeclareBrush(std::uint32_t BrushOrdinal_)
{
    if (!DeclaredBrushes.Resolve(BrushOrdinal_).Resolved)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such brush" });

    BrushOrdinal = BrushOrdinal_;

    return Outcome<bool>::Result(true);
}

Outcome<bool> ToolSequence::DeclareColour(const ColourSpecification& Declaring)
{
    // 🔴 `36` §1: a colour without its space is rejected rather than assumed to be in the working space. An
    //    assumed space here is the defect `36` exists to prevent, placed where every stroke reads it.
    if (!Declaring.ColourDeclared())
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "the colour declares no space" });

    ActiveColour = Declaring;

    return Outcome<bool>::Result(true);
}

Outcome<bool> ToolSequence::DeclareDisplay(DisplaySubject Declaring)
{
    if (Declaring == DisplaySubject::DisplayCount)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such display mode" });

    CurrentDisplay = Declaring;

    return Outcome<bool>::Result(true);
}

Outcome<bool> ToolSequence::DeclareChannel(ChannelSubject Declaring)
{
    if (Declaring == ChannelSubject::ChannelCount)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such channel" });

    CurrentChannel = Declaring;

    return Outcome<bool>::Result(true);
}

Outcome<bool> ToolSequence::DeclareOverlay(OverlaySubject Declaring, bool PresenceEnabled)
{
    if (Declaring == OverlaySubject::OverlayCount)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such overlay" });

    OverlayPresent[static_cast<std::size_t>(Declaring)] = PresenceEnabled;

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                     POINTER ARBITRATION
//------------------------------------------------------------------------------------------------------------------------

PointerPrecedence ToolSequence::Arbitrate(bool InterfaceReported,
                                          bool ManipulatorOpen,
                                          bool StrokeOpen) const
{
    // 🔴 A standing capture answers unconditionally. `14` §4.2: capture persists for the whole drag, so a drag
    //    that began on a manipulator handle continues to address that handle after the cursor leaves the
    //    workspace, and a stroke is not stolen by a panel it passes under.
    if (CurrentCapture.CaptureDeclared)
        return CurrentCapture.Holder;

    if (InterfaceReported)
        return PointerPrecedence::Interface;

    if (ManipulatorOpen)
        return PointerPrecedence::Manipulator;

    if (StrokeOpen)
        return PointerPrecedence::Stroke;

    return PointerPrecedence::Workspace;
}

Outcome<bool> ToolSequence::OpenCapture(PointerPrecedence Precedence, const ResolvedPointer& Opened)
{
    if (Precedence == PointerPrecedence::PrecedenceCount)
        return Outcome<bool>::Refuse({ RefusalReason::ContentUnsupported, "no such precedence" });

    // 🔴 A stronger claimant does **not** steal a standing capture. Arbitration happens once, before capture is
    //    taken; re-arbitrating mid-drag is the defect where a stroke stops the moment the cursor crosses a
    //    floating panel, and it is exactly the case `14` §4.2 declares against.
    if (CurrentCapture.CaptureDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "a capture already stands" });

    CurrentCapture.Holder          = Precedence;
    CurrentCapture.Opened          = Opened;
    CurrentCapture.CaptureDeclared = true;

    return Outcome<bool>::Result(true);
}

Outcome<bool> ToolSequence::ReleaseCapture()
{
    if (!CurrentCapture.CaptureDeclared)
        return Outcome<bool>::Refuse({ RefusalReason::HostDenied, "no capture stands" });

    CurrentCapture = PointerCapture{};

    return Outcome<bool>::Result(true);
}

//------------------------------------------------------------------------------------------------------------------------
//                                                       WHAT IS READ
//------------------------------------------------------------------------------------------------------------------------

const ColourSpecification& ToolSequence::Colour() const  { return ActiveColour;    }
const PointerCapture&      ToolSequence::Capture() const { return CurrentCapture; }

Outcome<const ToolSpecification*> ToolSequence::ActiveTool() const
{
    if (ToolOrdinal == AbsentTool)
    {
        return Outcome<const ToolSpecification*>::Refuse(
            { RefusalReason::ContentUnsupported, "no tool is active" });
    }

    return DeclaredTools.Resolve(ToolOrdinal);
}

Outcome<const BrushSpecification*> ToolSequence::ActiveBrush() const
{
    if (BrushOrdinal == AbsentTool)
    {
        return Outcome<const BrushSpecification*>::Refuse(
            { RefusalReason::ContentUnsupported, "no brush is active" });
    }

    return DeclaredBrushes.Resolve(BrushOrdinal);
}

std::uint32_t  ToolSequence::ActiveToolOrdinal() const  { return ToolOrdinal;      }
std::uint32_t  ToolSequence::ActiveBrushOrdinal() const { return BrushOrdinal;     }
DisplaySubject ToolSequence::Display() const            { return CurrentDisplay; }
ChannelSubject ToolSequence::IsolatedChannel() const    { return CurrentChannel; }

bool ToolSequence::OverlayActive(OverlaySubject Subject) const
{
    return Subject != OverlaySubject::OverlayCount
        && OverlayPresent[static_cast<std::size_t>(Subject)];
}

}   // namespace Slate
