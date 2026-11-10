#!/usr/bin/env python3
"""Run actual practice sessions without the debug battle or confirm fixture."""
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]

def cases():
    result=[]
    for preset in ("training","crowd","mixed","axel","jafar"):
        keys=[] if preset!="training" else [f"{f}-{f+1}:A" for f in range(180,900,18)]
        result.append(dict(name="practice-"+preset,preset=preset,frames=1000 if preset=="training" else 550,keys=keys))
    clear=[f"{f}-{f+1}:A" for f in range(180,3400,18)]
    clear += [f"{f}-{f+1}:R" for f in range(240,3400,180)]
    clear += [f"{f}-{f+20}:{('RIGHT','LEFT','UP','DOWN')[i%4]}"
              for i,f in enumerate(range(260,3400,130))]
    clear += [f"{f}-{f+1}:L" for f in range(400,3400,400)]
    result.append(dict(name="practice-clear",preset="crowd",frames=1800,keys=clear,
                       shots=[300,1000,1750]))
    result.append(dict(name="practice-retry",preset="training",frames=1000,
        keys=["400-401:SELECT","600-601:A"],expect_rounds=2,expect_preset=0,
        shots=[300,550,850]))
    result.append(dict(name="practice-held-magic-retry",preset="training",frames=800,
        keys=["400-401:SELECT","580-630:L","600-601:A"],expect_rounds=2,expect_preset=0,
        shots=[300,550,750]))
    result.append(dict(name="practice-spell-exit",preset="training",frames=950,
        keys=["300-301:R+UP","550-551:SELECT","730-731:A"],expect_rounds=2,expect_preset=0,
        shots=[520,600,850],expect_command="Ragnarok"))
    result.append(dict(name="practice-switch",preset="training",frames=1000,
        keys=["400-401:SELECT","550-551:DOWN","600-601:A"],expect_rounds=2,expect_preset=1,
        shots=[300,575,850]))
    result.append(dict(name="practice-title",preset="training",frames=1000,
        keys=["400-401:SELECT","600-601:B"],expect_title=True,shots=[300,550,850]))
    result.append(dict(name="story-title",frames=800,keys=[]))
    for action in (False,True):
        result.append(dict(name="story-battle-"+str(int(action)),entry=0,action=action,
                           frames=650,keys=["60-63:A"]+[f"{f}-{f+1}:A" for f in range(180,900,18)]))
    return result

