#!/usr/bin/env python3
"""Require actual Ariel/Sora overlap while preserving single-owner hit counts.

The isolated single-barrel reference distinguishes Sora's three real normals
and six Blitz contacts from Ariel damage. No state writes, fake hits or damage
cheats. --analyze-existing only reads the saved reports/logs, launching no game.
Use test_action_assist_phases.py for explicit production pose-boundary checks;
these encounter traces do not include per-frame Sora pose/altitude.
"""
import argparse
import hashlib
import json
from pathlib import Path

import test_action_assist_native as assists

ROOT = Path(__file__).resolve().parents[2]
DEV = ROOT / 'build/Kingdom-Hearts-Chain-of-Memories-Zwebti-Mix-Development'
BASE = '180-181:A,220-221:A,260-261:A,310-311:A'
CASES = [
    dict(name='physical-reference', index=None, preset='training', frames=800,
         keys=BASE, shots=[200, 350, 500, 760]),
    dict(name='ariel-probe-blitz', index=87, preset='training', frames=1000,
         keys='140-141:TECHNIQUE,' + BASE, shots=[200, 270, 345, 370, 480, 900]),
    dict(name='ariel-probe-late', index=87, preset='training', frames=900,
         keys='180-181:A,220-221:A,224-225:TECHNIQUE,260-261:A,310-311:A',
         shots=[270, 350, 420, 490, 800]),
]


def check(report, condition, message):
    if not condition:
        report['failures'].append(message)


def paused_at(data, frame):
    changes = [r for r in data['assist-pause'] if r['frame'] <= frame]
    return bool(changes and changes[-1]['paused'])


