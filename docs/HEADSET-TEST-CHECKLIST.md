# Quest 3 candidate 0.1.12 checks

Sideload over the previous candidate; preserve the log in Downloads/P06Quest.
Expected startup milestones: `Managed runtime ready`, `XR graphics initialized`,
`OpenXR session state=FOCUSED`, `First OpenXR frame submitted`, and visible game.
Logs alone cannot prove visible pixels.

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

Report the first failure, approximate run time and matching log. Do not publish a
release before the user confirms the candidate on Quest.