def run(case,output):
    folder=output/case["name"];folder.mkdir(parents=True,exist_ok=True)
    for old in folder.glob("host_shot*.bmp"):old.unlink()
    for name,target in (("B8CE.gba",ROOT/"roms/B8CE.gba"),("assets",ROOT/"port/assets")):
        path=folder/name
        if not path.exists():path.symlink_to(target,target_is_directory=target.is_dir())
    config=(ROOT/"mod/action-edition/kh_com.ini").read_text()
    if not case.get("action",True):config=re.sub(r"(?m)^action_combat=.*$","action_combat=0",config)
    (folder/"kh_com.ini").write_text(config)
    env={k:v for k,v in os.environ.items() if not k.startswith(("KHCOM_","SDL_"))}
    shots=case.get("shots",[300,550,case["frames"]-20])
    env.update(SDL_VIDEO_DRIVER="dummy",SDL_AUDIO_DRIVER="dummy",SDL_RENDER_DRIVER="software",
               KHCOM_FAST="1",KHCOM_PPU_THREADS="1",KHCOM_ACTION_TRACE="1",
               KHCOM_KEYS=",".join(case["keys"]),KHCOM_QUIT=str(case["frames"]+2),
               KHCOM_HOST_SHOT=",".join(map(str,shots)))
    if case.get("preset"):env["KHCOM_PRACTICE"]=case["preset"]
    if "entry" in case:env["KHCOM_BATTLE"]=str(case["entry"])+":0:0"
    if case.get("mode"):env["KHCOM_MODE"]=case["mode"]
    began=time.monotonic()
    with (folder/"host.log").open("w") as stream:
        result=subprocess.run([str(ROOT/"build/windows64/out/rekhcom-linux-test"),
            str(ROOT/"build/windows64/out/guest-us.bin"),"--data",str(folder)],
            env=env,cwd=ROOT,stdout=stream,stderr=stream,timeout=600)
    log=(folder/"kh_com.log").read_text(errors="replace")
    starts=[dict(frame=int(a),preset=int(b),entry=int(c),name=d) for a,b,c,d in
            re.findall(r"practice: start frame=(\d+) preset=(\d+) entry=(\d+) name=([^\n]+)",log)]
    menus=list(map(int,re.findall(r"practice: menu frame=(\d+)",log)))
    names=re.findall(r'action: accepted frame=\d+ id=\d+ command="([^"]+)"',log)
    chains=list(map(int,re.findall(r"action: chain frame=\d+ hits=(\d+)",log)))
    passed=result.returncode==0 and "CRASH" not in log and all((folder/f"host_shot_{n}.bmp").exists() for n in shots)
    if case.get("preset"):
        passed=passed and bool(starts) and starts[0]["frame"]<100 and "mode: mode_battle" in log
    elif "entry" in case:
        passed=passed and not starts and "mode: mode_battle" in log
        if case.get("action"):passed=passed and "Attack" in names
        else:passed=passed and not names
    else:passed=passed and not starts and "mode: mode_title" in log
    if case["name"]=="practice-training":passed=passed and max(chains or [0])>=3
    hp=[tuple(map(int,row)) for row in re.findall(r"action: hp frame=(\d+) old=(-?\d+) new=(-?\d+)",log)]
    hp_before_return=([new for frame,old,new in hp if len(menus)>1 and frame<menus[1]] or [560])[-1]
    if case["name"]=="practice-clear":passed=passed and len(menus)>=2 and hp_before_return>0
    if case.get("preset"):
        passed=passed and "practice: rules first_strike=1" not in log
    if case.get("expect_rounds"):passed=passed and len(starts)>=case["expect_rounds"]
    if case.get("expect_preset") is not None:passed=passed and starts[-1]["preset"]==case["expect_preset"]
    if case.get("expect_command"):passed=passed and case["expect_command"] in names and len(menus)>=2
    if case.get("expect_title"):passed=passed and "practice: title" in log and "mode: mode_title" in log
    report=dict(name=case["name"],passed=bool(passed),exit=result.returncode,
        elapsed_seconds=round(time.monotonic()-began,3),starts=starts,menus=menus,
        hp_before_first_return=hp_before_return,commands=sorted(set(names)),best_chain=max(chains or [0]),cheats=False,
        debug_confirm_fixture="entry" in case,screenshots=[str(folder/f"host_shot_{n}.bmp") for n in shots])
    (folder/"report.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report),flush=True)
    return report

def main():
    p=argparse.ArgumentParser();p.add_argument("--only");p.add_argument("--jobs",type=int,default=3)
    p.add_argument("--out",type=Path,default=ROOT/"build/zwebti-practice-final")
    args=p.parse_args();selected=cases()
    if args.only:selected=[x for x in selected if x["name"] in args.only.split(",")]
    output=args.out.resolve();output.mkdir(parents=True,exist_ok=True)
    guest=ROOT/"build/windows64/out/guest-us.bin";sha=hashlib.sha256(guest.read_bytes()).hexdigest()
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:reports=list(pool.map(lambda c:run(c,output),selected))
    summary=dict(guest_sha256=sha,guest_unchanged=hashlib.sha256(guest.read_bytes()).hexdigest()==sha,
        tests=reports,passed=all(r["passed"] for r in reports),scope="Actual native guest, isolated fixture data, no invincibility or damage cheats")
    (output/"report.json").write_text(json.dumps(summary,indent=2)+"\n")
    return 0 if summary["passed"] and summary["guest_unchanged"] else 1
if __name__=="__main__":raise SystemExit(main())
