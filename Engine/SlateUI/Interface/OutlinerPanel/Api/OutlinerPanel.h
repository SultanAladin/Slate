//============================================================================================================================================
//                                                             OUTLINERPANEL.H
//============================================================================================================================================
// 🧩 Presents RowSequence through RankIndex and writes intent back — holding no relation of its own.

#pragma once

#include "SlateDocument/Document/OutlinerSequence/Api/OutlinerSequence.h"

#include <cstdint>
#include <string>

namespace Slate
{

//------------------------------------------------------------------------------------------------------------------------
//                                               THE SEARCH ENTRY EXTENT
//------------------------------------------------------------------------------------------------------------------------

// 📝 The sought text is held as a fixed extent rather than a growing string because the interface writes into
//    it directly every tick. A name longer than this narrows on its first sixty-three characters and is then
//    confirmed exactly against the whole name, so the extent bounds the entry and never the answer.
inline constexpr std::uint32_t NameSearchExtent = 64u;   // [-] - characters the search entry accepts, terminator included

//------------------------------------------------------------------------------------------------------------------------
//                                                  THE OUTLINER PANEL
//------------------------------------------------------------------------------------------------------------------------

/// 🧩 The presentation half of `12` — reads the linearisation, declares intent, stores neither relation.
/// note  🔴 `12` §7 and `14` §6: the rows are read through `RankIndex` and the relations are never read here.
///        Only the counted span the artist can see is touched, and the scroll position is a row ordinal
///        resolved by count rather than a pixel offset the panel remembers on its own.
/// note  🔴 Every mutation leaves through `OutlinerSequence::Declare` and is applied at the next tick's ①.
///        A panel that mutated the relations where the click arrived would apply against a linearisation that
///        is halfway rebuilt, and would bypass the sequence that undoes it.
/// note  ⚠️ What is held here is what `14` §4.1 places beside the document — the scroll position, the search
///        entry, and whether the panel is shown. None of it is a transaction and none of it is scrubbed.
/// tag   owning
class OutlinerPanel
{
public:

    /// 🧩 Presents one tick of the outliner and declares whatever the artist asked for.
    /// in    Outliner  [-]  the sequence to read from and declare into
    /// out   Outcome   [-]  refuses with HostDenied when no interface tick is open
    /// pre   InterfaceExchange::Advance delivered and Seal has not
    /// post  every declared intent sits in the pending run; nothing was applied here
    /// cost  🚩
    /// tag   api, nonthrowing
    Outcome<bool> Present(OutlinerSequence& Outliner);

    /// 🧩 Declares whether the panel is shown at all.
    /// in    PresenceDeclared  [-]  whether the artist wants it
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    void DeclarePresence(bool PresenceDeclared);

    /// 🧩 Whether the panel is shown.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    bool PresenceStanding() const;

    /// 🧩 The counted ordinal at the top of the presented span — the scroll position, as a row.
    /// note  Recorded as a count rather than as a pixel offset, and the occupant it names is held alongside it.
    ///        When the counted total changes the offset is restored from that occupant before it is read, so
    ///        collapsing an enclosure above the view leaves the artist looking at the same occupant rather than
    ///        at whatever slid under the cursor. An anchor whose occupant left the count keeps its ordinal.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t VisiblePosition() const;

    /// 🧩 The occupant the presented span is anchored on, undeclared when nothing is counted.
    /// note  🔴 `14` §4.1 state, not the document's. Held so the scroll survives a count adjustment; it is
    ///        never declared as intent and no transaction records it.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    OccupantIdentity Anchored() const;

    /// 🧩 How many rows the last presentation actually touched.
    /// note  🔍 The measurement `12` §7 is checked against: it stays proportional to the panel's height and
    ///        not to the population. A million occupants that presented a million rows is the defect.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t RowsTouched() const;

    /// 🧩 The text the artist is searching names for, empty when nothing is sought.
    /// cost  🚩
    /// tag   api, nonthrowing
    std::string Sought() const;

    /// 🧩 How many names the last narrowing confirmed.
    /// cost  ✔️
    /// tag   api, nonallocating, nonthrowing
    std::uint32_t ConfirmedNames() const;

private:

    std::uint32_t     VisibleAnchor                 = 0u;     // [-] - counted ordinal at the top of the span
    std::uint32_t     RowsPresented                 = 0u;     // [-] - rows the last presentation touched
    std::uint32_t     ConfirmedCount                = 0u;     // [-] - names the last narrowing confirmed
    std::uint32_t     CountedWhenAnchored           = 0u;     // [-] - counted total the anchor was observed at
    OccupantIdentity  AnchoredOccupant              = {};     // [-] - who the span is anchored on, not where
    bool              PresenceEnabled               = true;   // [-] - whether the panel is shown
    char              SoughtEntry[NameSearchExtent] = {};     // [-] - what the interface writes the search into
};

}   // namespace Slate
