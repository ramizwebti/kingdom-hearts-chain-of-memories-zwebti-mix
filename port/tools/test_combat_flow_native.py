#!/usr/bin/env python3
"""Integrated combat-flow checks on the real guest and ordinary input path.

Uses the cohesion runner only for isolated native execution and trace parsing.
Its historical baseline flag disables old assertions; this suite always applies
its own strict checks. A separately supplied older guest is comparison evidence,
never a way to make the candidate pass. No state writes or gameplay cheats.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import test_cohesion_native as coh

ROOT = Path(__file__).resolve().parents[2]
NORMALS = (1, 2, 3, 4, 5, 6)


def rows(log, kind):
    return [dict((k, int(v)) for k, v in re.findall(r'(\w+)=(-?\d+)(?=\s|,|$)', line))
            for line in log.splitlines() if line.startswith('action: ' + kind + ' ')]


def case(name, keys, frames=520, shots=None):
    return dict(name=name, preset='training', keys=keys, frames=frames,
                shots=shots or [180, 205, 270, frames - 20])


def check(report, condition, message):
    if not condition:
        report['failures'].append(message)


def run_native(spec, args, guest, host, rom):
    report = coh.run_case(spec, args, guest, host, rom)
    report['failures'] = [] if report['passed'] else ['native execution, boot or capture failed']
    logpath = args.out.resolve() / spec['name'] / 'kh_com.log'
    log = logpath.read_text(errors='replace') if logpath.is_file() else ''
    for kind in ('recovery-link', 'opener', 'native-settled', 'target-keep', 'target-switch'):
        report[kind.replace('-', '_')] = rows(log, kind)
    check(report, not re.search(r'CRASH|out of memory|assertion failed|runtime error:', log, re.I),
          'runtime fault in native trace')
    for filename in report['screenshots']:
        path = Path(filename)
        data = path.read_bytes()[:54] if path.is_file() else b''
        check(report, len(data) >= 54 and data[:2] == b'BM' and
              all(struct.unpack_from('<ii', data, 18)), 'missing or invalid native capture ' + path.name)
    report['passed'] = not report['failures']
    return report


def write_report(report, args):
    report['passed'] = not report['failures']
    (args.out / report['name'] / 'flow-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(name=report['name'], passed=report['passed'], failures=report['failures'])), flush=True)
    return report


def full_blitz(report):
    commands = report['commands']
    check(report, [c['id'] for c in commands] == [1, 2, 3, 20], 'three normals then exactly one native finisher required')
    check(report, bool(commands) and commands[-1]['name'] == 'Blitz', 'expected neutral Blitz route')
    check(report, [s['command'] for s in report['strikes']] == [1, 2, 3], 'each normal must execute its actual strike')
    check(report, len(report['native_sequences']) == 1, 'expected exactly one native sequence')
    if len(report['native_sequences']) != 1:
        return None
    sequence = report['native_sequences'][0]
    check(report, sequence['segments'] == [1, 2, 3], 'Blitz must finish all three authored segments')
    check(report, sequence['completion'] is not None, 'Blitz must return through native completion')
    check(report, len(sequence['contacts']) >= 6, 'Blitz lost expected native multi-hit contacts')
    return sequence


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', type=Path, required=True)
    parser.add_argument('--guest', type=Path, required=True)
    parser.add_argument('--baseline-guest', type=Path)
    parser.add_argument('--rom', type=Path, default=ROOT / 'roms/B8CE.gba')
    parser.add_argument('--out', type=Path, default=ROOT / 'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development/native/out/combat-flow-tests')
    parser.add_argument('--only', help='comma-separated scenario names')
    parser.add_argument('--scale', type=int, default=2, choices=(1, 2, 3, 4))
    parser.add_argument('--preset', type=Path)
    args = parser.parse_args()
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    args.baseline, args.resume = True, False  # Infrastructure only; own assertions below are mandatory.
    guest, host, rom = args.guest.resolve(), args.host.resolve(), args.rom.resolve()
    guest_sha = hashlib.sha256(guest.read_bytes()).hexdigest()
    host_sha = hashlib.sha256(host.read_bytes()).hexdigest()
    config = (args.preset or ROOT / 'mod/action-edition/kh_com.ini').read_text()
    for setting in ('cheat_one_hit', 'cheat_infinite_hp', 'cheat_savestates'):
        if not re.search(r'(?m)^' + setting + r'=0\s*$', config):
            parser.error('native flow checks require ' + setting + '=0')

    tests = []
    normal_ref = run_native(case('flow-reference-normal', ['180-181:A'], 360,
                                 [180, 194, 205, 330]), args, guest, host, rom)
    check(normal_ref, [c['id'] for c in normal_ref['commands']] == [1], 'reference requires one accepted normal')
    check(normal_ref, len(normal_ref['strikes']) == 1 and normal_ref['strikes'][0]['command'] == 1,
          'reference requires one real normal strike')
    check(normal_ref, bool(normal_ref['contacts']), 'reference normal must genuinely hit its target')
    tests.append(write_report(normal_ref, args))
    base = coh.combo_keys()
    blitz_ref = run_native(case('flow-reference-blitz', base, 800,
                                [200, 280, 345, 480, 760]), args, guest, host, rom)
    reference_sequence = full_blitz(blitz_ref)
    tests.append(write_report(blitz_ref, args))
    if not normal_ref['passed'] or not blitz_ref['passed'] or reference_sequence is None:
        result = dict(passed=False, reason='cannot derive probes from valid native references', tests=tests)
        (args.out / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
        return 1

    first_accept = normal_ref['commands'][0]['frame']
    strike = normal_ref['strikes'][0]['frame']
    post = strike + 3
    startup = first_accept + 3
    last_contact = max(h['frame'] for h in reference_sequence['contacts'])
    complete = reference_sequence['completion']['frame']
    late_roll = last_contact + 5
    early_locked = reference_sequence['accepted'] + 12
    specs = [
        case('dense-linked-combo', coh.taps(180, 900, 4), 1300, [195, 240, 300, 450, 750, 1260]),
        case('locked-press-expires', base + [f'{early_locked}-{early_locked + 1}:A'], 800,
             [340, last_contact, complete + 10, 760]),
        case('roll-after-strike', ['180-181:A', f'{post}-{post + 1}:DODGE+RIGHT',
                                  f'{post + 20}-{post + 28}:LEFT', f'{post + 30}-{post + 31}:A+LEFT']),
        case('guard-after-strike', ['180-181:A', f'{post}-{post + 8}:GUARD',
                                   f'{post + 18}-{post + 19}:A']),
        case('roll-startup-reset', ['180-181:A', f'{startup}-{startup + 1}:DODGE+RIGHT',
                                   f'{startup + 20}-{startup + 28}:LEFT', f'{startup + 30}-{startup + 31}:A+LEFT']),
        case('guard-startup-reset', ['180-181:A', f'{startup}-{startup + 8}:GUARD',
                                    f'{startup + 18}-{startup + 19}:A']),
        case('guard-continuation-expiry', ['180-181:A', f'{post}-{post + 90}:GUARD',
                                         f'{post + 100}-{post + 101}:A']),
        case('blitz-buffered-late-roll', base + [f'{late_roll}-{late_roll + 1}:DODGE+LEFT'], 800,
             [last_contact, late_roll + 8, complete + 20, 760]),
        case('wide-depth-opener', ['180-181:A+UP'], 420, [180, 195, 210, 390]),
        case('rising-modifier-opener', ['180-181:A+MODIFIER+UP'], 420, [180, 195, 210, 390]),
        case('wide-opener-connected-route', ['180-181:A+UP'] + coh.taps(186, 330, 4), 1100,
             [195, 220, 260, 320, 450, 700, 1060]),
        case('rising-opener-connected-route', ['180-181:A+MODIFIER+UP'] + coh.taps(186, 330, 4), 1100,
             [195, 220, 260, 320, 450, 700, 1060]),
    ]
    available = {s['name']: s for s in specs}
    wanted = list(dict.fromkeys(args.only.split(','))) if args.only else list(available)
    unknown = set(wanted) - set(available)
    if unknown:
        parser.error('unknown scenario: ' + ','.join(sorted(unknown)))

    for name in wanted:
        report = run_native(available[name], args, guest, host, rom)
        commands = report['commands']
        ids = [c['id'] for c in commands]
        if name == 'dense-linked-combo':
            check(report, len(commands) >= 8 and all(c['id'] == (1, 2, 3, 20)[n % 4]
                  for n, c in enumerate(commands)), 'dense inputs did not produce stable three-piece/finisher cycles')
            check(report, len(report['recovery_link']) >= 2, 'no native recovery-window links observed')
            for link in report['recovery_link']:
                check(report, any(c['frame'] == link['frame'] and c['id'] == link['next'] for c in commands),
                      'recovery-link trace has no matching accepted next action')
                previous = next((c for c in reversed(commands) if c['frame'] < link['frame']), None)
                check(report, previous is not None and any(previous['frame'] <= s['frame'] < link['frame'] and
                      s['command'] == previous['id'] for s in report['strikes']),
                      'a recovery link skipped the preceding native strike')
            completed = [s for s in report['native_sequences'] if s['completion']]
            check(report, len(completed) >= 2 and len(completed) == len(report['native_sequences']),
                  'a dense-input finisher was skipped or truncated')
            for sequence in completed:
                check(report, sequence['segments'] == [1, 2, 3] and len(sequence['contacts']) >= 6,
                      'dense mashing lost native finisher segments/contacts')
                check(report, not any(sequence['accepted'] < c['frame'] < sequence['completion']['frame'] for c in commands),
                      'ordinary offensive input interrupted an active finisher')
        elif name == 'locked-press-expires':
            full_blitz(report)
            check(report, ids == [1, 2, 3, 20], 'old early-locked attack reappeared after the long finisher')
        elif name in ('roll-after-strike', 'guard-after-strike', 'roll-startup-reset', 'guard-startup-reset'):
            kind = 'dodge' if name.startswith('roll') else 'guard'
            probe = startup if 'startup' in name else post
            expected = [1, 1] if 'startup' in name else [1, 2]
            check(report, ids == expected, 'wrong sequence position after defensive interruption')
            cancellations = [c for c in report['deliberate_cancels'] if c['kind'] == kind]
            check(report, len(cancellations) == 1 and probe <= cancellations[0]['frame'] <= probe + 2,
                  'defensive request was not accepted once in its legal phase')
            if cancellations:
                cancel = cancellations[0]['frame']
                before = [s for s in report['strikes'] if s['frame'] < cancel]
                check(report, not before if 'startup' in name else len(before) == 1,
                      'startup/active cancellation did not preserve the intended strike boundary')
        elif name == 'guard-continuation-expiry':
            check(report, ids == [1, 1], 'held guard refreshed combo continuation indefinitely')
            check(report, any(c['kind'] == 'guard' for c in report['deliberate_cancels']), 'no actual guard entry')
        elif name == 'blitz-buffered-late-roll':
            check(report, ids == [1, 2, 3, 20], 'unexpected accepted action during defensive probe')
            cancels = [c for c in report['deliberate_cancels'] if c['kind'] == 'dodge']
            check(report, len(cancels) == 1, 'one-frame defensive tap during protected recovery was lost or repeated')
            if cancels:
                cancel = cancels[0]['frame']
                check(report, late_roll + 2 <= cancel < complete,
                      'dodge did not buffer until a legal pre-neutral recovery phase')
                check(report, cancel - last_contact >= 12, 'dodge bypassed protected final-impact recovery')
                check(report, not any(h['frame'] > cancel for h in report['native_contacts']),
                      'native damage continued after cancellation')
                check(report, not report['native_completions'], 'interrupted finisher later completed or duplicated')
        elif name in ('wide-depth-opener', 'rising-modifier-opener'):
            expected_pattern = 2 if name.startswith('wide') else 1
            check(report, ids == [1], 'an opener must remain one normal strike, not a finisher replacement')
            check(report, [r.get('pattern') for r in report['opener']] == [expected_pattern],
                  'controller gesture did not select its intended native opener')
            check(report, [s['command'] for s in report['strikes']] == [1], 'opener did not execute its native strike')
            check(report, bool(report['animation_completions']), 'single native opener did not finish')
        elif name in ('wide-opener-connected-route', 'rising-opener-connected-route'):
            expected_pattern = 2 if name.startswith('wide') else 1
            check(report, bool(report['opener']) and report['opener'][0].get('pattern') == expected_pattern,
                  'the full route did not start from its intended native opener')
            check(report, len(commands) == 4 and [c['combo'] for c in commands[:3]] == [1, 2, 3]
                  and all(c['id'] in NORMALS for c in commands[:3]) and commands[3]['id'] == 20,
                  'opener route lost a normal position, restarted the combo or replaced it with an early finisher')
            if len(commands) >= 4:
                if name.startswith('wide'):
                    check(report, ids[:3] == [1, 2, 3], 'grounded wide route unexpectedly changed normal form')
                check(report, commands[0]['id'] == 1, 'grounded opener did not begin as normal one')
                for index, command in enumerate(commands[:3]):
                    following = commands[index + 1]['frame']
                    actual_strikes = [s for s in report['strikes'] if
                                      command['frame'] <= s['frame'] < following and s['command'] == command['id']]
                    check(report, len(actual_strikes) == 1, 'normal position did not execute exactly one native strike')
                    check(report, any(c['accepted'] == command['frame'] and c['contact'] is not None
                                      for c in report['contact_commands']),
                          'an intended opener-route normal failed to connect with the training target')
                check(report, commands[3]['name'] in ('Blitz', 'Sonic Blade / Air Rush', 'Air Rush'),
                      'neutral continuation selected an unrelated finisher')
                check(report, len(report['native_sequences']) == 1, 'opener route did not produce exactly one native finisher')
                if len(report['native_sequences']) == 1:
                    sequence = report['native_sequences'][0]
                    expected_segments = 3 if sequence['name'] == 'Blitz' else 7
                    check(report, sequence['segments'] == list(range(1, expected_segments + 1)),
                          'chosen opener-route finisher lost native segments')
                    check(report, sequence['completion'] is not None, 'chosen opener-route finisher did not complete')
                    check(report, len(sequence['contacts']) >= (6 if expected_segments == 3 else 7),
                          'chosen opener-route finisher failed its expected native contacts')
                    if sequence['completion']:
                        check(report, not any(sequence['accepted'] < c['frame'] < sequence['completion']['frame']
                                              for c in commands), 'dense inputs interrupted the chosen finisher')
        tests.append(write_report(report, args))

    comparison = None
    if args.baseline_guest:
        baseline = run_native(case('older-guest-dense-comparison', coh.taps(180, 900, 4), 1300,
                                   [195, 240, 300, 450, 750, 1260]), args,
                              args.baseline_guest.resolve(), host, rom)
        comparison = dict(guest_sha256=hashlib.sha256(args.baseline_guest.read_bytes()).hexdigest(),
                          execution_passed=baseline['passed'], commands=baseline['commands'],
                          strikes=baseline['strikes'], native_sequences=baseline['native_sequences'],
                          recovery_links=baseline['recovery_link'], screenshots=baseline['screenshots'],
                          scope='comparison only; cannot satisfy candidate acceptance criteria')
    result = dict(project='Kingdom Hearts Chain of Memories: Zwebti Mix',
                  guest_sha256=guest_sha, host_sha256=host_sha,
                  guest_unchanged=guest_sha == hashlib.sha256(guest.read_bytes()).hexdigest(),
                  strict_candidate_checks=True, cheats=False, tests=tests,
                  probes=dict(first_accept=first_accept, first_strike=strike,
                              post_strike_defense=post, startup_defense=startup,
                              last_blitz_contact=last_contact, blitz_complete=complete,
                              buffered_late_roll=late_roll), baseline_comparison=comparison,
                  scope='Actual native game and ordinary timed inputs in isolated profiles; physical controller feel unverified')
    result['passed'] = result['guest_unchanged'] and all(r['passed'] for r in tests)
    (args.out / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
