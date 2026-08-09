//============================================================================================================================================
//                                                            SLATEUI.SYMBOLINDEX
//============================================================================================================================================
// 🧩 Symbol roll for SlateUI — The one seam the interface library crosses — device handles in, recorded commands out, no ImGui spelling.

%format   symbolindex 1.0
%scope    layer
%path     Engine/SlateUI
%folders  4
%symbols  44

//------------------------------------------------------------------------------------------------------------------------
//                                                     FOLDER INDEXES
//------------------------------------------------------------------------------------------------------------------------

I Api    | Api/Api.symbolindex       | 10 sym | The one seam the interface library crosses — device handles in, recorded commands out, no ImGui spelling.
I Source | Source/Source.symbolindex | 9 sym  | The only translation unit in the engine that includes ImGui.
I Api    | Api/Api.symbolindex       | 10 sym | Presents RowSequence through RankIndex and writes intent back — holding no relation of its own.
I Source | Source/Source.symbolindex | 15 sym | The counted span presented, and every gesture over it turned into a declared intent.

//------------------------------------------------------------------------------------------------------------------------
//                                                        SYMBOLS
//------------------------------------------------------------------------------------------------------------------------

T InterfaceAttachment                   | Api/InterfaceExchange.h      | 28-38   | Every device handle the interface library needs, supplied once at bring-up. is an ImGui spelling: the whole point of the seam is that a host including this header links the interface without acquiring ImGui's declarations. `00` §2.2 makes a host that includes `imgui.h` a defect, and a defect that cannot be spelled cannot be committed.
T InterfaceExchange                     | Api/InterfaceExchange.h      | 50-114  | Holds the interface context and the two vendor attachments that feed it. source file. `00` §2.2: exactly one copy of ImGui exists, compiled inside `SlateUI`. projection, so nothing recorded by this component is ever tone-mapped a second time.
F InterfaceExchange::~InterfaceExchange | Api/InterfaceExchange.h      | 57      | ?
F InterfaceExchange::Construct          | Api/InterfaceExchange.h      | 68      | Constructs the interface context over the supplied device handles. HostDenied when the vendor attachment declines `VK_KHR_dynamic_rendering` or a device at Vulkan 1.3. Construct refuses rather than recording into a target the device never agreed to.
F InterfaceExchange::Reclaim            | Api/InterfaceExchange.h      | 73      | Destroys the interface context and both vendor attachments.
F InterfaceExchange::Advance            | Api/InterfaceExchange.h      | 79      | Opens one interface tick and reads the window system's accumulated condition.
F InterfaceExchange::Seal               | Api/InterfaceExchange.h      | 86      | Closes the open tick and assembles its command content, ready to record.
F InterfaceExchange::Record             | Api/InterfaceExchange.h      | 94      | Records the assembled content into a command recording of the current rotation slot.
F InterfaceExchange::PointerCaptured    | Api/InterfaceExchange.h      | 99      | Whether the interface has taken the pointer, so that `22` must not treat it as a canvas stroke.
F InterfaceExchange::KeyboardCaptured   | Api/InterfaceExchange.h      | 104     | Whether the interface has taken text entry, so that no shortcut consumes the same key.
V InterfaceDescriptorCapacity           | Source/InterfaceExchange.cpp | 24      | ?
F InterfaceExchange::~InterfaceExchange | Source/InterfaceExchange.cpp | 31-34   | ?
F InterfaceExchange::Construct          | Source/InterfaceExchange.cpp | 36-128  | ?
F InterfaceExchange::Reclaim            | Source/InterfaceExchange.cpp | 130-154 | ?
F InterfaceExchange::Advance            | Source/InterfaceExchange.cpp | 160-178 | ?
F InterfaceExchange::Seal               | Source/InterfaceExchange.cpp | 180-192 | ?
F InterfaceExchange::Record             | Source/InterfaceExchange.cpp | 198-222 | ?
F InterfaceExchange::PointerCaptured    | Source/InterfaceExchange.cpp | 231-239 | ?
F InterfaceExchange::KeyboardCaptured   | Source/InterfaceExchange.cpp | 241-249 | ?
V NameSearchExtent                      | Api/OutlinerPanel.h          | 23      | ?
T OutlinerPanel                         | Api/OutlinerPanel.h          | 39-105  | The presentation half of `12` — reads the linearisation, declares intent, stores neither relation. Only the counted span the artist can see is touched, and the scroll position is a row ordinal resolved by count rather than a pixel offset the panel remembers on its own. A panel that mutated the relations where the click arrived would apply against a linearisation that is halfway rebuilt, and would bypass the sequence that undoes it. entry, and whether the panel is shown. None of it is a transaction and none of it is scrubbed.
F OutlinerPanel::Present                | Api/OutlinerPanel.h          | 50      | Presents one tick of the outliner and declares whatever the artist asked for.
F OutlinerPanel::DeclarePresence        | Api/OutlinerPanel.h          | 56      | Declares whether the panel is shown at all.
F OutlinerPanel::PresenceStanding       | Api/OutlinerPanel.h          | 61      | Whether the panel is shown.
F OutlinerPanel::VisiblePosition        | Api/OutlinerPanel.h          | 70      | The counted ordinal at the top of the presented span — the scroll position, as a row. When the counted total changes the offset is restored from that occupant before it is read, so collapsing an enclosure above the view leaves the artist looking at the same occupant rather than at whatever slid under the cursor. An anchor whose occupant left the count keeps its ordinal.
F OutlinerPanel::Anchored               | Api/OutlinerPanel.h          | 77      | The occupant the presented span is anchored on, undeclared when nothing is counted. never declared as intent and no transaction records it.
F OutlinerPanel::RowsTouched            | Api/OutlinerPanel.h          | 84      | How many rows the last presentation actually touched. not to the population. A million occupants that presented a million rows is the defect.
F OutlinerPanel::Sought                 | Api/OutlinerPanel.h          | 89      | The text the artist is searching names for, empty when nothing is sought.
F OutlinerPanel::ConfirmedNames         | Api/OutlinerPanel.h          | 94      | How many names the last narrowing confirmed.
V ReorderPayload                        | Source/OutlinerPanel.cpp     | 21      | ?
T DraggedRow                            | Source/OutlinerPanel.cpp     | 26-30   | ?
F DraggedIdentity                       | Source/OutlinerPanel.cpp     | 32-39   | ?
F CarriedIdentity                       | Source/OutlinerPanel.cpp     | 41-48   | ?
F DeclareStanding                       | Source/OutlinerPanel.cpp     | 56-67   | ?
F DeclareSelection                      | Source/OutlinerPanel.cpp     | 71-80   | ?
F DeclareEnclosure                      | Source/OutlinerPanel.cpp     | 85-97   | ?
F OutlinerPanel::Present                | Source/OutlinerPanel.cpp     | 105-359 | ?
F OutlinerPanel::DeclarePresence        | Source/OutlinerPanel.cpp     | 365-368 | ?
F OutlinerPanel::PresenceStanding       | Source/OutlinerPanel.cpp     | 370-373 | ?
F OutlinerPanel::VisiblePosition        | Source/OutlinerPanel.cpp     | 375-378 | ?
F OutlinerPanel::Anchored               | Source/OutlinerPanel.cpp     | 380-383 | ?
F OutlinerPanel::RowsTouched            | Source/OutlinerPanel.cpp     | 385-388 | ?
F OutlinerPanel::Sought                 | Source/OutlinerPanel.cpp     | 390-393 | ?
F OutlinerPanel::ConfirmedNames         | Source/OutlinerPanel.cpp     | 395-398 | ?
