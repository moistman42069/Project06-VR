"""Verify character capabilities against APK metadata and reference source.

This proves code presence, not that every character is selectable in this port.
"""
import pathlib,re,json
root=pathlib.Path(__file__).resolve().parents[1]
dump=(root/'vendor/dumper/dump.cs').read_text(encoding='utf-8-sig')
names=['SonicNew','SonicFast','Princess','SnowBoard','Shadow','Silver','Tails',
       'Amy','Knuckles','Blaze','Rouge','Omega','MetalSonic']
rows=[]
for name in names:
    m=re.search(r'^public class '+name+r' : PlayerBase[^\n]*\n\{(.*?)^\}',dump,re.M|re.S)
    assert m,name
    metadata=m[1]
    ref=(root/'out/Project06OSP-source'/f'{name}.cs').read_text()
    shared='AccelerationSystem(' in ref and 'RotatePlayer(' in ref
    assert shared==(name not in ['SonicFast','SnowBoard']),name
    spin=name in ['SonicNew','Shadow','MetalSonic']
    if spin:assert 'SpinDashState;' in metadata and 'void StateSpinDash()' in metadata
    homing=name in ['SonicNew','Shadow','MetalSonic','Princess']
    if homing:assert 'ReleasedKey;' in metadata and 'bool OnJumpDash()' in metadata,name
    assert ('PlayerRenderers;' in metadata or 'PlayerRenderer;' in metadata),name
    rows.append({'class':name,'shared_ground_motor':shared,'native_jump_homing':homing,
                 'native_crouch_spin':spin,'visual_renderer_field': 'PlayerRenderers' if 'PlayerRenderers;' in metadata else 'PlayerRenderer'})
(root/'evidence/character-profiles.json').write_text(json.dumps(rows,indent=2)+'\n')
print('PASS: 13 pinned character classes, visual fields and native ability/motor profiles')