def analyze(case, out):
    folder = out / case['name']
    report = json.loads((folder / 'report.json').read_text())
    report['failures'] = list(report['failures'])
    check(report, report['case'] == case, 'saved encounter inputs differ from this fixture')
    log = (folder / 'kh_com.log').read_text(errors='replace')
    data = report['data']
    for kind in ('strike', 'melee-contact'):
        data[kind] = assists.rows(log, kind)
    check(report, [r['id'] for r in data['accepted']] == [1, 2, 3, 20],
          'requires exactly three normals followed by native Blitz')
    check(report, [r['command'] for r in data['strike']] == [1, 2, 3],
          'a normal did not execute its real strike')
    check(report, [r['command'] for r in data['melee-contact']] == [1, 2, 3],
          'single-target normal contact missing or reopened by companion reset')
    check(report, [r['step'] for r in data['melee-contact']] == [1, 2, 3],
          'normal contact progression differs from three authored strikes')
    check(report, [r['segment'] for r in data['native-segment']] == [1, 2, 3],
          'Blitz did not preserve all three native segments')
    check(report, len(data['native-complete']) == 1, 'Blitz did not naturally complete once')
    check(report, len(data['finisher-contact']) == 6 and
          all(r['pose'] == 6 and r['attack'] == 93 for r in data['finisher-contact']),
          'Blitz lost or gained contacts outside its six native strikes')
    check(report, all(r['visible'] for r in data['assist-state']), 'Sora hidden during companion update')
    if case['index'] is not None:
        check(report, len(data['assist-start']) == len(data['assist-end']) == 1,
              'real native companion did not start and finish once')
        check(report, bool(data['assist-contact']), 'companion never dealt actual collision damage')
        check(report, bool(data['assist-state']) and
              all(r['effect'] == r['companion'] == r['fade'] == r['tiles'] == 0
                  for r in data['assist-end']), 'native companion resources did not clean up')
        for hit in data['melee-contact']:
            if data['assist-start'] and data['assist-start'][0]['frame'] <= hit['frame']:
                check(report, paused_at(data, hit['frame']), 'Ariel ran inside Sora normal active phase')
        # An emitted assist-state proves the real native task advanced. Its
        # sparse snapshots can establish startup/recovery overlap, not every pose.
        normals = [r for r in data['accepted'] if r['id'] <= 3]
        starts = [(r['frame'], hit['frame']) for r, hit in zip(normals, data['strike'])]
        recoveries = [(hit['frame'], done['frame']) for hit, done in
                      zip(data['strike'], [r for r in data['animation-complete'] if r['command'] <= 3])]
        report['startup_updates'] = [r['frame'] for r in data['assist-state']
                                     if any(a <= r['frame'] < b for a, b in starts)]
        report['recovery_updates'] = [r['frame'] for r in data['assist-state']
                                      if any(a < r['frame'] <= b for a, b in recoveries)]
        if case['name'] == 'ariel-probe-blitz':
            check(report, bool(report['startup_updates']), 'no native companion update in normal startup')
            check(report, bool(report['recovery_updates']), 'no native companion update in normal recovery')
        if case['name'] == 'ariel-probe-late':
            contacts = data['finisher-contact']
            check(report, bool(data['assist-end']) and bool(contacts) and
                  data['assist-end'][0]['frame'] > contacts[-1]['frame'],
                  'companion must remain alive across every Blitz strike window')
            for hit in contacts:
                check(report, paused_at(data, hit['frame']), 'companion advanced in Blitz active strike')
            finisher = data['accepted'][-1]['frame'] if data['accepted'] else 0
            complete = data['native-complete'][-1]['frame'] if data['native-complete'] else 0
            check(report, any(finisher < r['frame'] < complete and not paused_at(data, r['frame'])
                              for r in data['assist-contact']),
                  'no genuine companion collision during safe Blitz overlap')
            check(report, len([r for r in data['assist-pause'] if r['player'] and r['paused']]) == 3,
                  'expected isolated pause across each of three Blitz strike windows')
    report['passed'] = not report['failures']
    (folder / 'physical-report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', type=Path, default=DEV / 'native/out/rekhcom64.exe')
    parser.add_argument('--guest', type=Path, default=ROOT / 'build/windows64/out/guest-us.bin')
    parser.add_argument('--rom', type=Path, default=ROOT / 'roms/B8CE.gba')
    parser.add_argument('--out', type=Path, default=DEV / 'validation/physical-assists')
    parser.add_argument('--analyze-existing', action='store_true')
    args = parser.parse_args()
    out = args.out.resolve()
    if not out.is_relative_to((ROOT / 'build').resolve()):
        parser.error('--out must stay inside this checkout build directory')
    paths = dict(host_sha256=args.host.resolve(), guest_sha256=args.guest.resolve())
    fingerprint = {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in paths.items()}
    assists.HOST, assists.GUEST, assists.ROM, assists.OUT = args.host.resolve(), args.guest.resolve(), args.rom.resolve(), out
    if not args.analyze_existing:
        for case in CASES:
            assists.run(case)
    reports = [analyze(case, out) for case in CASES]
    reference = reports[0]
    for report in reports[1:]:
        check(report, report['guest_sha256'] == reference['guest_sha256'] == fingerprint['guest_sha256'],
              'reference and assist probes must use this exact unchanged guest')
        check(report, len(report['data']['finisher-contact']) == len(reference['data']['finisher-contact']),
              'assist changed native Blitz contact count relative to no-assist reference')
        report['passed'] = not report['failures']
    unchanged = all(hashlib.sha256(path.read_bytes()).hexdigest() == fingerprint[name] for name, path in paths.items())
    result = dict(passed=all(r['passed'] for r in reports) and unchanged, binaries_unchanged=unchanged,
                  **fingerprint, cheats=False, rom_copied=False, pose_boundary_fixture='separate; not established by encounter traces',
                  tests=reports)
    (out / 'physical-report.json').write_text(json.dumps(result, indent=2) + '\n')
    for report in reports:
        print(json.dumps(dict(name=report['case']['name'], passed=report['passed'], failures=report['failures'])), flush=True)
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
