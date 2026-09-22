# Quest 3 headset test checklist

For 0.1.1, first check the log reports nonzero Unity resource IDs and
`UnityPlayerActivity.onCreate completed`.

Use a fresh launch and retain the corresponding `Downloads/P06Quest` log.

- [ ] App appears as Project 06 Quest and launches from Unknown Sources.
- [ ] OpenXR starts without a flat-screen-only fallback or immediate crash.
- [ ] Title screen and main menu accept left/right sticks and buttons.
- [ ] A gameplay stage loads and Sonic is visible in third-person stereo.
- [ ] Head rotation and translation move the view correctly.
- [ ] Left/right sticks, A/B/X/Y, triggers and grips perform expected actions.
- [ ] Both stick clicks open and close the VR menu without stray game input.
- [ ] Menu navigation and Resume work; game pauses while the menu is open.
- [ ] HUD is readable and does not appear as a duplicated/flat overlay.
- [ ] 3D stereo screen mode works after selecting it.
- [ ] 2D theatre mode works with one centered image.
- [ ] Holding the right Meta/system button recenters through Quest normally.
- [ ] Scene transition, death/reset and return to the title remain stable.
- [ ] Log contains no repeating OpenXR, hook, frame or native-crash errors.

Report the first failed item, approximate time in the run, and the matching log.
