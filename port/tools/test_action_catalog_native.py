#!/usr/bin/env python3
"""Run every catalog move through the production native actor dispatcher.

An owned ROM is referenced with --rom; it is never copied into test profiles.
Each case starts a fresh training session, presses TECHNIQUE once, and records
the actual move/state/variant, effect or companion lifetime, and recovery.
These checks establish native execution and bounded cleanup, not campaign
balance, immunity bypass, or healing when the fixture already has full HP.
"""
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
NATIVE_COMMAND = 20
CAPTURES = (200, 350, 600)


def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def enum_values(text, prefix):
    """Read ordinary integer enums from the same native source as the game."""
    result = {}
    for body in re.findall(r'enum\s+\w*\s*\{([^}]+)\}', text, re.S):
        if prefix not in body:
            continue
        body = re.sub(r'/\*.*?\*/', '', body, flags=re.S)
        body = re.sub(r'//[^\n]*|^\s*#[^\n]*$', '', body, flags=re.M)
        current = -1
        for item in body.split(','):
            match = re.fullmatch(r'\s*(\w+)\s*(?:=\s*(0x[0-9a-fA-F]+|\d+))?\s*', item)
            if not match:
                continue
            current = int(match[2], 0) if match[2] else current + 1
            result[match[1]] = current
    return result


def source_expectations(metadata):
    """Check the real dispatcher case and variant, rather than its UI name."""
    sora = (ROOT / 'src/btl/btl_sora.c').read_text(encoding='utf-8')
    values = enum_values(sora, 'BTL_SORA_STATE_')
    for name, prefix in (('btl.h', 'SUMMON_LEVEL_'), ('btl.h', 'BTL_ITEM_'),
                         ('btl3.h', 'BTL_RAID_'), ('btl_effect.h', 'SPELL_TIER_')):
        values.update(enum_values((ROOT / 'include/btl' / name).read_text(), prefix))
    start = sora.index('switch (move)', sora.index('case BTL_REACTION_CARD_ACTION:'))
    end = sora.index('case BTL_REACTION_HAZARD:', start)
    cases = dict(re.findall(r'case\s+(MOVE_\w+)\s*:(.*?)(?=\n\s*case\s+MOVE_|\n\s*default:)',
                            sora[start:end], re.S))
    symbols = re.findall(r'\bE\((MOVE_\w+),',
                        (ROOT / 'port/src/port_action_catalog.c').read_text())
    if len(symbols) != len(metadata) or len(metadata) != 124:
        raise ValueError('metadata must match all 124 source catalog rows')
    result = []
    for row, symbol in zip(metadata, symbols):
        body = cases[symbol]
        state = re.search(r'work->state\s*=\s*(BTL_SORA_STATE_\w+)', body)
        state_name = state[1] if state else ('BTL_SORA_STATE_COMBO' if 'StartBtlSoraCombo(work)' in body else None)
        if not state_name:
            raise ValueError('no concrete native state for ' + symbol)
        expected = dict(row, symbol=symbol, expected_state=values[state_name],
                        expected_state_name=state_name)
        variant = re.search(r'work->variant\[0\]\s*=\s*(\w+)', body)
        blade = re.search(r'work->keyblade\s*=\s*(\d+)', body)
        if variant:
            expected['expected_variant'] = values[variant[1]]
        if blade:
            expected['expected_keyblade'] = int(blade[1])
        result.append(expected)
    return result, values


def parse(log):
    events = []
    for kind, tail in re.findall(r'action: ((?:native|assist)-[a-z-]+)\s+([^\r\n]+)', log):
        event = dict(kind=kind)
        event.update({key: int(value, 0) for key, value in
                      re.findall(r'(\w+)=(-?(?:0x[0-9a-fA-F]+|\d+))', tail)})
        events.append(event)
    accepted = [dict(frame=int(a), command=int(b), name=c) for a, b, c in
                re.findall(r'action: accepted frame=(\d+) id=(\d+) command="([^"]+)"', log)]
    strikes = [dict(frame=int(a), command=int(b), timer=int(c)) for a, b, c in
               re.findall(r'action: strike frame=(\d+) command=(\d+) timer=(\d+)', log)]
    contacts = [dict(frame=int(a), hits=int(b), damage=int(c)) for a, b, c in
                re.findall(r'action: chain frame=(\d+) hits=(\d+) damage=(\d+)', log)]
    hp = [dict(frame=int(a), old=int(b), new=int(c)) for a, b, c in
          re.findall(r'action: hp frame=(\d+) old=(-?\d+) new=(-?\d+)', log)]
    return dict(events=events, accepted=accepted, strikes=strikes, contacts=contacts, hp_changes=hp)


