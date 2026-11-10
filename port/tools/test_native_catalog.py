#!/usr/bin/env python3
"""Check the native inventory against independent source/schema denominators."""
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys

from build_native_catalog import ROOT, build, HTML, comment_mask, matching


def main():
    catalog = build()
    entries = catalog['entries']
    counts = Counter(entry['kind'] for entry in entries)
    assert not catalog['unresolved'], catalog['unresolved']
    assert len({entry['id'] for entry in entries}) == len(entries)
    by_id = {entry['id']: entry for entry in entries}
    manifests = sorted((ROOT / 'config/assets').glob('*.yaml'))
    source = '\n'.join(path.read_text(encoding='utf-8') for path in manifests)
    assert len(manifests) == catalog['coverage']['asset_manifests']
    assert counts['sprite-sheet'] == len(re.findall(r'^  format: sprite_sheet$', source, re.M))
    assert counts['palette'] == len(re.findall(r'^  format: palette$', source, re.M))
    assert counts['sprite-frame'] + counts['sprite-animation'] == len(re.findall(r'^  - symbol:', source, re.M))
    assert counts['authored-animation-header'] == len(re.findall(r'^  record: AnimHeader$', source, re.M))
    assert counts['authored-oam-sprite'] == len(re.findall(r'^  record: Sprite$', source, re.M))
    assert catalog['coverage']['all_manifest_entry_records'] == len(re.findall(r'^- name:', source, re.M))
    native = '\n'.join(path.read_text(encoding='utf-8', errors='replace') for path in (ROOT / 'src').rglob('*.c'))
    assert counts['native-task'] + counts['mod-task-bridge'] == len(re.findall(r'\bTaskDesc\s+\w+\s*=', native))
    assert counts['background-animation'] == len(re.findall(r'\bBgAnimationDef\s+\w+\s*=', native))
    assert counts['background-effect-api'] == len(re.findall(r'^void BgFxStart\w+\(', native, re.M))
    attacks = [entry for entry in entries if entry['kind'] == 'native-attack-definition']
    assert len(attacks) == 330 and {entry['attack_id'] for entry in attacks} == set(range(330))
    assert all(len(entry['parameters']) == 6 and not entry.get('unresolved_flag_symbols') for entry in attacks)
    assert any('FIRE' in entry['elements'] for entry in attacks)
    assert any('CONFUSE' in entry['statuses'] for entry in attacks)
    assert any('ATTACK_FLAG_WARP_AWAY' in entry['behavior_flags'] for entry in attacks)
    assert counts['actor-state-enum'] >= 180 and counts['actor-state'] >= 1000
    expected_enums = set()
    expected_members = Counter()
    for path in list((ROOT / 'src').rglob('*.c')) + list((ROOT / 'include').rglob('*.h')):
        text = comment_mask(path.read_text(encoding='utf-8', errors='replace'))
        for enum in re.finditer(r'\benum\s*(\w+)?\s*\{([^{}]*)\}', text, re.S):
            name, body = enum[1] or 'anonymous', enum[2]
            if not re.search(r'State|Phase', name) and not re.search(r'\b\w+_STATE_\w+', body):
                continue
            key = (path.relative_to(ROOT).as_posix(), name, text.count('\n', 0, enum.start()) + 1)
            expected_enums.add(key)
            body = re.sub(r'^\s*#[^\n]*', '', body, flags=re.M)
            for part in body.split(','):
                member = re.match(r'\s*([A-Z][A-Z0-9_]+)', part)
                if member:
                    expected_members[(key[0], name, member[1])] += 1
    actual_enums = {(entry['source'], entry['symbol'], entry['line']) for entry in entries if entry['kind'] == 'actor-state-enum'}
    actual_members = Counter((entry['source'], entry['enum'], entry['symbol']) for entry in entries if entry['kind'] == 'actor-state')
    assert actual_enums == expected_enums
    assert actual_members == expected_members, (actual_members - expected_members, expected_members - actual_members)
    assert any(entry['symbol'] == 'HUM_AXCEL_STATE_FIRE_WALL' for entry in entries if entry['kind'] == 'actor-state')
    assert any(entry['case_sites'] for entry in entries if entry['kind'] == 'actor-state' and entry['source'].startswith('src/bos/'))
    callsites = [entry for entry in entries if entry['kind'] == 'authored-hitbox-callsite']
    expected_calls = Counter()
    for path in (ROOT / 'src').rglob('*.c'):
        text = comment_mask(path.read_text(encoding='utf-8', errors='replace'))
        expected_calls.update(re.findall(r'\b(ApplyAttackBox|ApplyAttackToBtlObj|TestAttackBox)\s*\(', text))
        for name in re.findall(r'^(?:s32|u8)\s+(ApplyAttackBox|ApplyAttackToBtlObj|TestAttackBox)\s*\(', text, re.M):
            expected_calls[name] -= 1
    assert Counter(entry['api'] for entry in callsites) == expected_calls
    assert callsites and all(entry['arguments'] for entry in callsites)
    assert all(len(entry['authored_geometry']) == 6 for entry in callsites if entry['api'] == 'ApplyAttackBox')
    assert all(entry['literal_attack_id'] is None or 0 <= entry['literal_attack_id'] < 330 for entry in callsites if entry['api'] != 'TestAttackBox')
    assert any(entry['lexical_state_contexts'] and entry['source'].startswith('src/bos/') for entry in callsites)
    sora = [entry for entry in entries if entry['kind'] == 'sora-native-move']
    assert len(sora) == 126 and sum(entry['action_catalog_member'] for entry in sora) == 124
    assert {entry['symbol'] for entry in sora if not entry['action_catalog_member']} == {'MOVE_SLEIGHT_57', 'MOVE_SLEIGHT_68'}
    for table, expected in (('sBtlSoraAnimDefs', 77), ('sBtlSoraDirAnimDefs', 30)):
        bindings = [entry for entry in entries if entry['kind'] == 'native-animation-binding' and entry['table'] == table]
        assert len(bindings) == expected
        assert all(entry.get('resolved_animation') for entry in bindings)
    for entry in entries:
        assert entry['source'] and entry['line'] >= 1 and entry['native_family']
        assert all(target in by_id for target in entry['dependencies'])
        if entry['kind'] in ('sprite-animation', 'authored-animation-header'):
            assert entry['duration_ticks'] == sum(frame[1] for frame in entry['timeline'])
        if entry['kind'] in ('native-task', 'mod-task-bridge'):
            assert not entry['missing_callback_definitions'], entry['symbol']
    # Inputs are auditable source files only; the generator never needs a ROM.
    assert all(row['path'].startswith(('src/', 'include/', 'config/assets/', 'port/src/port_action_')) for row in catalog['inputs'])
    assert all(Path(row['path']).suffix in ('.c', '.h', '.yaml') for row in catalog['inputs'])
    assert all(row['sha256'] == hashlib.sha256((ROOT / row['path']).read_bytes()).hexdigest() for row in catalog['inputs'])
    payload = json.dumps(catalog, ensure_ascii=False, separators=(',', ':'))
    rendered = HTML.replace('__CATALOG__', payload.replace('<', '\\u003c'))
    embedded = re.search(r'<script id="catalog" type="application/json">(.*?)</script>', rendered, re.S)
    assert json.loads(embedded[1]) == json.loads(payload)
    print('PASS: complete authored sprite/frame/timeline/palette denominators; all330 native damage definitions; source-wide state/geometry coverage; Sora107 exact animation bindings;124 concrete move subset; task callbacks/dependency links; ROM-free HTML/JSON round trip.')
    print(json.dumps(dict(entries=len(entries), counts=dict(counts), conditional_callback_notes=len(catalog['parse_notes'])), indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
