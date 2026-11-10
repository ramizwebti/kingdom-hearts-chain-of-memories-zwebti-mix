#!/usr/bin/env python3
"""Reproduce a source-only native animation/effect inventory; never opens a ROM.

The intentionally narrow manifest reader handles authored entry/sprite/timeline
records and preserves unfamiliar records as metadata. C initializers are parsed
with balanced delimiters; conditional source variants are retained and labeled.
This inventory describes source availability, not arbitrary actor interchange.
"""
import argparse
import ast
import bisect
from collections import Counter, defaultdict
import hashlib
import html
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
DATA_TYPES = ('AnimDef', 'FldAnimDef', 'SoraAttackDef', 'RikuAttackDef',
              'BgAnimationDef', 'BgAnimationChunk', 'TaskDesc',
              'EventBgEffectDef', 'EventBgEffectFrame', 'MapTileAnimationDef',
              'MapTileAnimationTrack', 'MapTileAnimationFrame', 'BattleAttackDef',
              'EmyDef', 'HumDef', 'HumSubDef', 'EnemyBaseStats')
VISUAL_FORMATS = {'sprite_sheet', 'palette', 'tiles4', 'tiles8', 'tilemap4',
                  'tilemap8', 'tilemap', 'bg_map', 'bitmap', 'lz_tiles4', 'lz_tiles8', 'glyphs'}
VISUAL_RECORDS = {'SpriteTable', 'AnimTable', 'Sprite', 'AnimHeader', 'PcSpriteTable',
                  'EventBackgroundDef', 'EventMapObjectDef', 'EventMapObjectPlacement',
                  'PrizeMapCardGroupList', 'MapAnmEntry', 'MapAnmScript', 'UnkStruct_080E87EC'}
RESOURCE_CALL = re.compile(r'\b((?:BgFx|BgAnim|Fade|SetBattleZoom|BtlMapFollow|WorldToScreen|TaskCreate|TaskPool|LoadObj|AllocObj|ReleaseObj|ApplyAttack|MakeOpponents|RequestSora|RequestRiku|CreateBtl|m4aSongNum)\w*)\s*\(')
ASSET_TOKEN = re.compile(r'\b(?:[gs]\w*(?:Tiles\w*|Frames|Anims|Palette\w*)|gTaskDesc\w+|[gs]BgAnimDef\w+)\b')


def read(path):
    return path.read_text(encoding='utf-8', errors='replace')


def comment_mask(text):
    return re.sub(r'/\*.*?\*/|//[^\n]*', lambda m: re.sub(r'[^\n]', ' ', m[0]), text, flags=re.S)


def matching(text, opening, left='{', right='}'):
    depth, quote, escaped = 0, None, False
    for i in range(opening, len(text)):
        ch = text[i]
        if quote:
            if escaped:
                escaped = False
            elif ch == '\\':
                escaped = True
            elif ch == quote:
                quote = None
            continue
        if ch in ('"', "'"):
            quote = ch
        elif ch == left:
            depth += 1
        elif ch == right:
            depth -= 1
            if not depth:
                return i + 1
    raise ValueError('unbalanced source delimiter at ' + str(opening))


def split_c(text):
    parts, start, depth = [], 0, 0
    for i, ch in enumerate(text):
        if ch in '({[':
            depth += 1
        elif ch in ')}]':
            depth -= 1
        elif ch == ',' and depth == 0:
            parts.append(text[start:i].strip())
            start = i + 1
    if text[start:].strip():
        parts.append(text[start:].strip())
    return parts


def number(value):
    try:
        return int(value.strip(), 0)
    except (ValueError, AttributeError):
        return None


def family(symbol, group):
    known = {'Sor': 'Sora', 'Riku': 'Riku', 'Croud': 'Cloud', 'Cloud': 'Cloud',
             'Aladdin': 'Aladdin', 'Ariel': 'Ariel', 'Jack': 'Jack', 'Goofy': 'Goofy',
             'Donald': 'Donald', 'Shinba': 'Simba', 'Simba': 'Simba', 'Genie': 'Genie',
             'Tink': 'Tinker Bell', 'SmnTink': 'Tinker Bell', 'Mushu': 'Mushu',
             'Bambi': 'Bambi', 'Beast': 'Beast', 'PeterPan': 'Peter Pan', 'Pan': 'Peter Pan'}
    clean = re.sub(r'^[gs]', '', symbol)
    for prefix, label in known.items():
        if clean.startswith(prefix):
            return label
    return group


