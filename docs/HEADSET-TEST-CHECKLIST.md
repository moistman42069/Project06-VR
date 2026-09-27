# Quest 3 candidate 0.1.9 checks

Sideload over the previous candidate; preserve the log in Downloads/P06Quest.
Expected startup milestones: `Managed runtime ready`, `XR graphics initialized`,
`OpenXR session state=FOCUSED`, `First OpenXR frame submitted`, and visible game.
Logs alone cannot prove visible pixels.

- [ ] Fresh launch reaches title and both eyes remain free of pink flicker.
- [ ] Original sticks/buttons/grips/triggers work and third-person stereo remains correct.
- [ ] Both stick clicks open the blue category list; A enters and X backs out; closing input is swallowed.
- [ ] All three display modes, saved settings and Meta-button recenter work.
- [ ] First person hides Sonic, shows matching gloves and keeps the HUD visible.
- [ ] Eye-height adjustment, right-stick yaw and head tracking feel correct.
- [ ] Looking in a new direction then pushing forward follows that heading.
- [ ] Upgrade cuffs disappear and glove fingers point with controller aim.
- [ ] Test character changes; unsupported abilities must not emit substitute attacks.
- [ ] Losing one controller hides that hand without moving the camera back.
- [ ] Disable first person: body, HUD mode and near clipping restore.
- [ ] Enable running: alternating two-hand swings control speed and head direction.
- [ ] Standing still, using one hand, menus and left-stick override do not run accidentally.
- [ ] Homing requires an eligible native target, pointing dwell and inward pull.
- [ ] Crouch calibrates standing, charges and releases once when standing again.
- [ ] Opening a menu or losing tracking during charge does not launch Sonic.
- [ ] Haptic toggle/intensity work; rings, actions, movement and charge/release feel appropriate.
- [ ] Vibration stops on pause, Quest overlay and focus loss.
- [ ] Repeat the full recorded water route; preserve entry/exit/recovery log lines.
- [ ] Normal low-speed falls, slopes, rails, springs and non-water triggers still work.
- [ ] Death/reset, scene transitions and return to title remain stable.

Report the first failure, approximate run time and matching log. Do not publish a
release before the user confirms the candidate on Quest.