def evaluate(case, data, state_ids):
    """Reject label-only acceptance, idle fallback, and unfinished effects."""
    if case['flags'] & 8 and case['symbol'] != 'MOVE_TRINITY_LIMIT':
        return evaluate_assist(case, data)
    move = case['nativeMove']
    events = [event for event in data['events'] if event.get('move') == move]
    dispatch = [event for event in events if event['kind'] == 'native-dispatch']
    complete = [event for event in events if event['kind'] == 'native-complete']
    snapshots = [event for event in events if event['kind'] in ('native-state', 'native-settled')]
    reasons = []
    if len(data['accepted']) != 1 or data['accepted'][0]['command'] != NATIVE_COMMAND:
        reasons.append('expected one production native command acceptance')
    elif data['accepted'][0]['name'] != case.get('name', data['accepted'][0]['name']):
        reasons.append('accepted native catalog name mismatch')
    if not dispatch:
        reasons.append('no full-width native dispatch trace')
    else:
        first = dispatch[0]
        if first.get('state') != case['expected_state']:
            reasons.append('native dispatcher entered wrong state')
        for actual, wanted in (('variant', 'expected_variant'), ('keyblade', 'expected_keyblade')):
            if wanted in case and first.get(actual) != case[wanted]:
                reasons.append('wrong or unobserved native ' + actual)
    if not snapshots:
        reasons.append('no native execution/lifetime snapshots')
    effect_seen = any(event.get('effect', 0) for event in snapshots)
    fade_seen = any(event.get('fade', 0) for event in snapshots)
    companion_seen = any(event.get('companion', 0) for event in snapshots)
    baseline_tasks = snapshots[0].get('tasks') if snapshots else None
    baseline_battle = snapshots[0].get('battle') if snapshots else None
    tasks_seen = max([event.get('tasks', 0) for event in snapshots] or [0])
    battle_tasks_seen = max([event.get('battle', 0) for event in snapshots] or [0])
    task_spawn_seen = baseline_tasks is not None and tasks_seen > baseline_tasks
    battle_spawn_seen = baseline_battle is not None and battle_tasks_seen > baseline_battle
    segment_seen = any(event['kind'] == 'native-segment' for event in events)
    strike_seen = any(event['command'] == NATIVE_COMMAND for event in data['strikes'])
    execution_seen = bool(effect_seen or fade_seen or companion_seen or task_spawn_seen or battle_spawn_seen or segment_seen or strike_seen or
                          len({event.get('state') for event in snapshots}) > 1)
    if not execution_seen:
        reasons.append('no native effect, task, strike, segment, or state progression')
    if case['flags'] & 4 and not (effect_seen or fade_seen or companion_seen or task_spawn_seen or battle_spawn_seen):
        reasons.append('no actual effect/task/companion lifetime observed')
    if case['flags'] & 8 and case['symbol'] != 'MOVE_TRINITY_LIMIT' and not companion_seen:
        reasons.append('no companion became active')
    if not complete:
        reasons.append('native action did not reach completion')
    elif complete[0].get('elapsed', 0) < 12:
        reasons.append('native action completed before meaningful playback')
    terminal = {state_ids['BTL_SORA_STATE_IDLE'], state_ids['BTL_SORA_STATE_AIRBORNE'],
                state_ids['BTL_SORA_STATE_LAND']}
    settled = [event for event in snapshots if complete and event.get('frame', -1) >= complete[0]['frame']
               and event.get('effect') == 0 and event.get('companion') == 0
               and event.get('fade') == 0
               and baseline_tasks is not None and event.get('tasks', baseline_tasks + 1) <= baseline_tasks
               and event.get('state') in terminal]
    if not settled:
        reasons.append('effect/companion/Sora-task cleanup and return to neutral not observed')
    return dict(passed=not reasons, failure_reasons=reasons, dispatch=dispatch,
                completions=complete, native_snapshots=snapshots,
                native_segments=[event for event in events if event['kind'] == 'native-segment'],
                effect_observed=effect_seen, fade_observed=fade_seen, companion_observed=companion_seen,
                baseline_sora_tasks=baseline_tasks, maximum_sora_tasks=tasks_seen,
                baseline_battle_tasks=baseline_battle, maximum_battle_tasks=battle_tasks_seen,
                battle_task_spawn_observed=battle_spawn_seen,
                sora_task_spawn_observed=task_spawn_seen, native_execution_observed=execution_seen,
                settled=settled[-1] if settled else None)


