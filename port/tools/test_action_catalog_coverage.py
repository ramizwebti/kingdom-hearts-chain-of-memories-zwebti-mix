"""Ensure catalog covers exactly Sora's concrete native MoveId switch cases."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def main():
    text = (ROOT / 'src/btl/btl_sora.c').read_text(encoding='utf-8')
    start = text.index('case BTL_REACTION_CARD_ACTION:')
    match = re.search(r'switch\s*\(move\)\s*\{', text[start:])
    assert match, 'Native Sora dispatcher missing'
    a = start + match.end()
    depth, b = 1, a
    while depth:
        depth += (text[b] == '{') - (text[b] == '}')
        b += 1
    native = set(re.findall(r'case\s+(MOVE_\w+)\s*:', text[a:b]))
    idle = {'MOVE_SLEIGHT_57', 'MOVE_SLEIGHT_68'}
    assert idle <= native
    native -= idle
    catalog_text = (ROOT / 'port/src/port_action_catalog.c').read_text(encoding='utf-8')
    rows = re.findall(r'E\((MOVE_\w+),', catalog_text)
    catalog = set(rows)
    assert len(rows) == len(catalog) == 124
    assert native == catalog, {'missing': sorted(native - catalog), 'extra': sorted(catalog - native)}
    assert 'work->summonDesc = ' in text[a:b] and 'SPELL_TIER_GA' in text[a:b]
    # Channel ownership includes later states of a technique, not just its
    # initial pose: Sonic and Zantetsuken both start shared effects later.
    chains = {
        'MOVE_SLIDING_DASH': ('SLIDING_DASH', 'SLIDING_DASH_SLIDE', 'SLIDING_DASH_END'),
        'MOVE_BLITZ': ('BLITZ', 'BLITZ_STRIKE'),
        'MOVE_ARS_ARCANUM': ('ARS_ARCANUM', 'ARS_ARCANUM_FINISH'),
        'MOVE_SONIC_BLADE': ('SONIC_BLADE',),
        'MOVE_ZANTETSUKEN': ('ZANTETSUKEN', 'ZANTETSUKEN_SLASH', 'ZANTETSUKEN_END'),
        'MOVE_RAGNAROK': ('RAGNAROK', 'RAGNAROK_JUMP', 'RAGNAROK_HOVER', 'RAGNAROK_CHARGE', 'RAGNAROK_SHOT'),
        'MOVE_STUN_IMPACT': ('STUN_IMPACT',),
        'MOVE_FIRAGA_BREAK': ('FIRAGA_BREAK',),
        'MOVE_SHOCK_IMPACT': ('SHOCK_IMPACT',),
    }
    for move in ('STRIKE_RAID', 'FIRE_RAID', 'BLIZZARD_RAID', 'THUNDER_RAID',
                 'GRAVITY_RAID', 'STOP_RAID', 'REFLECT_RAID', 'JUDGMENT'):
        chains['MOVE_' + move] = ('RAID', 'RAID_CATCH')
    assert len(chains) == 17
    for move, states in chains.items():
        shared = False
        for state in states:
            match = re.search(r'^    case BTL_SORA_STATE_' + state + r':(.*?)(?=^    case BTL_SORA_STATE_|\Z)', text, re.M | re.S)
            assert match, 'Missing native technique state ' + state
            shared |= bool(re.search(r'\bBgFxStart\w*\s*\(', match[1]))
        row = re.search(r'^\s*E\(' + move + r',[^\n]+', catalog_text, re.M)
        assert shared == ('PORT_CATALOG_EFFECT_CHANNEL' in row[0]), 'Wrong shared effect metadata: ' + move
    print('PASS: all 124 concrete native Sora dispatcher cases represented exactly; two idle placeholders excluded. This is source coverage, not execution proof.')
    print('PASS: all 17 technique channel flags match complete native state chains, including Sonic and Zantetsuken delayed effects.')


if __name__ == '__main__':
    main()
