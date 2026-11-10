#!/usr/bin/env python3
"""Narrow read-only observations of an owned native child game's spell impacts.

Ordinary keys drive the game. This observer reads only its frame counter,
damage-feedback tuple, and live actors' HP/status/position fields. It never
writes child memory, changes rules, or persists a process-memory dump.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]
SPELLS=('Fire','Blizzard','Thunder','Cure','Gravity','Stop','Aero')
# Extracted from the actual production 32-bit headers with __builtin_offsetof.
OFF={'sora_actor':64,'obj_hp':44,'obj_maxhp':46,'obj_flags':52,'obj_z':12,
     'obj_status':232,'obj_status_timer':236,'battle_pool':128,'pool_active':8,
     'node_next':8,'node_owner':0,'obj_kind':0}

def symbols(elf):
    names={'sFeedback','gFrameCounter','sActionWork','gBtlWork'}
    result={}
    for line in subprocess.check_output(['nm','-a',str(elf)],text=True).splitlines():
        parts=line.split()
        if len(parts)==3 and parts[2]in names:result[parts[2]]=int(parts[0],16)
    if set(result)!=names:raise ValueError('missing exact game observer symbols')
    return result

def run(spell,args,sym):
    folder=args.out.parent/'cohesion-magic-native'/spell.lower();folder.mkdir(parents=True,exist_ok=True)
    for name,target in [('B8CE.gba',ROOT/'roms/B8CE.gba'),('assets',ROOT/'port/assets')]:
        p=folder/name
        if not p.exists():p.symlink_to(target,target_is_directory=target.is_dir())
    config=(ROOT/'mod/action-edition/kh_com.ini').read_text()
    config=re.sub(r'(?m)^window_scale=.*$','window_scale=1',config)
    (folder/'kh_com.ini').write_text(config)
    (folder/'rebel_mode.txt').write_text('unlocked\n')
    index=SPELLS.index(spell)
    # Repeated ordinary presses catch ready windows amid genuine enemy hit
    # reactions; a single sparse press during damage is correctly discarded.
    keys=[f'{120+i*10}-{121+i*10}:R'for i in range(index)]+[f'{f}-{f+1}:L'for f in range(180,551,12)]
    env={k:v for k,v in os.environ.items()if not k.startswith(('KHCOM_','SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',
        KHCOM_FAST='1',KHCOM_PPU_THREADS='1',KHCOM_ACTION_TRACE='1',
        KHCOM_PRACTICE='mixed',KHCOM_KEYS=','.join(keys),KHCOM_QUIT='620')
    transitions=[];previous=None;errors=[];sampled_frames=set();fd=None;last_sample=None;max_gap=0
    began=time.monotonic()
    with(folder/'host.log').open('w')as log:
        child=subprocess.Popen([str(args.host),str(args.guest),'--data',str(folder)],env=env,cwd=ROOT,stdout=log,stderr=log)
        try:
            fd=os.open(f'/proc/{child.pid}/mem',os.O_RDONLY)
            def read(addr,fmt):
                size=struct.calcsize(fmt);return struct.unpack(fmt,os.pread(fd,size,addr))
            def actor(ptr):
                if not 0x10000<=ptr<0xf0000000:return None
                return dict(pointer=hex(ptr),kind=read(ptr+OFF['obj_kind'],'<i')[0],
                    hp=read(ptr+OFF['obj_hp'],'<h')[0],max_hp=read(ptr+OFF['obj_maxhp'],'<h')[0],
                    z=read(ptr+OFF['obj_z'],'<i')[0],flags=hex(read(ptr+OFF['obj_flags'],'<Q')[0]),
                    status=read(ptr+OFF['obj_status'],'<i')[0],status_timer=read(ptr+OFF['obj_status_timer'],'<h')[0])
            while child.poll()is None:
                if time.monotonic()-began>120:child.kill();errors.append('native case timeout');break
                try:
                    frame=read(sym['gFrameCounter'],'<I')[0]
                    sora=read(sym['sActionWork'],'<I')[0];battle=read(sym['gBtlWork'],'<I')[0]
                    if not (5<=frame<=615 and sora and battle):time.sleep(.001);continue
                    player=actor(sora+OFF['sora_actor'])
                    enemies=[];node=read(battle+OFF['battle_pool']+OFF['pool_active'],'<I')[0]
                    seen=set()
                    while node and node not in seen and len(seen)<16:
                        seen.add(node);obj=actor(read(node+OFF['node_owner'],'<I')[0])
                        if obj:enemies.append(obj)
                        node=read(node+OFF['node_next'],'<I')[0]
                    hits,best,ttl,pulse,damage=read(sym['sFeedback'],'<HHHHI')
                    if read(sym['gFrameCounter'],'<I')[0]!=frame:continue
                    sampled_frames.add(frame)
                    if last_sample is not None:max_gap=max(max_gap,frame-last_sample)
                    snapshot=dict(player=player,enemies=enemies,feedback=dict(hits=hits,best=best,damage=damage))
                    # Positions are limited to actual status/damage transitions;
                    # this is not a per-frame trajectory or memory capture.
                    key=(player['hp']if player else None,tuple((o['pointer'],o['hp'],o['status'],int(o['flags'],16)&0x40000)for o in enemies),hits,best,damage)
                    if key!=previous:
                        transitions.append(dict(frame=frame,previous_sample_frame=last_sample,**snapshot));previous=key
                    last_sample=frame
                except (OSError,struct.error):
                    pass # Loader startup and actor list changes can race reads.
                time.sleep(.001)
        finally:
            if fd is not None:os.close(fd)
            code=child.wait(timeout=10)
    game=(folder/'kh_com.log').read_text(errors='replace')if(folder/'kh_com.log').exists()else''
    commands=[dict(frame=int(a),name=b)for a,b in re.findall(r'action: accepted frame=(\d+).*?command="([^"]+)"',game)]
    releases=[int(f)for f in re.findall(r'action: cast-release frame=(\d+)',game)]
    recoveries=[int(f)for f in re.findall(r'action: cast-recovery frame=(\d+)',game)]
    hp_losses=[];heals=[];stops=[];gravity=[];feedback_events=[]
    for before,after in zip(transitions,transitions[1:]):
        if after['feedback']['damage']>before['feedback']['damage']:
            release=max([f for f in releases if f<=after['frame']]or[-1])
            recovery=next((f for f in recoveries if release<=f<=after['frame']),None)if release>=0 else None
            feedback_events.append(dict(frame=after['frame'],damage_gain=after['feedback']['damage']-before['feedback']['damage'],
                total_damage=after['feedback']['damage'],hits=after['feedback']['hits'],cast_release=release,
                cast_recovery=recovery,last_unchanged_sample=after['previous_sample_frame'],
                after_recovery=recovery is not None and after['previous_sample_frame']is not None and recovery<=after['previous_sample_frame']<after['frame']))
        old={o['pointer']:o for o in before['enemies']}
        for obj in after['enemies']:
            prev=old.get(obj['pointer'])
            if prev and prev['hp']>obj['hp']:
                release=max([f for f in releases if f<=after['frame']]or[-1])
                recovery=next((f for f in recoveries if release<=f<=after['frame']),None)if release>=0 else None
                hp_losses.append(dict(frame=after['frame'],enemy=obj['pointer'],kind=obj['kind'],
                    old=prev['hp'],new=obj['hp'],damage=prev['hp']-obj['hp'],
                    cast_release=release if release>=0 else None,cast_recovery=recovery,
                    last_unchanged_sample=after['previous_sample_frame'],
                    after_recovery=recovery is not None and after['previous_sample_frame']is not None and recovery<=after['previous_sample_frame']<after['frame']))
            if obj['status']==2 and (not prev or prev['status']!=2):
                stops.append(dict(frame=after['frame'],enemy=obj['pointer'],status=obj['status'],timer=obj['status_timer']))
            if int(obj['flags'],16)&0x40000 and (not prev or not int(prev['flags'],16)&0x40000):
                gravity.append(dict(frame=after['frame'],enemy=obj['pointer'],hp=obj['hp'],z=obj['z']))
        a,b=before['player'],after['player']
        if a and b and b['hp']>a['hp']:
            heals.append(dict(frame=after['frame'],old=a['hp'],new=b['hp'],healed=b['hp']-a['hp'],max_hp=b['max_hp']))
    result=dict(spell=spell,exit=code,errors=errors,keys=keys,preset='mixed',cheats=False,
        observer='read-only own-child /proc/PID/mem; bounded feedback and actor fields; no writes',
        sampled_frames=len(sampled_frames),max_frame_sampling_gap=max_gap,commands=commands,releases=releases,recoveries=recoveries,
        actor_observer_available=any(t['player']for t in transitions),
        native_hp_changes=[dict(frame=int(f),old=int(a),new=int(b))for f,a,b in re.findall(r'action: hp frame=(\d+) old=(-?\d+) new=(-?\d+)',game)],
        feedback_damage_events=feedback_events,enemy_hp_losses=hp_losses,player_heals=heals,stop_status_applied=stops,gravity_pending=gravity,
        max_feedback_damage=max([t['feedback']['damage']for t in transitions]or[0]),transitions=transitions,
        log=str(folder/'kh_com.log'),elapsed_seconds=round(time.monotonic()-began,2))
    (folder/'observations.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k]for k in('spell','exit','sampled_frames','max_feedback_damage','commands')}),flush=True)
    return result

def summarize(tests):
    proof=[]
    for t in tests:
        spell=t['spell'];evidence=None
        if spell in('Fire','Thunder','Blizzard','Aero'):
            evidence=next((e for e in t['feedback_damage_events']if e['after_recovery']),None)
            meaning='actual enemy damage after caster recovered'
        elif spell=='Cure':
            evidence=[e for e in t['native_hp_changes']if e['new']>e['old']]
            meaning='actual healing on naturally wounded Sora'
        elif spell=='Gravity':
            evidence={'pending':t['gravity_pending'],'hp_losses':t['enemy_hp_losses']}
            meaning='gravity pending flag and actual enemy HP loss after caster recovery'
            if not t['gravity_pending']or not any(e['after_recovery']for e in t['enemy_hp_losses']):evidence=None
        elif spell=='Stop':
            evidence=t['stop_status_applied'];meaning='actual STOP bad status on ordinary enemy'
        proof.append(dict(spell=spell,proven=meaning,evidence=evidence,passed=bool(evidence)))
    return proof

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--guest',type=Path,default=ROOT/'build/windows64/out/guest-us.bin')
    p.add_argument('--host',type=Path,default=ROOT/'build/windows64/out/rekhcom-linux-test')
    p.add_argument('--elf',type=Path,default=ROOT/'port/ps5/build/guest-us/guest.elf')
    p.add_argument('--out',type=Path,default=ROOT/'build/cohesion-magic-impact.json')
    p.add_argument('--only',default='Fire,Thunder,Cure,Gravity,Stop,Blizzard,Aero')
    p.add_argument('--append',action='store_true')
    args=p.parse_args();args.out.parent.mkdir(parents=True,exist_ok=True)
    sha=hashlib.sha256(args.guest.read_bytes()).hexdigest();sym=symbols(args.elf);results=[]
    if args.append and args.out.exists():
        prior=json.loads(args.out.read_text())
        if prior['guest_sha256']!=sha:raise ValueError('cannot combine different guests')
        results=prior['tests']
    for spell in args.only.split(','):
        if spell not in SPELLS:p.error('unknown spell')
        results=[r for r in results if r['spell']!=spell]
        results.append(run(spell,args,sym))
        unchanged=sha==hashlib.sha256(args.guest.read_bytes()).hexdigest();proof=summarize(results)
        args.out.write_text(json.dumps(dict(guest_sha256=sha,guest_unchanged=unchanged,
            production_layout_offsets=OFF,symbols={k:hex(v)for k,v in sym.items()},tests=results,
            summary=proof,passed=unchanged and all(t['exit']==0 and not t['errors']for t in results)and all(e['passed']for e in proof),
            scope='Read-only native observations on ordinary mixed-practice enemies. No Windows/full-campaign claim.'),indent=2)+'\n')
if __name__=='__main__':main()