def evaluate_assist(case, data):
    """Independent native task playback must leave Sora visible and untouched."""
    events = [event for event in data['events'] if event.get('move') == case['nativeMove']]
    starts = [event for event in events if event['kind'] == 'assist-start']
    states = [event for event in events if event['kind'] == 'assist-state']
    ends = [event for event in events if event['kind'] == 'assist-end']
    reasons = []
    if len(starts) != 1:
        reasons.append('expected one actual independent native assist spawn')
    elif starts[0].get('variant') != case['expected_variant']:
        reasons.append('wrong independent native companion variant')
    if any(event['kind'] == 'native-dispatch' for event in events):
        reasons.append('assist took over Sora native dispatcher')
    if data['accepted']:
        reasons.append('assist consumed a Sora attack command')
    if not states:
        reasons.append('no actual companion native-state/animation telemetry')
    if any(event.get('visible') != 1 for event in starts + states):
        reasons.append('Sora became hidden during independent assist')
    if any(event.get('soraState') not in (1, 3, 4) for event in starts + states):
        reasons.append('safe neutral fixture forced Sora into assist takeover state')
    playback = bool(len({(event.get('state'), event.get('anim'), event.get('pose'), event.get('timer')) for event in states}) > 1)
    if not playback:
        reasons.append('no authored native companion playback progression')
    if not ends:
        reasons.append('native assist did not finish within bounded frame budget')
    elif ends[-1].get('elapsed', 0) < 24:
        reasons.append('native assist ended before meaningful playback')
    clean = ends[-1] if ends and ends[-1].get('tiles') == 0 and ends[-1].get('companion') == 0 else None
    if not clean:
        reasons.append('independent assist owned tiles/companion flag not released')
    if starts and starts[0].get('tiles') != 3200:
        reasons.append('independent native assist has no dedicated tile reservation')
    return dict(passed=not reasons, failure_reasons=reasons, dispatch=starts,
                completions=ends, native_snapshots=states, native_segments=[],
                effect_observed=any(event.get('effect') for event in states),
                fade_observed=any(event.get('fade') for event in states),
                companion_observed=any(event.get('companion') for event in states),
                native_execution_observed=playback, settled=clean,
                independent_assist=True, assist_contacts=[event for event in events if event['kind'] == 'assist-contact'],
                assist_heals=[event for event in events if event['kind'] == 'assist-heal'],
                lifecycle_note='Owned native companion task/tile lifetime checked. Background FX can outlive native task; full battle allocator-leak freedom is not claimed.')


def bitmap_valid(path):
    if not path.is_file() or path.stat().st_size < 54:
        return False
    with path.open('rb') as stream:
        header = stream.read(54)
    return header[:2] == b'BM' and all(struct.unpack_from('<ii', header, 18))


