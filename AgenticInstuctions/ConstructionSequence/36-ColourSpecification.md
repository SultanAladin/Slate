# 36 — ColourSpecification

Every colour in Slate is a coordinate **and the space it is a coordinate in**. This document declares the spaces,
the conversions between them, and the one rule that prevents the most common defect in a painting application: an
image decoded twice, or not at all, because nobody recorded what it was.

## Position In The Sequence

| Field       | Value                                                                        |
|-------------|-------------------------------------------------------------------------------|
| Units       | `SlateMath.lib` (the conversions), `SlateDocument.lib` (the declaration)      |
| Layers      | `Layer1_Numeric`, `Layer3_Document`                                           |
| Upstream    | `00` (tiers), `02` (`Shared/`, parity)                                        |
| Downstream  | `50`, `56`, `58`, `66`, `76`, `44`, `18`, `42`                               |
| Unblocks    | A declared working space; paint that matches display                         |

## 1. The Components

| Component               | What it owns                                                       |
|-------------------------|---------------------------------------------------------------------|
| `ColourSpecification`   | A coordinate together with the space it is expressed in            |
| `ColourProjection`      | Space-to-space conversion; lives in `Shared/`, proven at Tier B    |
| `TransferProjection`    | The encoding transfer and its inverse                              |
| `WhiteProjection`       | Chromatic adaptation between differing white points                |

🔴 There is no bare triple anywhere in Slate. A colour without its space is a number that three subsystems will
each interpret differently, and all three will look plausible.

## 2. The Three Spaces That Exist At Once

| Space           | Referred to  | Where it is used                                              |
|-----------------|--------------|----------------------------------------------------------------|
| Working         | Scene        | Every computation above `08` §3 ⑧ — painting, shading, `28`   |
| Display         | Display      | Below ⑧ — `66`'s output, `26`, `80`, `14`                     |
| Content-native  | Its own      | An imported image, before it is converted at intake            |

The working space is **declared per document** and stored in it. It is linear and wide — wide enough that a
saturated illuminant does not clip on entry — and it is never assumed. A document opened without a declared
working space is a document from before this rule and is converted on open, with the assumption reported.

⚠️ The display space is not the working space, is not stored in the document, and does not travel with it. `48`
gates the same for exposure and for the same reason: a document that looks different on the machine that opens it
is a document whose appearance belongs to the machine.

## 3. Content-Native Is Not Optional

Imagery arriving through `50` declares its own space at intake, and is converted into the working space once,
there. The declaration comes from the content where the format carries one and from the artist where it does not.

| Situation                                     | Behaviour                                                   |
|-----------------------------------------------|--------------------------------------------------------------|
| The content declares its space                 | Converted at intake; the declaration is recorded            |
| The content declares nothing                   | An assumption is made, recorded, and reported through `86`  |
| The artist overrides                           | Re-converted from the original, never from the converted    |

🔴 Re-conversion is always from the retained original. Converting a converted image is the defect where correcting
an artist's mistake makes the image worse than the mistake did.

## 4. Not Every Channel Is A Colour

`18` declares twenty channels and only some of them carry colour. A roughness value put through a transfer
function is a wrong number that still looks like a plausible surface, which is why the mistake survives review.

| Channel carries         | Converted | Example                                    |
|-------------------------|-----------|---------------------------------------------|
| Reflectance or emission | Yes       | Base colour, emissive colour, transmission |
| A scalar measure        | No        | Roughness, occlusion, thickness            |
| A direction             | No        | Tangent-space perturbation                 |

`42` declares, per channel, which of the three a material's source is. Conversion at intake reads that declaration
and nothing else. There is no heuristic here — no inference from the image's own encoding, and none from its name.

## 5. Illuminant Colour

`44`'s illuminants declare colour as a `ColourSpecification` and, where the illuminant is described by a
temperature instead, `WhiteProjection` produces the coordinate from it. The temperature is retained as the
authored value, because an artist who set 5600 expects to see 5600 when they return.

## 6. The Picker — `00` §12 Resolved

🔴 A colour sampled from the workspace is sampled **scene-referred**, before `66`, and converted into the working
space. This closes `00` §12's open row and `76` §6's copy of it.

The reason is that the alternative does not round-trip. A display-referred sample has been through exposure and
the tone projection; painting with it and then viewing the result applies both again, so what the artist sampled
is not what they get. The tone projection is not invertible in general, so no correction after the fact recovers
it.

Sampling the display value is offered as a **separate, named action** for the case it is actually right for —
matching a reference image placed beside the work. It reports that it is display-referred, because a value
sampled that way and painted will not match the thing it was sampled from.

## 7. Precision

| Computation                       | Tier | Reason                                              |
|-----------------------------------|------|------------------------------------------------------|
| Primaries conversion              | B    | A linear projection; `Shared/`, parity-proven       |
| Transfer encode and decode        | B    | Non-linear; the error compounds through `66`        |
| Chromatic adaptation              | B    | As above                                            |
| Space identity comparison         | A    | An integer; a mistaken match converts nothing       |

## 8. Gates

- **Gate:** Every stored colour carries its space; no bare triple exists in the tree.
- **Gate:** The working space is declared in the document, never assumed.
- **Gate:** The display space is never stored in the document.
- **Gate:** Imported content is converted once, at intake, and re-conversion is from the retained original.
- **Gate:** An undeclared content space produces a recorded assumption and an `86` report.
- **Gate:** Only channels `42` declares as colour-carrying are converted.
- **Gate:** `ColourProjection` lives in `Shared/` and is proven at Tier B by `ParityRunner`.
- **Gate:** Workspace sampling is scene-referred; display sampling is a separate action and says so.

## 9. Open

| Open question                                                           | Blocks                        |
|--------------------------------------------------------------------------|--------------------------------|
| Which primaries the default working space uses                           | Nothing structural; a constant |
| Whether the display space is queried from the OS or declared in settings | `66` encode only               |
| Whether a document may declare a second working space for a layer        | `56`; probably refused         |