class Builder:
    def __init__(self):
        self.entries = []
        self.inputs = []
        self.coverage = Counter()
        self.unresolved = []
        self.parse_notes = []
        self.sources = {}
        self.functions = []

    def source(self, path):
        rel = path.relative_to(ROOT).as_posix()
        text = read(path)
        self.inputs.append(dict(path=rel, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
        self.sources[rel] = text
        offsets = [-1] + [m.start() for m in re.finditer('\n', text)]
        return rel, text, lambda pos: bisect.bisect_left(offsets, pos)

    def add(self, kind, symbol, source, line, group, **fields):
        entry = dict(id='%s:%s:%s:%s' % (kind, source, line, symbol), kind=kind,
                     symbol=symbol, source=source, line=line, group=group,
                     native_family=family(symbol, group), dependency_symbols=[], usages=[], **fields)
        self.entries.append(entry)
        return entry

    def manifests(self):
        for path in sorted((ROOT / 'config/assets').glob('*.yaml')):
            rel, text, line = self.source(path)
            self.coverage['asset_manifests'] += 1
            group_match = re.search(r'^group:\s*(.+)', text, re.M)
            group = group_match[1] if group_match else path.stem
            section = re.search(r'^entries:\s*$', text, re.M)
            if not section:
                self.unresolved.append(dict(source=rel, reason='manifest has no entries section'))
                continue
            records = list(re.finditer(r'^- name:\s*([^\n]+)\n', text[section.end():], re.M))
            for i, match in enumerate(records):
                base = section.end() + match.start()
                end = section.end() + records[i + 1].start() if i + 1 < len(records) else len(text)
                block = text[base:end]
                name = match[1].strip('"\' ')
                fields = dict(re.findall(r'^  (format|record|symbol|palette|tiles|width|height|cell|anchor|columns|items):\s*([^\n]+)', block, re.M))
                fmt = fields.get('format', fields.get('record', 'unclassified'))
                self.coverage['all_manifest_entry_records'] += 1
                self.coverage['manifest_format:' + fmt] += 1
                # Nonvisual records (songs, messages, rooms) remain counted in
                # scope coverage rather than being mislabeled as effects.
                raw_palette = fmt == 'raw' and 'Palette' in fields.get('symbol', name)
                movie = group == 'movies'
                visual = raw_palette or movie or fmt in VISUAL_FORMATS or fmt in VISUAL_RECORDS or any(x in group for x in ('bg_animation', 'tile_animation'))
                if not visual:
                    continue
                symbol = fields.get('symbol', name)
                kind = {'sprite_sheet': 'sprite-sheet', 'palette': 'palette', 'SpriteTable': 'sprite-table', 'AnimTable': 'animation-table', 'AnimHeader': 'authored-animation-header', 'Sprite': 'authored-oam-sprite', 'PcSpriteTable': 'custom-sprite-table', 'UnkStruct_080E87EC': 'native-map-pattern-record'}.get(fmt, 'visual-resource')
                if raw_palette:
                    kind = 'raw-palette-resource'
                if movie:
                    kind = 'native-movie-resource'
                entry = self.add(kind, symbol, rel, line(base), group, name=name, format=fmt)
                self.coverage['visual_manifest_entries'] += 1
                entry['regions'] = re.findall(r'^  (us|jp|eu):', block, re.M)
                entry['authored_metadata'] = {key: value for key, value in fields.items() if key not in ('format', 'symbol')}
                entry['sizes_bytes'] = {region: int(size) for region, size in re.findall(r'^  (us|jp|eu):\s*\n(?:    [^\n]+\n)*?    size:\s*(\d+)', block, re.M)}
                if fmt == 'palette' or raw_palette:
                    entry['native_color_format'] = 'GBA BGR555; numerical cartridge colors are not read or copied'
                    entry['color_counts'] = {region: size // 2 for region, size in entry['sizes_bytes'].items()}
                if fmt in ('AnimTable', 'SpriteTable', 'PcSpriteTable'):
                    entry['items'] = re.findall(r'\b[gs]\w+\b', fields.get('items', ''))
                    entry['dependency_symbols'] = entry['items'][:]
                else:
                    # Preserve visual record references, including event maps,
                    # map-animation scripts and special boss sprite tables.
                    entry['dependency_symbols'] = sorted(set(re.findall(r'\b[gs][A-Z]\w*', block)) - {symbol})
                if fmt in ('AnimHeader', 'Sprite'):
                    rows = [ast.literal_eval(value) for value in re.findall(r'^  - (\[[^\n]*\])', block, re.M)]
                    entry['timeline' if fmt == 'AnimHeader' else 'authored_oam'] = rows
                    if fmt == 'AnimHeader':
                        entry['duration_ticks'] = sum(row[1] for row in rows)
                        self.coverage['standalone_authored_animation_headers'] += 1
                    else:
                        self.coverage['standalone_authored_oam_sprites'] += 1
                if fmt not in ('sprite_sheet', 'palette', 'AnimTable', 'SpriteTable', 'AnimHeader', 'Sprite'):
                    entry['record_fields'] = dict(re.findall(r'^    (\w+):\s*([^\n]+)', block, re.M))
                if fmt != 'sprite_sheet':
                    continue
                entry['palette_name'] = fields.get('palette')
                entry['texture_symbol'] = fields.get('tiles', symbol)
                frames_start = re.search(r'^  frames:\s*$', block, re.M)
                animations_start = re.search(r'^  animations:\s*$', block, re.M)
                if not frames_start or not animations_start:
                    self.unresolved.append(dict(source=rel, line=line(base), symbol=symbol, reason='sprite sheet lacks frames or animation section'))
                    continue
                frame_section = block[frames_start.end():animations_start.start()]
                animation_section = block[animations_start.end():]
                sheet_frames, sheet_animations = [], []
                for child in re.finditer(r'^  - symbol:\s*(\w+)\n(.*?)(?=^  - symbol:|\Z)', frame_section, re.M | re.S):
                    frame = self.add('sprite-frame', child[1], rel, line(base + frames_start.end() + child.start()), group,
                                     sheet=symbol, pieces=[], **{})
                    for piece in re.finditer(r'^    - \[(\d+)x(\d+),\s*(-?\d+),\s*(-?\d+)([^\n]*)', child[2], re.M):
                        attrs = {key: value == 'true' if value in ('true', 'false') else int(value)
                                 for key, value in re.findall(r'(\w+):\s*(true|false|-?\d+)', piece[5])}
                        frame['pieces'].append(dict(width=int(piece[1]), height=int(piece[2]), x=int(piece[3]), y=int(piece[4]), attributes=attrs))
                    frame['dependency_symbols'] = [symbol]
                    sheet_frames.append(child[1])
                for child in re.finditer(r'^  - symbol:\s*(\w+)\n(.*?)(?=^  - symbol:|^  (?:us|jp|eu):|\Z)', animation_section, re.M | re.S):
                    anim = self.add('sprite-animation', child[1], rel, line(base + animations_start.end() + child.start()), group, sheet=symbol)
                    timeline = re.search(r'^    frames:\s*(\[[^\n]*\])', child[2], re.M)
                    if not timeline:
                        self.unresolved.append(dict(source=rel, symbol=child[1], reason='animation timeline is not an authored one-line sequence'))
                        continue
                    try:
                        anim['timeline'] = ast.literal_eval(timeline[1])
                        anim['duration_ticks'] = sum(row[1] for row in anim['timeline'])
                        indexes = [row[0] for row in anim['timeline']]
                        anim['dependency_symbols'] = [sheet_frames[index] for index in indexes if 0 <= index < len(sheet_frames)]
                        bad = sorted(set(index for index in indexes if not 0 <= index < len(sheet_frames)))
                        if bad:
                            anim['unresolved_frame_indexes'] = bad
                            self.unresolved.append(dict(source=rel, symbol=child[1], reason='authored frame indexes outside parsed sheet', indexes=bad))
                    except (SyntaxError, ValueError, TypeError) as error:
                        self.unresolved.append(dict(source=rel, symbol=child[1], reason='unparsed timeline: ' + str(error)))
                    sheet_animations.append(child[1])
                entry['frame_symbols'] = sheet_frames
                entry['animation_symbols'] = sheet_animations
                entry['frame_count'] = len(sheet_frames)
                entry['animation_count'] = len(sheet_animations)
                entry['dependency_symbols'] = sheet_animations + ([entry['texture_symbol']] if entry['texture_symbol'] != symbol else [])
                self.coverage['authored_sprite_frames'] += len(sheet_frames)
                self.coverage['authored_sprite_animations'] += len(sheet_animations)

    def c_sources(self):
        declaration = re.compile(r'\b(' + '|'.join(DATA_TYPES) + r')\s+(\w+)\s*((?:\[[^\]]*\]\s*)*)=\s*\{')
        function_re = re.compile(r'^[ \t]*(?:(?:static|inline|const|extern)\s+)*(?:void|u8|u16|u32|s8|s16|s32|int|unsigned|BtlObj|Task|[A-Z]\w*Work)\s*\**\s*(\w+)\s*\(([^;{}]*?)\)\s*\{', re.M)
        for path in sorted((ROOT / 'src').rglob('*.c')):
            rel, text, line = self.source(path)
            self.coverage['native_c_files'] += 1
            clean = comment_mask(text)
            group = path.parent.name + '/' + path.stem
            functions = list(function_re.finditer(clean))
            for fi, match in enumerate(functions):
                boundary = functions[fi + 1].start() if fi + 1 < len(functions) else len(clean)
                try:
                    end = matching(clean, match.end() - 1)
                    if end > boundary:
                        raise ValueError('conditional source branches cross function boundary')
                except ValueError:
                    end = boundary
                    self.parse_notes.append(dict(source=rel, symbol=match[1], reason='conditional/unbalanced physical C branches: callback usage bounded by next function definition'))
                body = text[match.end():end - 1]
                self.functions.append(dict(name=match[1], signature=text[match.start():match.end() - 1].strip(), source=rel,
                                           line=line(match.start()), body_line=line(match.end()), end_line=line(end), body=body,
                                           calls=sorted(set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', comment_mask(body))) - {'if', 'for', 'while', 'switch', 'sizeof'}),
                                           resources=sorted(set(RESOURCE_CALL.findall(comment_mask(body)))),
                                           assets=sorted(set(ASSET_TOKEN.findall(comment_mask(body))))))
            for match in declaration.finditer(clean):
                end = matching(clean, match.end() - 1)
                initializer = text[match.end() - 1:end]
                ctype, name, dimensions = match.group(1, 2, 3)
                conditional = bool(re.search(r'^\s*#', initializer, re.M))
                fields_text = re.sub(r'^\s*#[^\n]*', '', comment_mask(initializer), flags=re.M)
                leaves = list(re.finditer(r'\{([^{}]*)\}', fields_text))
                self.coverage['C_declarations:' + ctype] += 1
                kind = {'TaskDesc': 'native-task', 'BgAnimationDef': 'background-animation',
                        'EventBgEffectDef': 'event-background-effect', 'SoraAttackDef': 'sora-attack-pattern',
                        'RikuAttackDef': 'riku-attack-pattern'}.get(ctype, 'native-animation-table')
                if ctype == 'TaskDesc' and re.search(r'"task_action_', initializer):
                    kind = 'mod-task-bridge'
                rows = [split_c(leaf[1]) for leaf in leaves]
                entry = self.add(kind, name, rel, line(match.start()), group, native_type=ctype,
                                 declared_dimensions=re.findall(r'\[([^\]]*)\]', dimensions),
                                 conditional_source_variants=conditional, rows=rows,
                                 initializer_expression=initializer.strip())
                entry['dependency_symbols'] = sorted(set(ASSET_TOKEN.findall(initializer)))
                if ctype == 'TaskDesc':
                    entry['callbacks'] = re.findall(r'\(Task(?:Init|Update|Draw|Destroy)Func\)\s*(\w+)', initializer)
                    label = re.search(r'"([^"]+)"', initializer)
                    entry['name'] = label[1] if label else name
                if ctype in ('AnimDef', 'FldAnimDef'):
                    for index, row in enumerate(rows):
                        if len(row) == 4:
                            item = self.add('native-animation-binding', name + '[%d]' % index, rel, line(match.start()), group,
                                            table=name, index=index, frames_table=row[0], animations_table=row[1],
                                            tiles=row[2], anim_id_expression=row[3], native_type=ctype)
                            item['dependency_symbols'] = row[:3]
                if ctype == 'BgAnimationDef' and len(rows) == 1 and len(rows[0]) == 9:
                    keys = ('chunks', 'tilemap', 'palette', 'palette_bytes_expression', 'tiles_per_frame', 'origin_x', 'origin_y', 'frame_count', 'frame_duration')
                    entry['animation'] = dict(zip(keys, (number(value) if number(value) is not None else value for value in rows[0])))
                    n, d = number(rows[0][-2]), number(rows[0][-1])
                    entry['duration_ticks'] = n * d if n is not None and d is not None and d != 65535 else None
                    entry['lifetime_note'] = '65535 sentinel/held animation: owning updater determines lifetime' if d == 65535 else 'Authored frame span; owning updater may loop/release early'
                if ctype == 'BattleAttackDef':
                    for index, row in enumerate(rows):
                        keys = ('power', 'knockback_speed', 'knockback_lift', 'hit_stop_ticks', 'hit_effect', 'flags_expression')
                        attack = self.add('native-attack-definition', name + '[%d]' % index, rel,
                                          line(match.start()), group, attack_id=index,
                                          parameters=dict(zip(keys, [number(v) if number(v) is not None else v for v in row])))
                        attack['elements'] = re.findall(r'ATTACK_FLAG_ELEMENT_(\w+)', row[-1])
                        attack['statuses'] = re.findall(r'ATTACK_FLAG_INFLICT_(\w+)', row[-1])
                        attack['kind_flags'] = re.findall(r'ATTACK_FLAG_KIND_(\w+)', row[-1])
                        attack['dependency_symbols'] = [row[4]] if row[4] != 'NULL' else []
                        attack['playability'] = 'Shared native damage definition. Authored callsites identify actors; not a standalone Sora technique.'

    def enrich_c(self):
        by_function = defaultdict(list)
        for fn in self.functions:
            by_function[fn['name']].append(fn)
        for fn in self.functions:
            if fn['name'].startswith('BgFxStart'):
                entry = self.add('background-effect-api', fn['name'], fn['source'], fn['line'], 'battle BGFX',
                                 signature=fn['signature'], called_functions=fn['calls'], resource_calls=fn['resources'])
                updates = re.findall(r'sBgFx->update\s*=\s*(\w+)', fn['body'])
                entry['native_update_callbacks'] = updates
                entry['dependency_symbols'] = fn['assets']
                combined = fn['body'] + ''.join(other['body'] for update in updates for other in by_function.get(update, []))
                entry['lifecycle'] = self.hazards(combined)
                entry['lifecycle']['shared_bgfx_singleton'] = True
                self.coverage['native_bgfx_start_definitions'] += 1
        for entry in self.entries:
            if entry['kind'] not in ('native-task', 'mod-task-bridge'):
                continue
            callbacks = [fn for callback in entry['callbacks'] for fn in by_function.get(callback, []) if fn['source'] == entry['source']]
            entry['callback_signatures'] = [fn['signature'] for fn in callbacks]
            entry['resource_calls'] = sorted(set(call for fn in callbacks for call in fn['resources']))
            entry['dependency_symbols'] = sorted(set(entry['dependency_symbols'] + [symbol for fn in callbacks for symbol in fn['assets']]))
            entry['lifecycle'] = self.hazards('\n'.join(fn['body'] for fn in callbacks))
            entry['missing_callback_definitions'] = [name for name in entry['callbacks'] if not any(fn['name'] == name for fn in callbacks)]
            entry['mod_callback_overrides'] = [name for name in entry['callbacks'] if 'Action' in name]
        # Exact authored C animation-id binding -> manifest timeline.
        tables = {entry['symbol']: entry for entry in self.entries if entry['kind'] == 'animation-table'}
        for entry in self.entries:
            if entry['kind'] != 'native-animation-binding':
                continue
            index = number(entry['anim_id_expression'])
            table = tables.get(entry['animations_table'])
            if index is not None and table and index < len(table['items']):
                entry['resolved_animation'] = table['items'][index]
                entry['dependency_symbols'].append(entry['resolved_animation'])

    @staticmethod
    def hazards(body):
        return dict(shared_bgfx_singleton='BgFx' in body or 'BgAnim' in body,
                    changes_camera=bool(re.search(r'BtlMapFollow|FocusBtl|SetBattleZoom', body)),
                    changes_global_fade='Fade' in body,
                    changes_enemy_movement='BTL_FLAG_ENEMY_MOVE_ENABLED' in body,
                    uses_sora_tile_slot=bool(re.search(r'\btiles\s*=\s*g(?:Riku)?BtlWork->tiles\b', body)),
                    uses_shared_friend_tile_slot=bool(re.search(r'\btiles\s*=\s*g(?:Riku)?BtlWork->tiles2\b', body)),
                    owns_obj_tiles=bool(re.search(r'(?:Load|Alloc)ObjTiles\(', body)),
                    releases_obj_tiles='ReleaseObjTiles(' in body,
                    owns_obj_palette='LoadObjPalette(' in body,
                    releases_obj_palette='ReleaseObjPalette(' in body,
                    owns_child_task_pool='TaskPoolInit(' in body,
                    destroys_child_task_pool='TaskPoolDestroy(' in body,
                    retains_actor_pointer=bool(re.search(r'work->(?:actor|target|obj)\s*=', body)),
                    direct_attack_calls=sorted(set(re.findall(r'\b(ApplyAttack\w*)\s*\(', body))),
                    card_reload_dependencies=sorted(set(re.findall(r'\b(Request(?:Sora|Riku)\w*)\s*\(', body))),
                    depends_on_event_or_field=bool(re.search(r'gEventState|gFieldState|EventCharaWork|FldObj', body)),
                    input_reads=sorted(set(re.findall(r'\b(GetKeys\w*|GetKey\w*)\s*\(', body))),
                    note='Static source dependencies; absence of a listed call is not runtime interchangeability proof')

    def sora(self):
        path = ROOT / 'include/card/card_ids.h'
        rel, text, line = self.source(path)
        enum = re.search(r'enum MoveId\s*\{(.*?)\};', text, re.S)
        ids, current = {}, -1
        for match in re.finditer(r'\b(MOVE_\w+)\s*(?:=\s*(0x[0-9a-fA-F]+|\d+))?\s*,?', enum[1]):
            current = int(match[2], 0) if match[2] else current + 1
            ids[match[1]] = current
            self.add('native-move-id', match[1], rel, line(enum.start(1) + match.start()), 'Shared native MoveId registry', native_move_id=current)
        self.coverage['all_native_move_enum_members'] = len(ids)
        source = 'src/btl/btl_sora.c'
        sora = self.sources[source]
        clean = comment_mask(sora)
        start = clean.index('switch (move)', clean.index('case BTL_REACTION_CARD_ACTION:'))
        opening = clean.index('{', start)
        end = matching(clean, opening)
        switch = sora[opening + 1:end - 1]
        cases = list(re.finditer(r'case\s+(MOVE_\w+)\s*:', switch))
        catalog_path = ROOT / 'port/src/port_action_catalog.c'
        cat_rel, catalog, cat_line = self.source(catalog_path)
        catalog_rows = {match[1]: match for match in re.finditer(r'\bE\((MOVE_\w+),"([^"]+)",([^\n]+)', catalog)}
        for i, match in enumerate(cases):
            symbol = match[1]
            body = switch[match.end():cases[i + 1].start() if i + 1 < len(cases) else len(switch)]
            row = catalog_rows.get(symbol)
            entry = self.add('sora-native-move', symbol, source, sora.count('\n', 0, opening + 1 + match.start()) + 1,
                             'Sora native dispatcher', native_move_id=ids[symbol], name=row[2] if row else symbol,
                             action_catalog_member=row is not None,
                             native_state_assignments=sorted(set(re.findall(r'\bBTL_SORA_STATE_\w+', body))),
                             native_variants=re.findall(r'work->variant\[0\]\s*=\s*(\w+)', body),
                             initializer_calls=sorted(set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', body)) - {'sizeof'}))
            entry['dependency_symbols'] = sorted(set(ASSET_TOKEN.findall(body)))
            if row:
                entry['selection_metadata_source'] = dict(path=cat_rel, line=cat_line(row.start()))
                entry['selection_metadata'] = row[3].rstrip('),')
            if symbol in ('MOVE_SLEIGHT_57', 'MOVE_SLEIGHT_68'):
                entry['availability_note'] = 'Explicit idle placeholder, not a concrete player action'
        self.coverage['sora_move_case_labels'] = len(cases)
        self.coverage['sora_action_catalog_members'] = sum(entry['kind'] == 'sora-native-move' and entry.get('action_catalog_member', False) for entry in self.entries)
        sora_symbols = {match[1] for match in cases}
        for entry in self.entries:
            if entry['kind'] == 'native-move-id':
                entry['sora_dispatcher_member'] = entry['symbol'] in sora_symbols
                entry['role'] = 'Sora concrete native dispatcher' if entry['symbol'] in catalog_rows else 'Idle placeholder' if entry['symbol'] in sora_symbols else 'Other actor, scripted, stock or gimmick registry member; not a Sora direct action'
        enum_match = re.search(r'enum BtlSoraState\s*\{(.*?)\};', sora, re.S)
        for match in re.finditer(r'\b(BTL_SORA_STATE_\w+)\b', enum_match[1]):
            symbol = match[1]
            blocks = list(re.finditer(r'^    case ' + symbol + r':(.*?)(?=^    case BTL_SORA_STATE_|\Z)', sora, re.M | re.S))
            body = '\n'.join(block[1] for block in blocks)
            entry = self.add('sora-state', symbol, source, sora.count('\n', 0, enum_match.start(1) + match.start()) + 1,
                             'Sora native actor', animation_calls=re.findall(r'\b(SetBtlSora\w*Animation)\(([^;]+)\);', body),
                             resource_calls=sorted(set(RESOURCE_CALL.findall(body))),
                             transitions=sorted(set(re.findall(r'\bBTL_SORA_STATE_\w+', body))))
            entry['dependency_symbols'] = sorted(set(ASSET_TOKEN.findall(body)))
        fade_rel, fade, fade_line = self.source(ROOT / 'include/fade.h')
        for index, match in enumerate(re.finditer(r'^[ \t]*(FADE_MODE_\w+)\s*,?', fade, re.M)):
            self.add('native-color-mode', match[1], fade_rel, fade_line(match.start()), 'Native global palette effects', native_id=index)

    def links(self):
        symbols = defaultdict(list)
        palette_names = defaultdict(list)
        for entry in self.entries:
            symbols[entry['symbol']].append(entry)
            if entry['kind'] in ('palette', 'raw-palette-resource'):
                palette_names[entry['name']].append(entry)
        for entry in self.entries:
            if entry.get('palette_name'):
                entry['dependency_symbols'] += [row['symbol'] for row in palette_names[entry['palette_name']]]
            entry['dependencies'] = sorted(set(other['id'] for symbol in entry['dependency_symbols'] for other in symbols.get(symbol, []) if other is not entry))
            entry['unresolved_dependency_symbols'] = sorted(set(symbol for symbol in entry['dependency_symbols'] if symbol not in symbols))
        # Every source use is retained; no arbitrary first-N truncation.
        wanted = set(symbols)
        for source, text in self.sources.items():
            if not source.endswith('.c'):
                continue
            lines = text.splitlines()
            for lineno, line in enumerate(lines, 1):
                for symbol in set(re.findall(r'\b[A-Za-z_]\w*\b', line)) & wanted:
                    for entry in symbols[symbol]:
                        if source != entry['source'] or lineno != entry['line']:
                            entry['usages'].append(dict(source=source, line=lineno))
        by_id = {entry['id']: entry for entry in self.entries}
        for entry in self.entries:
            for target in entry['dependencies']:
                by_id[target].setdefault('used_by_entries', []).append(entry['id'])

    def all_actor_states_and_attacks(self):
        actor_header = ROOT / 'include/btl/battle_actor_types.h'
        if actor_header.relative_to(ROOT).as_posix() not in self.sources:
            self.source(actor_header)
        flag_values = {name: int(value, 0) for name, value in re.findall(
            r'\b(ATTACK_FLAG_\w+)\s*(?:=|\s)\s*(0x[0-9a-fA-F]+|\d+)', read(actor_header))}
        for entry in self.entries:
            if entry['kind'] == 'native-attack-definition':
                flags = re.findall(r'\bATTACK_FLAG_\w+\b', entry['parameters']['flags_expression'])
                entry['behavior_flags'] = [flag for flag in flags if not any(part in flag for part in ('_ELEMENT_', '_KIND_', '_INFLICT_'))]
                entry['flags_numeric'] = 0
                for flag in flags:
                    if flag in flag_values:
                        entry['flags_numeric'] |= flag_values[flag]
                    else:
                        entry.setdefault('unresolved_flag_symbols', []).append(flag)
        # Actor enums sometimes live in headers rather than their task C file.
        for path in sorted((ROOT / 'include').rglob('*.h')):
            text = read(path)
            if re.search(r'enum\s+\w*(?:State|Phase)\w*\s*\{|\b\w+_STATE_\w+', text):
                rel = path.relative_to(ROOT).as_posix()
                if rel not in self.sources:
                    self.source(path)
        members = defaultdict(list)
        for source, text in list(self.sources.items()):
            if not source.endswith(('.c', '.h')):
                continue
            clean = comment_mask(text)
            offsets = [-1] + [m.start() for m in re.finditer('\n', text)]
            line = lambda pos: bisect.bisect_left(offsets, pos)
            group = str(Path(source).parent) + '/' + Path(source).stem
            for enum in re.finditer(r'\benum\s*(\w+)?\s*\{([^{}]*)\}', clean, re.S):
                name, body = enum[1] or 'anonymous', enum[2]
                if not re.search(r'State|Phase', name) and not re.search(r'\b\w+_STATE_\w+', body):
                    continue
                declared = self.add('actor-state-enum', name, source, line(enum.start()), group,
                                    conditional_source_variants='#' in body, member_symbols=[])
                self.coverage['actor_state_enum_declarations'] += 1
                current = -1
                for member in re.finditer(r'\b([A-Z][A-Z0-9_]+)\s*(?:=\s*([^,\n]+))?\s*,?', body):
                    symbol, expression = member[1], member[2]
                    if symbol in ('PLATFORM_PC', 'VERSION_US', 'VERSION_EU', 'VERSION_JP'):
                        continue
                    numeric = number(expression) if expression else current + 1 if current is not None else None
                    current = numeric
                    native_id = numeric if '#' not in body or expression else None
                    state = self.add('actor-state', symbol, source, line(enum.start(2) + member.start()), group,
                                     enum=name, native_id=native_id, explicit_value=expression,
                                     source_ordinal=len(declared['member_symbols']), case_sites=[])
                    declared['member_symbols'].append(symbol)
                    members[symbol].append(state)
            for table in re.finditer(r'\b(?:static\s+)?(?:const\s+)?(?:s32|u32|u16|s16|u8|s8)\s+(\w*(?:AttackIds|HitFrames|HitBox|Hitbox)\w*)\s*\[[^;=]*\]\s*=\s*\{', clean):
                end = matching(clean, table.end() - 1)
                expression = text[table.end():end - 1]
                values = split_c(expression)
                record = self.add('native-attack-index-table', table[1], source, line(table.start()), group,
                                  values=values, integer_values=[number(value) for value in values])
                if 'AttackIds' in table[1]:
                    record['dependency_symbols'] = ['sBattleAttackDefs[%d]' % number(value) for value in values if number(value) is not None]
            for call in re.finditer(r'\b(ApplyAttackBox|ApplyAttackToBtlObj|TestAttackBox)\s*\(', clean):
                end = matching(clean, call.end() - 1, '(', ')')
                after = clean[end:].lstrip()
                args = split_c(text[call.end():end - 1])
                if after.startswith('{') or not args or re.match(r'^(s32|s16|u32|int)\s+\w+', args[0]):
                    continue
                record = self.add('authored-hitbox-callsite', call[1] + '@' + str(call.start()), source, line(call.start()), group,
                                  api=call[1], arguments=args, owner_functions=[],
                                  geometry_units='x/y/z are native8.8 world expressions; halfX/halfY/halfZ are pixel extents at callsite')
                if call[1] != 'TestAttackBox':
                    record['attack_id_expression'] = args[0]
                    attack_id = number(args[0])
                    record['literal_attack_id'] = attack_id
                    record['dependency_symbols'] = ['sBattleAttackDefs[%d]' % attack_id] if attack_id is not None else []
                if call[1] == 'ApplyAttackBox' and len(args) == 7:
                    record['authored_geometry'] = dict(zip(('x', 'y', 'z', 'half_x', 'half_y', 'half_z'), args[1:]))
                for fn in self.functions:
                    if fn['source'] == source and fn['line'] <= line(call.start()) <= fn['end_line']:
                        record['owner_functions'].append(fn['name'])
                self.coverage['authored_hitbox_callsites'] += 1
        state_symbols = set(members)
        for fn in self.functions:
            cases = [m for m in re.finditer(r'\bcase\s+([A-Z][A-Z0-9_]+)\s*:', fn['body']) if m[1] in state_symbols]
            for i, match in enumerate(cases):
                symbol = match[1]
                body = fn['body'][match.end():cases[i + 1].start() if i + 1 < len(cases) else len(fn['body'])]
                assignments = sorted(set(re.findall(r'\b(?:state|nextState|phase)\s*=\s*([A-Z][A-Z0-9_]+)', body)) & state_symbols)
                calls = sorted(set(re.findall(r'\b([A-Za-z_]\w*(?:Anim|Animation)\w*)\s*\(', body)))
                animation_calls = []
                for call in re.finditer(r'\b([A-Za-z_]\w*(?:Anim|Animation)\w*)\s*\(', body):
                    try:
                        end = matching(body, call.end() - 1, '(', ')')
                    except ValueError:
                        continue
                    animation_calls.append(dict(function=call[1], arguments=split_c(body[call.end():end - 1])))
                site = dict(function=fn['name'], source=fn['source'],
                            line=fn['body_line'] + fn['body'][:match.start()].count('\n'),
                            authored_assignment_transitions=assignments,
                            state_references=sorted(set(re.findall(r'\b[A-Z][A-Z0-9_]+\b', body)) & state_symbols),
                            animation_functions=calls, animation_calls=animation_calls,
                            resource_calls=sorted(set(RESOURCE_CALL.findall(body))),
                            relative_state_changes=re.findall(r'\b(?:state|phase)\s*(?:\+\+|--|\+=\s*\d+)', body),
                            bounds_note='Lexical state case region; nested conditions and variable expressions remain source-authored')
                local = [entry for entry in members[symbol] if entry['source'] == fn['source']]
                for entry in local or members[symbol]:
                    entry['case_sites'].append(site)
                    entry['dependency_symbols'] += list(ASSET_TOKEN.findall(body))
        case_sites = defaultdict(list)
        for entries in members.values():
            for entry in entries:
                for site in entry['case_sites']:
                    case_sites[(site['source'], site['function'])].append((site['line'], entry['symbol']))
        for entry in self.entries:
            if entry['kind'] != 'authored-hitbox-callsite':
                continue
            contexts = []
            for fn in entry['owner_functions']:
                preceding = [site for site in case_sites[(entry['source'], fn)] if site[0] <= entry['line']]
                if preceding:
                    contexts.append(max(preceding)[1])
            entry['lexical_state_contexts'] = sorted(set(contexts))
            entry['dependency_symbols'] += entry['lexical_state_contexts']

    def controller_routes(self):
        path = ROOT / 'port/src/port_action_routes.c'
        if not path.exists():
            return
        source, text, line = self.source(path)
        clean = comment_mask(text)
        routed = set(re.findall(r'\bMOVE_\w+', clean))
        for entry in self.entries:
            if entry['kind'] in ('native-move-id', 'sora-native-move'):
                entry['connected_controller_route_member'] = entry['symbol'] in routed
        self.coverage['connected_route_move_symbols'] = len(routed)
        for match in re.finditer(r'^(?:uint32_t|unsigned|const char\*)\s+(PortActionRoute\w+)\s*\([^;]*?\)\s*\{', clean, re.M):
            end = matching(clean, match.end() - 1)
            body = text[match.end():end - 1]
            entry = self.add('controller-route-source', match[1], source, line(match.start()), 'Zwebti Mix connected controller routes',
                             signature=text[match.start():match.end() - 1].strip(),
                             declared_move_symbols=sorted(set(re.findall(r'\bMOVE_\w+', body))),
                             implementation_expression=body.strip(),
                             coverage_test='port/tools/test_action_routes_catalog.c',
                             note='Production route logic is inventoried here; the separate C test executes all directions/charge/modifier combinations against catalog validity and complete meaningful coverage.')
            entry['dependency_symbols'] = entry['declared_move_symbols']

    def result(self):
        self.links()
        counts = dict(sorted(Counter(entry['kind'] for entry in self.entries).items()))
        return dict(schema_version=1, title='Kingdom Hearts Chain of Memories: Zwebti Mix — Native Source Catalog',
                    scope=['All64 config/assets manifests inventoried; visual resource entries, authored sprite pieces and animation timelines catalogued.',
                           'All src/**/*.c scanned for selected native animation/effect/task declaration types and all BgFxStart function definitions.',
                           'Sora native move dispatcher, attack patterns, animation bindings and states are a subset of this larger source inventory.',
                           'All330 authored BattleAttackDef IDs and all parsed attack-box callsites are indexed. State/Phase enums cover actors plus other native game flow; lexical transitions are source metadata, not a simulated complete control-flow proof.',
                           'Conditional region/platform declarations remain distinct source records. Definition counts are not unique gameplay behavior counts.',
                           'Mod-only task wrappers are counted separately from native task declarations. Original task declarations may have explicitly listed PC action callback overrides.',
                           'No ROM, saves, extracted graphics, palette pixel bytes, or external artwork are read. Source palette identity/count is provided; numerical cartridge RGB swatches are outside this metadata scope.',
                           'Palette counts are16-bit entries, including transparent/duplicate colors and multi-bank tables, not unique RGB colors. Two raw named palette resources and movie containers are listed separately from typed palettes and battle effects.',
                           'Static call/dependency metadata does not prove runtime safety. Independent effect/assist reuse needs owned resources and verified lifecycle.'],
                    counts=counts, coverage=dict(sorted(self.coverage.items())), inputs=self.inputs,
                    generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                    source_manifest_sha256=hashlib.sha256(json.dumps(self.inputs, sort_keys=True).encode()).hexdigest(),
                    parse_notes=self.parse_notes, unresolved=self.unresolved, entries=self.entries)


HTML = '''<!doctype html><html lang="en"><meta charset="utf-8"><title>Kingdom Hearts Chain of Memories: Zwebti Mix — Native Catalog</title>
<style>body{font:15px system-ui;background:#101722;color:#e5edf7;margin:0}header{padding:28px 4vw;background:#172334}h1{font-size:24px;margin:0 0 10px}main{padding:20px 4vw}p{line-height:1.5;max-width:1100px}button,input,select{font:inherit;padding:9px;border:1px solid #42526b;border-radius:5px;background:#1b293d;color:#f3f6fc}input{min-width:340px}table{width:100%;border-collapse:collapse;margin-top:16px}th,td{text-align:left;padding:9px;border-bottom:1px solid #2a3a50;font-size:13px}tr:hover{background:#23324a;cursor:pointer}pre{white-space:pre-wrap;overflow-wrap:anywhere;padding:16px;background:#0b111a;line-height:1.5}#detail{position:fixed;inset:8% 8%;background:#162236;overflow:auto;border:1px solid #557399;padding:20px;box-shadow:0 0 0 100vw #0009}#detail[hidden]{display:none}.cards{display:flex;gap:8px;flex-wrap:wrap}.card{padding:10px;background:#263752;border-radius:5px}a{color:#82ceff}small{color:#b4c2d4}#summary{margin-top:12px}</style>
<header><h1>Kingdom Hearts Chain of Memories: Zwebti Mix</h1><div>Native attacks, animations, sprites, palettes and effects — source inventory</div></header><main>
<p>This catalog covers the declared native resources in this checkout. It contains metadata and authored timings; the original artwork and numerical cartridge palettes remain in the local game resources. A source entry is an available implementation or asset, not a promise that an enemy actor can be called as a Sora attack.</p>
<div class="cards" id="counts"></div><details><summary>Scope, coverage and unresolved records</summary><div id="scope"></div><pre id="coverage"></pre></details>
<p><input id="search" placeholder="Search Sora, Cloud, fire, petal, palette, symbol…"> <select id="kind"><option value="">All resource kinds</option></select> <select id="group"><option value="">All native groups</option></select> <button id="previous">Previous</button> <button id="next">Next</button></p><div id="summary"></div>
<table><thead><tr><th>Kind</th><th>Name / native symbol</th><th>Family</th><th>Timing / structure</th><th>Source</th></tr></thead><tbody id="rows"></tbody></table><section id="detail" hidden><button id="close">Close details</button><h2 id="heading"></h2><pre id="json"></pre></section></main>
<script id="catalog" type="application/json">__CATALOG__</script><script>
const data=JSON.parse(document.getElementById('catalog').textContent),q=s=>document.getElementById(s),escape=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let page=0,filtered=data.entries;const pageSize=100;
q('counts').innerHTML=Object.entries(data.counts).map(([k,v])=>`<div class="card"><b>${v.toLocaleString()}</b> ${escape(k)}</div>`).join('');
q('scope').innerHTML=data.scope.map(s=>`<p>${escape(s)}</p>`).join('');q('coverage').textContent=JSON.stringify({coverage:data.coverage,unresolved:data.unresolved,parse_notes:data.parse_notes},null,2);
for(const id of ['kind','group'])for(const value of [...new Set(data.entries.map(e=>e[id]))].sort()){let o=document.createElement('option');o.value=o.textContent=value;q(id).append(o)}
function render(){let term=q('search').value.trim().toLowerCase();filtered=data.entries.filter(e=>(!q('kind').value||e.kind===q('kind').value)&&(!q('group').value||e.group===q('group').value)&&(!term||[e.symbol,e.name,e.group,e.native_family,e.source,...e.dependency_symbols].join(' ').toLowerCase().includes(term)));let max=Math.max(0,Math.ceil(filtered.length/pageSize)-1);page=Math.min(page,max);q('summary').textContent=`${filtered.length.toLocaleString()} matching entries · page ${page+1}/${max+1} · click an entry for its exact source, dependencies, usage and lifecycle`;q('rows').innerHTML=filtered.slice(page*pageSize,(page+1)*pageSize).map((e,i)=>`<tr data-i="${page*pageSize+i}"><td>${escape(e.kind)}</td><td>${escape(e.name||e.symbol)}${e.name&&e.name!==e.symbol?'<br><small>'+escape(e.symbol)+'</small>':''}</td><td>${escape(e.native_family)}</td><td>${e.duration_ticks!==undefined&&e.duration_ticks!==null?escape(e.duration_ticks)+' native ticks':e.frame_count!==undefined?escape(e.frame_count)+' frames / '+escape(e.animation_count)+' animations':e.rows?escape(e.rows.length)+' initializer rows':''}</td><td>${escape(e.source)}:${e.line}</td></tr>`).join('');q('previous').disabled=!page;q('next').disabled=page>=max}
q('rows').onclick=event=>{let tr=event.target.closest('tr');if(!tr)return;let e=filtered[Number(tr.dataset.i)];q('heading').textContent=e.name||e.symbol;q('json').textContent=JSON.stringify(e,null,2);q('detail').hidden=false};q('close').onclick=()=>q('detail').hidden=true;document.onkeydown=e=>{if(e.key==='Escape')q('detail').hidden=true};for(const id of ['search','kind','group'])q(id).oninput=()=>{page=0;render()};q('previous').onclick=()=>{page--;render()};q('next').onclick=()=>{page++;render()};render();
</script></html>'''


def build():
    builder = Builder()
    builder.manifests()
    builder.c_sources()
    builder.enrich_c()
    builder.sora()
    builder.all_actor_states_and_attacks()
    builder.controller_routes()
    return builder.result()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'build/native-catalog-v010')
    args = parser.parse_args()
    catalog = build()
    args.out.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(catalog, ensure_ascii=False, separators=(',', ':'))
    (args.out / 'native_catalog.json').write_text(payload + '\n', encoding='utf-8')
    (args.out / 'native_catalog.html').write_text(HTML.replace('__CATALOG__', payload.replace('<', '\\u003c')), encoding='utf-8')
    (args.out / 'coverage.json').write_text(json.dumps(dict(counts=catalog['counts'], coverage=catalog['coverage'], source_manifest_sha256=catalog['source_manifest_sha256'], unresolved=catalog['unresolved'], parse_notes=catalog['parse_notes']), indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(out=str(args.out.resolve()), counts=catalog['counts'], unresolved=len(catalog['unresolved'])), indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