def run_attempt(case, frames, args, config, states):
    folder = args.out / ('%03d-' % case['index'] + re.sub(r'[^a-z0-9]+', '-', case.get('fixture', case['name']).lower()).strip('-')) / ('frames-%04d' % frames)
    folder.mkdir(parents=True, exist_ok=True)
    capture_frames = sorted(set(CAPTURES + ((260, 450) if case['flags'] & 8 else ())))
    # Remove only known diagnostics belonging to this isolated test attempt.
    for name in ('host.log', 'kh_com.log') + tuple('host_shot_%d.bmp' % frame for frame in capture_frames):
        (folder / name).unlink(missing_ok=True)
    (folder / 'kh_com.ini').write_text(config, encoding='utf-8')
    (folder / 'rebel_mode.txt').write_text('unlocked\n', encoding='utf-8')
    env = {key: value for key, value in os.environ.items() if not key.startswith(('KHCOM_', 'SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', SDL_RENDER_DRIVER='software',
               KHCOM_FAST='1', KHCOM_PPU_THREADS='1', KHCOM_ACTION_TRACE='1',
               KHCOM_ASSETS=str(ROOT / 'port/assets'), KHCOM_PRACTICE=case.get('preset', 'training'),
               KHCOM_CATALOG_INDEX=str(case['index']), KHCOM_KEYS=case.get('keys', '180-181:TECHNIQUE'),
               KHCOM_QUIT=str(frames + 2), KHCOM_HOST_SHOT=','.join(map(str, capture_frames)))
    began = time.monotonic()
    with (folder / 'host.log').open('w', encoding='utf-8') as stream:
        try:
            code = subprocess.run([str(args.host), str(args.guest), '--data', str(folder),
                                   '--rom', str(args.rom)], cwd=ROOT, env=env, stdout=stream,
                                  stderr=stream, timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
    log_path = folder / 'kh_com.log'
    log = log_path.read_text(errors='replace') if log_path.exists() else ''
    data = parse(log)
    verdict = evaluate(case, data, states)
    reasons = verdict['failure_reasons']
    if code != 0:
        reasons.append('process exit ' + str(code))
    if re.search(r'\bCRASH\b|AddressSanitizer|runtime error:|out of memory|assertion failed', log, re.I):
        reasons.append('crash, sanitizer, assertion, or allocation failure in log')
    if not re.search(r'practice: start frame=\d+ preset=' + str(case.get('preset_id', 0)) + r' ', log) or 'mode: mode_battle' not in log:
        reasons.append('isolated training battle not entered')
    shots = [folder / ('host_shot_%d.bmp' % frame) for frame in capture_frames]
    invalid = [path.name for path in shots if not bitmap_valid(path)]
    if invalid:
        reasons.append('missing or invalid captures: ' + ', '.join(invalid))
    result = dict(case=case, frames=frames, exit=code,
                  elapsed_seconds=round(time.monotonic() - began, 3), cheats=False,
                  screenshots=[str(path) for path in shots], accepted=data['accepted'],
                  strikes=data['strikes'], contacts=data['contacts'], hp_changes=data['hp_changes'], **verdict)
    result['passed'] = not reasons
    (folder / 'report.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return result


def run_case(case, args, config, states, fingerprint):
    case_folder = args.out / ('%03d-' % case['index'] + re.sub(r'[^a-z0-9]+', '-', case['name'].lower()).strip('-'))
    previous_path = case_folder / 'report.json'
    if args.resume and previous_path.is_file():
        previous = json.loads(previous_path.read_text(encoding='utf-8'))
        if previous.get('fingerprint') == fingerprint and previous.get('passed') and all(
                bitmap_valid(Path(path)) for path in previous['attempts'][-1]['screenshots']):
            print(json.dumps(dict(index=case['index'], name=case['name'], passed=True, resumed=True)), flush=True)
            return previous
    attempts = []
    for frames in sorted(set((650, min(1800, args.max_frames), args.max_frames))):
        result = run_attempt(case, frames, args, config, states)
        attempts.append(result)
        if result['passed'] or result['exit'] != 0 or not result['dispatch']:
            break
        if result['completions'] and result['settled']:
            # Once playback and all channels settled, time cannot supply an
            # effect or correct variant that never happened in that action.
            break
        # A mapping failure is deterministic; extra time cannot repair it.
        if any(reason.startswith(('wrong ', 'native dispatcher entered')) for reason in result['failure_reasons']):
            break
    final = dict(index=case['index'], name=case['name'], nativeMove=case['nativeMove'],
                 category=case['categoryName'], fingerprint=fingerprint,
                 passed=attempts[-1]['passed'], attempts=attempts,
                 failure_reasons=attempts[-1]['failure_reasons'])
    if case['category'] == 5:
        final['semantics'] = 'Native card-reload item; action fixture may have no applicable depleted cards. No HP or MP restoration claim.'
    elif case['flags'] & 32:
        final['semantics'] = 'Support native effect executed; full-HP fixture does not establish amount healed or resource recovery.'
    folder = Path(attempts[-1]['screenshots'][0]).parent.parent
    (folder / 'report.json').write_text(json.dumps(final, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(index=case['index'], name=case['name'], passed=final['passed'],
                          frames=attempts[-1]['frames'], failures=final['failure_reasons'])), flush=True)
    return final


def self_test():
    states = dict(BTL_SORA_STATE_IDLE=1, BTL_SORA_STATE_AIRBORNE=3, BTL_SORA_STATE_LAND=4)
    case = dict(nativeMove=0x8002ACAB, expected_state=6, expected_variant=1, flags=4, symbol='MOVE_FIRA')
    log = '''action: accepted frame=181 id=20 command="Fira"
action: native-dispatch frame=181 move=2147658923 state=6 variant=1 keyblade=0
action: native-state frame=208 move=2147658923 state=6 effect=1 companion=0 fade=0 tasks=0 battle=10
action: native-complete frame=250 command=20 move=2147658923 elapsed=69 state=1
action: native-state frame=263 move=2147658923 state=1 effect=0 companion=0 fade=0 tasks=0 battle=10
'''
    assert evaluate(case, parse(log), states)['passed']
    assert not evaluate(case, parse(log.replace('effect=1', 'effect=0')), states)['passed']
    assert not evaluate(case, parse(log.replace('variant=1', 'variant=0')), states)['passed']
    assert not evaluate(case, parse(log.split('action: native-complete')[0]), states)['passed']
    assert not evaluate(case, parse('action: accepted frame=181 id=20 command="Fira"'), states)['passed']
    print('PASS: runtime validator rejects acceptance-only, missing effect, wrong tier and incomplete cleanup evidence.')


def serialization_probe(args, config, states, cases, provenance):
    """Measure a real enemy hit, then request a second still-active companion."""
    case = dict(cases[61], fixture='serialization-reference', preset='mixed', preset_id=2)
    reference = run_attempt(case, 1200, args, config, states)
    snapshots = reference['native_snapshots']
    neutral = {states['BTL_SORA_STATE_IDLE'], states['BTL_SORA_STATE_AIRBORNE'], states['BTL_SORA_STATE_LAND']}
    schedule = None
    for hit in reference['hp_changes']:
        if hit['new'] >= hit['old']:
            continue
        before = [row for row in snapshots if row['frame'] <= hit['frame']]
        if not before or not before[-1].get('companion'):
            continue
        recovered = next((row for row in snapshots if row['frame'] > hit['frame'] and
                          row.get('companion') and row.get('state') in neutral), None)
        if recovered:
            schedule = dict(hit_frame=hit['frame'], recovered_frame=recovered['frame'],
                            press_frame=recovered['frame'] + 1)
    replay = None
    failures = []
    if schedule is None:
        failures.append('fixture did not observe real enemy damage and neutral recovery while first companion remained active')
    else:
        case.update(fixture='serialization-replay', keys='180-181:TECHNIQUE,%d-%d:TECHNIQUE' %
                    (schedule['press_frame'], schedule['press_frame'] + 1))
        replay = run_attempt(case, 1800, args, config, states)
        dispatches = replay['dispatch']
        if len(dispatches) != 2:
            failures.append('expected two actual native companion dispatches')
        else:
            second = dispatches[1]['frame']
            prior = [row for row in replay['native_snapshots'] if row['frame'] <= second]
            clears = [row for row in prior if row['frame'] > 183 and row.get('companion') == 0]
            if not clears or clears[-1]['frame'] < schedule['press_frame']:
                failures.append('second companion dispatched before observed first-companion cleanup')
            if second <= schedule['press_frame']:
                failures.append('second request did not wait for active companion ownership')
            if not any(row['old'] > row['new'] and row['frame'] <= schedule['recovered_frame'] for row in replay['hp_changes']):
                failures.append('replay did not reproduce real enemy damage')
            active = [row for row in replay['native_snapshots'] if row['frame'] > second and row.get('companion')]
            if not active:
                failures.append('second companion never became active')
            clean = [row for row in replay['native_snapshots'] if active and row['frame'] > active[-1]['frame']
                     and row.get('companion') == 0 and row.get('effect') == 0 and row.get('fade') == 0
                     and row.get('tasks', 999) <= 2 and row.get('state') in neutral]
            if not clean:
                failures.append('companion/effect/task ownership did not clean up')
        if replay['exit'] != 0 or any('process exit' in reason or 'crash' in reason for reason in replay['failure_reasons']):
            failures.append('native replay process failed')
    report = dict(**provenance, passed=not failures, failure_reasons=failures,
                  schedule=schedule, reference=reference, replay=replay, cheats=False,
                  scope='Real mixed encounter damage/recovery; second Paradise III request waits for existing companion ownership after Sora is interrupted.')
    (args.out / 'serialization-report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(serialization_probe=True, passed=report['passed'], schedule=schedule, failures=failures)), flush=True)
    return 0 if report['passed'] else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', type=Path, default=ROOT / 'build/windows64/out/rekhcom64.exe')
    parser.add_argument('--guest', type=Path, default=ROOT / 'build/windows64/out/guest-us.bin')
    parser.add_argument('--rom', type=Path, default=ROOT / 'roms/B8CE.gba')
    parser.add_argument('--catalog', type=Path, default=ROOT / 'build/catalog-tests/catalog.json')
    parser.add_argument('--out', type=Path, default=ROOT / 'build/catalog-v09')
    parser.add_argument('--only', help='comma-separated catalog indexes or inclusive ranges, e.g. 0-16,53')
    parser.add_argument('--jobs', type=int, choices=(1, 2), default=1)
    parser.add_argument('--max-frames', type=int, choices=(650, 1800, 3600), default=3600)
    parser.add_argument('--timeout', type=int, default=360)
    parser.add_argument('--scale', type=int, choices=(1, 2, 3, 4), default=2)
    parser.add_argument('--resume', action='store_true', help='reuse passing cases with identical binaries, source expectations, config and validator')
    parser.add_argument('--serialization-probe', action='store_true', help='adapt to a real enemy hit and verify interrupted companion ownership serializes the next request')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    for name in ('host', 'guest', 'rom', 'catalog'):
        path = getattr(args, name).resolve()
        if not path.is_file():
            parser.error('missing --%s: %s' % (name, path))
        setattr(args, name, path)
    args.out = args.out.resolve()
    if not args.out.is_relative_to((ROOT / 'build').resolve()):
        parser.error('--out must stay in this mod checkout build directory for isolated profiles')
    metadata = json.loads(args.catalog.read_text(encoding='utf-8-sig'))
    cases, states = source_expectations(metadata)
    all_expectations = cases
    if args.only:
        indexes = []
        try:
            for item in args.only.split(','):
                ends = [int(value) for value in item.split('-')]
                indexes.extend(range(ends[0], ends[-1] + 1))
            indexes = list(dict.fromkeys(indexes))
            if not indexes or any(index < 0 or index >= 124 for index in indexes):
                raise ValueError()
        except ValueError:
            parser.error('--only must contain valid catalog indexes/ranges 0..123')
        cases = [cases[index] for index in indexes]
    config = (ROOT / 'mod/action-edition/kh_com.ini').read_text()
    for key, value in dict(action_combat=1, window_scale=args.scale, cheat_one_hit=0,
                           cheat_infinite_hp=0, cheat_savestates=0).items():
        config = re.sub(r'(?m)^' + key + r'=.*$', key + '=' + str(value), config)
    provenance = dict(host_sha256=digest(args.host), guest_sha256=digest(args.guest),
                      catalog_sha256=digest(args.catalog), validator_sha256=digest(Path(__file__)))
    fingerprint = hashlib.sha256(json.dumps(dict(provenance=provenance, config=config,
                                               cases=all_expectations), sort_keys=True).encode()).hexdigest()
    args.out.mkdir(parents=True, exist_ok=True)
    if args.serialization_probe:
        return serialization_probe(args, config, states, all_expectations, provenance)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        reports = list(pool.map(lambda case: run_case(case, args, config, states, fingerprint), cases))
    result = dict(**provenance, passed=all(row['passed'] for row in reports),
                  guest_unchanged=provenance['guest_sha256'] == digest(args.guest),
                  host_unchanged=provenance['host_sha256'] == digest(args.host),
                  tested=len(reports), catalog_total=124, full_catalog=len(reports) == 124,
                  failures=[row['index'] for row in reports if not row['passed']], tests=reports,
                  cheats=False, rom_copied=False, scope='Actual native actor dispatch, tiers, execution lifetime, neutral recovery and Sora-task/shared-channel cleanup in isolated training. No campaign, immunity-bypass, full-HP healing or allocator-leak claim.')
    result['category_summary'] = {category: dict(tested=sum(row['category'] == category for row in reports),
        passed=sum(row['category'] == category and row['passed'] for row in reports))
        for category in dict.fromkeys(row['category'] for row in reports)}
    result['passed'] = result['passed'] and result['guest_unchanged'] and result['host_unchanged']
    (args.out / 'report.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: result[key] for key in ('passed', 'tested', 'full_catalog', 'failures', 'guest_unchanged', 'host_unchanged')}), flush=True)
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
