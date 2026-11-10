#!/usr/bin/env python3
"""Historical pre-0.5 native fixtures for the former special-chord controls.

These R+direction and hold/release-magic expectations intentionally describe
the old design. Use test_cohesion_native.py for the current cast/cycle/earned
combo controls; do not treat these accepted-command checks as combat timing.

Fixtures use the cartridge's debug battle menu and ordinary button schedules.
Every process has its own data folder. No invincibility or damage cheats are
enabled. Command traces prove actor acceptance; boss runs also record HP loss.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]


def scenarios():
    result = []
    def add(name, entry, keys, frames=600, expected=(), action=True, riku=False):
        result.append(dict(name=name, entry=entry, keys=["60-63:A"] + keys,
                           frames=frames, expected=list(expected), action=action, riku=riku))
    attacks = [f"{frame}-{frame+1}:A" for frame in range(180, 900, 18)]
    add("ground-combo", 206, attacks, 1000, ("Attack", "Finisher"))
    air = []
    for start in (180, 340, 500, 660):
        air.append(f"{start}-{start+2}:B")
        air += [f"{start+n}-{start+n+1}:A" for n in (12, 28, 44, 60)]
    add("aerial-combo", 206, air, 850, ("Aerial Attack", "Aerial Finisher"))
    add("movement-cancels", 206, ["180-181:A", "196-197:DODGE+RIGHT",
        "260-262:B", "280-281:A", "296-297:A", "312-313:A",
        "400-401:A", "416-417:B", "500-540:RIGHT+UP", "560-561:DODGE+LEFT"], 800)
    add("crowd-peak-eight", 0, ["300-301:DODGE+LEFT", "600-601:DODGE+RIGHT"], 1200)
    specials = [("blitz", "R", "Blitz"), ("dash-right", "R+RIGHT", "Sliding Dash"),
                ("raid-left", "R+LEFT", "Strike Raid"),
                ("stun-impact", "R+DOWN", "Stun Impact"),
                ("ragnarok", "R+UP", "Ragnarok")]
    for name, chord, expected in specials:
        add(name, 206, [f"{frame}-{frame+1}:{chord}" for frame in (180, 300, 420)],
            650, (expected,))
    add("sonic-blade", 206,
        [item for start in (180, 340, 500) for item in
         (f"{start}-{start+1}:B", f"{start+20}-{start+21}:R")], 750, ("Sonic Blade",))
    for index, name in enumerate(("Fire", "Blizzard", "Thunder", "Cure", "Gravity", "Stop", "Aero")):
        keys = []
        if index:
            keys.append(f"220-{220+index*10}:L")
            keys += [f"{220+n*10}-{221+n*10}:RIGHT" for n in range(index)]
        keys += [f"{frame}-{frame+1}:L" for frame in (350, 470, 590)]
        add("magic-"+name.lower(), 206, keys, 700, (name,))
    for name, entry in (("normal-damage", 0), ("axel-damage", 186), ("jafar-gimmick", 204)):
        keys = [f"{frame}-{frame+1}:A" for frame in range(180, 2500, 18)]
        keys += [f"{frame}-{frame+1}:R" for frame in range(240, 2500, 180)]
        keys += [f"{frame}-{frame+20}:{('RIGHT','LEFT','UP','DOWN')[n%4]}"
                 for n, frame in enumerate(range(260, 2500, 130))]
        keys += [f"{frame}-{frame+1}:L" for frame in range(400, 2500, 400)]
        add(name, entry, keys, 2800)
    add("tutorial-fallback", 207, [f"{f}-{f+1}:A" for f in range(180, 800, 24)], 900)
    add("tutorial-fallback-2", 208, [f"{f}-{f+1}:A" for f in range(180, 800, 24)], 900)
    add("riku-fallback", 0, attacks, 1000, riku=True)
    add("original-fallback", 0, attacks, 1000, action=False)
    add("field-movement", None, [], 950)
    result[-1]["keys"] = ["500-520:RIGHT", "540-560:UP", "580-600:LEFT",
                           "620-623:B", "640-670:RIGHT"]
    add("shipping-title", "title", [], 1900)
    result[-1]["keys"] = []
    add("shipping-technique-preview", 0, ["220-240:L", "220-221:RIGHT", "230-231:RIGHT",
        "300-301:R+UP"] +
        [f"{frame}-{frame+1}:A" for frame in range(800, 1100, 18)], 1200,
        ("Ragnarok",))
    add("shipping-magic-preview", 0, ["220-240:L", "220-221:RIGHT", "230-231:RIGHT"] +
        [f"{frame}-{frame+1}:L" for frame in (350, 470, 590)] +
        [f"{frame}-{frame+1}:A" for frame in range(800, 1100, 18)], 1200, ("Thunder",))
    add("jafar-blizzard-reaction", 204, ["220-221:L+RIGHT", "300-340:RIGHT"] +
        [f"{frame}-{frame+1}:L" for frame in (350, 500, 650, 800, 950, 1100)],
        1400, ("Blizzard",))
    add("original-hurt-flash", 0, [], 1000, action=False)
    # Authored Traverse Town encounter 4 contains a Soldier and Shadows.
    # The bonus wave must reuse a kind actually introduced by that encounter.
    add("crowd-mixed-waves", 4,
        [f"{f}-{f+1}:A" for f in range(180, 1800, 18)] +
        [f"{f}-{f+1}:R+DOWN" for f in range(280, 1800, 180)] +
        [f"{f}-{f+14}:{('RIGHT', 'LEFT')[i % 2]}"
         for i, f in enumerate(range(220, 1800, 110))], 1900)
    # These ordinary rolls place Sora through a live Shadow attack at frame
    # 412. Attack after recovery spends the earned window; no memory edits.
    evade = [f"{frame}-{frame+1}:DODGE+{('LEFT','RIGHT')[i%2]}"
             for i, frame in enumerate(range(220, 406, 37))]
    evade += [f"{frame}-{frame+1}:A" for frame in (434, 452, 470)]
    add("perfect-dodge-counter", 0, evade, 700, ("Finisher",))
    result[-1]["shots"] = list(range(400, 475, 3))
    add("polish-melee-preview", 4,
        [f"{f}-{f+1}:A" for f in range(180, 420, 18)], 450, ("Attack", "Finisher"))
    result[-1]["shots"] = list(range(180, 342, 3))
    # Deliberately hold the magic selector before choosing. Opening the
    # palette must not accidentally cast the formerly selected Fire spell.
    add("magic-picker-no-cast", 206,
        ["180-230:L", "200-201:RIGHT", "210-211:RIGHT", "300-301:L"],
        450, ("Thunder",))
    result[-1]["shots"] = [190, 205, 220, 245, 310, 400]
    add("magic-picker-dodge", 206, ["180-260:L", "220-221:DODGE"], 450)
    result[-1]["shots"] = [210, 224, 240, 265]
    add("melee-mp-recovery", 4, ["180-181:L"] +
        [f"{f}-{f+1}:A" for f in range(210, 1000, 18)], 1100,
        ("Fire", "Attack"))
    result[-1]["shots"] = list(range(262, 325, 3))
    add("target-assist", 4,
        [f"{f}-{f+1}:A" for f in range(180, 1350, 18)] +
        [f"{f}-{f+8}:{('RIGHT','LEFT')[i%2]}"
         for i,f in enumerate(range(310, 1250, 180))], 1450, ("Attack", "Finisher"))
    result[-1]["shots"] = list(range(340, 403, 3))
    add("finisher-magic-link", 206,
        ["180-181:A", "198-199:A", "216-217:A", "240-241:L"],
        480, ("Attack", "Finisher", "Fire"))
    add("enemy-threat-preview", 0, ["300-301:DODGE+LEFT"], 650)
    result[-1]["shots"] = list(range(485, 548, 3))
    return result


def run_case(case, host, guest, rom, output, window_scale, preset):
    folder = output / case["name"]
    folder.mkdir(parents=True, exist_ok=True)
    # Reusing an output folder must never turn an old capture into evidence
    # that the current executable rendered a frame successfully.
    for previous in folder.glob("host_shot_*.bmp"):
        previous.unlink()
    # A symlink keeps the cartridge out of generated distributable artifacts.
    link = folder / "B8CE.gba"
    if not link.exists(): link.symlink_to(rom)
    assets = folder / "assets"
    if not assets.exists(): assets.symlink_to(ROOT / "port/assets", target_is_directory=True)
    (folder / "kh_com.ini").write_text(
        "config_version=9\nview_mode=4\nwindow_scale=" + str(window_scale) + "\nhd_text=1\n"
        "hd_music=0\naction_combat=" + str(int(case["action"])) +
        "\ncheat_infinite_hp=0\ncheat_one_hit=0\n", encoding="utf-8")
    if preset:
        (folder / "kh_com.ini").write_bytes(preset.read_bytes())
        # A shipping preset should still exercise classic-mode fallback when
        # the scenario explicitly requests it.
        if not case["action"]:
            config = (folder / "kh_com.ini").read_text(encoding="utf-8")
            config = re.sub(r"(?m)^action_combat=.*$", "action_combat=0", config)
            (folder / "kh_com.ini").write_text(config, encoding="utf-8")
    shots = [300, min(550, case["frames"]-20), case["frames"]-20]
    if case["name"] == "shipping-technique-preview":
        shots = [300, 550, 640, 740, 920] + list(range(316, 396, 4))
    if case["name"] == "shipping-magic-preview":
        shots = [300, 550, 640, 740, 920] + list(range(366, 446, 4))
    if case["name"] == "original-hurt-flash":
        shots = list(range(300, 920, 4))
    shots = case.get("shots", shots)
    env = {k:v for k,v in os.environ.items() if not k.startswith(("KHCOM_", "SDL_"))}
    env.update(SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", SDL_RENDER_DRIVER="software",
               KHCOM_FAST="1", KHCOM_PPU_THREADS="1", KHCOM_ACTION_TRACE="1",
               KHCOM_BATTLE=f'{case["entry"]}:0:{int(case["riku"])}',
               KHCOM_KEYS=",".join(case["keys"]), KHCOM_HOST_SHOT=",".join(map(str, shots)),
               KHCOM_QUIT=str(case["frames"]+2))
    if case["entry"] is None:
        env.pop("KHCOM_BATTLE")
        env["KHCOM_MODE"] = "60:room:11000,80:fieldfreeze"
    elif case["entry"] == "title":
        env.pop("KHCOM_BATTLE")
        env.pop("KHCOM_KEYS")
    began = time.monotonic()
    with (folder / "host.log").open("w") as log:
        try:
            completed = subprocess.run([str(host), str(guest), "--data", str(folder)],
                                       env=env, cwd=ROOT, stdout=log, stderr=log, timeout=360)
            code = completed.returncode
        except subprocess.TimeoutExpired:
            code = "timeout"
    elapsed = time.monotonic() - began
    log = (folder / "kh_com.log").read_text(errors="replace") if (folder / "kh_com.log").exists() else ""
    commands = re.findall(r'action: accepted frame=(\d+) id=(\d+) command="([^"]+)" mp=(\d+) combo=(\d+) air=(\d+)', log)
    names = sorted({row[2] for row in commands})
    hp = [tuple(map(int, row)) for row in re.findall(r'action: hp frame=(\d+) old=(-?\d+) new=(-?\d+)', log)]
    tutorial_exit = any(int(frame)>62 for frame in re.findall(r'mode: mode_chkbtl \(frame (\d+)\)', log))
    exits = [int(frame) for frame in re.findall(r'mode: mode_chkbtl \(frame (\d+)\)', log)
             if int(frame) > 62]
    exit_hp = [next((new for frame, _, new in reversed(hp) if frame < exit_frame), None)
               for exit_frame in exits]
    movement = re.findall(r'action: (?:jump|dodge|cancel)[^\n]*', log)
    field_movement = re.findall(r'action-field:[^\n]*', log)
    crowd_peak = max([int(value) for value in re.findall(r'crowd: peak live=(\d+)', log)] or [0])
    shadow_peak = max([int(value) for value in re.findall(r'action: shadow attack frame=\d+ concurrent=(\d+)', log)] or [0])
    spawn_pattern = (r'crowd: spawn id=(\d+) live=(\d+) tiles=(\d+) pending=(\d+) '
                     r'total=(\d+) draw=(\d+) bonus=(\d+) mixed=(\d+) '
                     r'x=(-?\d+) y=(-?\d+) player=(-?\d+),(-?\d+)')
    spawns = [dict(zip(("id", "live", "tiles", "pending", "total", "draw", "bonus", "mixed",
                       "x", "y", "player_x", "player_y"), map(int, row)))
              for row in re.findall(spawn_pattern, log)]
    perfect_dodges = re.findall(r'action: perfect dodge[^\n]*', log)
    counters = re.findall(r'action: counter[^\n]*', log)
    targets = [dict(zip(("frame", "found", "x", "y", "dx", "dy"), map(int, row)))
               for row in re.findall(r'action: target frame=(\d+) found=(\d+) x=(-?\d+) y=(-?\d+) dx=(-?\d+) dy=(-?\d+)', log)]
    approaches = [dict(zip(("frame", "dx", "dy"), map(int, row)))
                  for row in re.findall(r'action: approach frame=(\d+) dx=(-?\d+) dy=(-?\d+)', log)]
    links = re.findall(r'action: link[^\n]*', log)
    threats = re.findall(r'action: threat start[^\n]*', log)
    mp_rewards = re.findall(r'action: melee mp[^\n]*', log)
    reactions = re.findall(r'action: boss reaction activated[^\n]*', log)
    chains = [dict(zip(("frame", "hits", "damage"), map(int, row)))
              for row in re.findall(r'action: chain frame=(\d+) hits=(\d+) damage=(\d+)', log)]
    fallback = "fallback" in case["name"]
    entered = ("mode: Mode_MapFld" in log if case["entry"] is None else
               "mode: mode_title" in log if case["entry"] == "title" else "mode: mode_battle" in log)
    passed = (code == 0 and entered and "CRASH" not in log and
              all(name in names for name in case["expected"]) and (not fallback or not commands))
    if "tutorial" in case["name"]:
        passed = passed and tutorial_exit
    if case["name"] in ("normal-damage", "axel-damage", "jafar-gimmick"):
        passed = passed and any(old > new for _, old, new in hp)
    if case["name"] == "movement-cancels":
        passed = passed and all(any(token in line for line in movement)
                                for token in ("action: jump", "action: dodge", "action: cancel"))
    if case["name"] == "crowd-peak-eight":
        passed = passed and crowd_peak == 8 and 2 <= shadow_peak <= 3
    if case["name"] == "crowd-mixed-waves":
        passed = (passed and any(row["bonus"] and row["mixed"] for row in spawns)
                  and all(row["live"] <= 8 and row["tiles"] <= 352 for row in spawns)
                  and all(abs(row["x"]-row["player_x"]) >= 72 or
                          abs(row["y"]-row["player_y"]) >= 48
                          for row in spawns if row["bonus"]))
    if case["name"] == "perfect-dodge-counter":
        passed = passed and bool(perfect_dodges) and bool(counters)
    if case["name"] == "magic-picker-no-cast":
        passed = passed and all(int(row[0]) >= 300 and row[2] == "Thunder" for row in commands)
    if case["name"] == "magic-picker-dodge":
        passed = passed and not commands and any("action: dodge" in line for line in movement)
    if case["name"] == "melee-mp-recovery":
        passed = passed and bool(mp_rewards) and bool(chains)
    if case["name"] == "target-assist":
        passed = (passed and any(row["found"] for row in targets) and bool(approaches) and bool(chains)
                  and all(abs(row["dx"]) <= 64 and abs(row["dy"]) <= 32
                          for row in targets if row["found"])
                  and all(abs(row["dx"]) <= 3*256 and abs(row["dy"]) <= 384
                          for row in approaches))
    if case["name"] == "finisher-magic-link":
        passed = passed and bool(links)
    if case["name"] == "enemy-threat-preview":
        passed = passed and bool(threats)
    if case["name"] == "jafar-blizzard-reaction":
        passed = passed and bool(reactions)
    if case["name"] == "normal-damage":
        passed = passed and any(value is not None and value > 0 for value in exit_hp)
    if case["name"] == "field-movement":
        passed = passed and any(int(frame)>=500 and int(speed)>0 for frame,speed in
            re.findall(r'action-field: frame=(\d+) state=\d+ speed=(\d+)', log))
    captures = [folder / f"host_shot_{frame}.bmp" for frame in shots]
    missing_captures = [path.name for path in captures if not path.is_file()]
    passed = passed and not missing_captures
    result = dict(name=case["name"], passed=passed, exit=code, frames=case["frames"],
                  elapsed_seconds=round(elapsed, 3), missing_captures=missing_captures,
                  cheats=False, commands=names, accepted=len(commands), expected=case["expected"],
                  hp_changes=hp, hp_loss=sum(old-new for _,old,new in hp if old>new),
                  tutorial_exit_observed=tutorial_exit if "tutorial" in case["name"] else None,
                  battle_exit_frames=exits, last_observed_hp_before_exit=exit_hp,
                  movement=movement, field_movement=field_movement, crowd_peak=crowd_peak,
                  spawns=spawns, perfect_dodges=perfect_dodges, counters=counters,
                  command_trace=[dict(zip(("frame", "id", "name", "mp", "combo", "air"),
                                         (int(row[0]), int(row[1]), row[2], int(row[3]), int(row[4]), int(row[5]))))
                                 for row in commands],
                  targets=targets, approaches=approaches, links=links,
                  threats=threats, melee_mp_rewards=mp_rewards,
                  chains=chains, best_hit_chain=max([row["hits"] for row in chains] or [0]),
                  shadow_peak_concurrency=shadow_peak,
                  boss_reactions=reactions,
                  screenshots=[str(p) for p in sorted(folder.glob("host_shot_*.bmp"))])
    (folder / "report.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result), flush=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", type=Path, default=ROOT / "build/windows64/out/rekhcom-linux-test")
    parser.add_argument("--guest", type=Path, default=ROOT / "build/windows64/out/guest-us.bin")
    parser.add_argument("--rom", type=Path, default=ROOT / "roms/B8CE.gba")
    parser.add_argument("--out", type=Path, default=ROOT / "build/action-native-regression")
    parser.add_argument("--only", help="comma-separated scenario names")
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--window-scale", type=int, choices=(1, 2, 3, 4), default=2,
                        help="4 gives 1280x720 visual captures; 2 is faster for logic coverage")
    parser.add_argument("--preset", type=Path, help="copy this shipping INI exactly, instead of the test configuration")
    args = parser.parse_args()
    cases = scenarios()
    if args.only:
        requested = list(dict.fromkeys(args.only.split(",")))
        available = {case["name"]: case for case in cases}
        if not requested or any(name not in available for name in requested):
            parser.error("unknown or empty --only scenario")
        cases = [available[name] for name in requested]
    output = args.out.resolve(); output.mkdir(parents=True, exist_ok=True)
    guest = args.guest.resolve(); host = args.host.resolve(); rom = args.rom.resolve()
    sha256 = hashlib.sha256(guest.read_bytes()).hexdigest()
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        reports = list(pool.map(lambda case: run_case(case, host, guest, rom, output, args.window_scale,
                              args.preset.resolve() if args.preset else None), cases))
    result = dict(guest_sha256=sha256, host_sha256=hashlib.sha256(host.read_bytes()).hexdigest(),
                  preset_sha256=hashlib.sha256(args.preset.read_bytes()).hexdigest() if args.preset else None,
                  guest_unchanged=hashlib.sha256(guest.read_bytes()).hexdigest()==sha256,
                  tests=reports, passed=all(row["passed"] for row in reports),
                  scope="Scripted native guest debug battles; no invincibility/damage cheats; full campaign unverified")
    (output / "report.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
    return 0 if result["passed"] and result["guest_unchanged"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
