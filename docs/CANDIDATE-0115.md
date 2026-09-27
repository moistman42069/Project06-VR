# Candidate 0.1.15: accepted defaults, UI width and visibility

The user accepts 0.1.14 as a fantastic working baseline. Its preserved log and
hash are recorded in evidence/candidate-0114-run.json. Settings below were supplied
explicitly in chat; the previous log did not contain their numeric values.

| Default | Value |
| --- | --- |
| HUD distance | 2.00 |
| HUD horizontal / vertical offset | -0.40 / -0.30 |
| HUD size | 20% |
| Screen distance / width | 1.00 / 3.00 |
| Title/menu size / distance | 15% / 3.00 |
| Left hand roll / right hand roll | +30 / -30 degrees |

Other defaults remain unchanged, including hand pitch -10 and yaw -5/+5.
Existing settings files retain their saved values; fresh installs use these
defaults. HUD/title/hand reset controls use the corresponding new defaults.

## Controls and boundaries

UI contains **HUD Width**, **Title/Menu Width** and **Hide In-Game UI** below
HUD Follows View. Width is a 5-300% multiplier on horizontal scale only; overall
size still adjusts both dimensions. Width defaults to 100%, hiding defaults off.
In-game width uses the existing immersive world-space HUD path; front-end width
uses its existing title canvas path. Existing world-space level UI is untouched.

Hiding disables rendering on bridge-owned Canvas components, preserving their
previous enabled state. It does not disable game objects or gameplay scripts.
The VR panel and title/menu remain accessible, and opening the VR panel temporarily
restores HUD visibility for adjustment. Turning hiding off restores previous
Canvas visibility, including leaving originally disabled canvases disabled.
Scene destruction releases the same existing canvas roots.

Settings schema 9 reads versions 1-8. Startup logs contain the field legend and
active/default snapshot; each saved change records the complete serialized
settings. Save failures are identified separately. Logging and persistence share
one serializer to prevent disagreement between saved and reported values.
The game-save schema remains fixed, and existing progress is not rewritten.

## Validation and limits

ARM64 build and pinned API audit pass. Host checks cover the exact requested
defaults, menu changes, width/hide persistence, schema-8 migration, snapshot
round-trip, and visibility restoration including originally disabled canvases.
Existing tracking/menu/gesture checks pass; the new UI page is visually inspected.
Final APK checks verify signature, alignment, preserved game payload and matching
source archive. Native headset behavior of the new controls remains unverified.

The accepted run has three existing Settings.SetLocalSettings null references;
that unrelated game path is unchanged. No speculative water or other gameplay
changes. Retain 0.1.14 as fallback; no public release.
