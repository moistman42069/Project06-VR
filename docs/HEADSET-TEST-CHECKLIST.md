# Quest 3 release 0.1.15 checks and future regression list

The user tested 0.1.15 on Quest 3 and accepted it as the first release. This
checklist is retained for future regression testing and broader compatibility
checks; it is not a claim that every listed scenario was individually tested.

Sideload over the previous candidate; preserve the log in Downloads/P06Quest.
Expected startup milestones: `Managed runtime ready`, `XR graphics initialized`,
`OpenXR session state=FOCUSED`, `First OpenXR frame submitted`, and visible game.
Logs alone cannot prove visible pixels.

- [ ] HUD and title width change independently without changing height; resets restore width 100%.
- [ ] Hide In-Game UI hides game canvases, while VR panel/title remain accessible; disabling restores visibility.
- [ ] Opening VR settings temporarily restores HUD; closing hides it again when configured.
- [ ] Startup and setting changes emit complete VR settings snapshots in the run log.
- [ ] Change several settings, relaunch, update over the existing app and verify exact values remain.
- [ ] Existing saved settings and saves survive update; fresh defaults match the user's supplied values.
- [ ] In-Game HUD Follows View: look up/down/sideways and move head in first/third person; HUD follows with saved offsets and size.
- [ ] Disable HUD follow: prior placement restores; enable Immersive Mode (All): HUD follow turns on and persists after restart.
- [ ] Title/menu UI and anchored VR panel retain independent behavior.
- [ ] HUD size reaches 5%; offsets/depth remain independently adjustable and reset recovers visibility.
- [ ] Title and main menu start smaller; separate UI controls work after stage-to-title transitions without pink flicker.
- [ ] Open VR panel, rotate/move head: panel remains in place; close/reopen relocates it.
- [ ] Recenter with panel open, and suspend/resume: panel stays accessible.
- [ ] VR panel follow-view option and size/distance adjustments work; reset recovers defaults.
- [ ] First VR row enables all four interactions, reflects individual overrides, and turns the four off together.
- [ ] Default glove alignment feels better; custom hand angles persist and reset restores the new defaults.
- [ ] Install over current APK without uninstalling or clearing app data.
- [ ] UI: HUD left/right, down/up, size and depth respond separately in a level.
- [ ] Repeat HUD adjustments in first and third person; reset restores defaults.
- [ ] VR: each hand pitch/yaw/roll changes only that glove; reset restores its accepted orientation.
- [ ] Calibration persists after relaunch; old motion/haptic preferences and saves remain.
- [ ] With hand rotations changed, physical running/homing/crouch still use the original tracked poses.
- [ ] Return to title/level select after moving HUD; menus remain visible in both eyes.
- [ ] Previously incompatible, intact slots show their progress and load without resetting.
- [ ] After slot selection, reach level select and enter a stage without crashing.
- [ ] Quit/relaunch, then reopen the same slot; progress and settings remain.
- [ ] Fresh launch reaches title and both eyes remain free of pink flicker.
- [ ] Enter a stage, return to level select, then title: neither eye turns pink.
- [ ] Original sticks/buttons/grips/triggers work and third-person stereo remains correct.
- [ ] Both stick clicks open the blue category list; A enters and X backs out; closing input is swallowed.
- [ ] All three display modes, saved settings and Meta-button recenter work.
- [ ] First person hides Sonic, shows matching gloves and keeps the HUD visible.
- [ ] Eye-height adjustment, right-stick yaw and head tracking feel correct.
- [ ] Looking in a new direction then pushing forward follows that heading on ground AND after jumping/springs.
- [ ] Nearby world geometry stays visible while looking around and moving; check frame rate.
- [ ] Body accessories stay hidden; smooth Sonic gloves point with controller aim, no feet.
- [ ] VR category scrolls to running, homing and crouch controls; X returns to root.
- [ ] Test character changes; unsupported abilities must not emit substitute attacks.
- [ ] Losing one controller hides that hand without moving the camera back.
- [ ] Disable first person: body, HUD mode and near clipping restore.
- [ ] Enable running: alternating two-hand swings control speed and head direction.
- [ ] Standing still, using one hand, menus and left-stick override do not run accidentally.
- [ ] Homing requires an eligible native target and an outward swing from either hand; inward pull/head-only motion must not attack.
- [ ] Crouch calibrates standing, charges and releases once when standing again.
- [ ] Opening a menu or losing tracking during charge does not launch Sonic.
- [ ] Haptic toggle/intensity work; rings, actions, movement and charge/release feel appropriate.
- [ ] Vibration stops on pause, Quest overlay and focus loss.
- [ ] Repeat the full recorded water route; preserve entry/exit/recovery log lines.
- [ ] Normal low-speed falls, slopes, rails, springs and non-water triggers still work.
- [ ] Death/reset, scene transitions and return to title remain stable.

For future tests, report the first failure, approximate run time and matching
log. The release acceptance record is in `docs/RELEASE-0.1.15.md`.
