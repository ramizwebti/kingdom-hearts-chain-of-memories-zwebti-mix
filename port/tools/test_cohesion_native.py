#!/usr/bin/env python3
"""Measure native controller combat, charged routes, contact and recovery.

Uses an owned ROM, isolated data directories, ordinary timed inputs and the
actual x32 game image. No damage/invulnerability cheats or memory edits.
Historical R+direction technique fixtures remain in test_action_native.py;
this suite uses controller-semantic inputs without a finisher selection menu.
"""
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[2]
SPELLS=("Fire","Blizzard","Thunder","Cure","Gravity","Stop","Aero")
NORMALS=(1,2,3,4,5,6)
MELEE=NORMALS+(9,11,20)
NATIVE=20
FINISHER_NAMES=("Blitz","Sonic Blade / Air Rush","Air Rush","Sliding Dash","Strike Raid",
    "Stun Impact","Ragnarok","Ars Arcanum","Zantetsuken","Fire Raid","Blizzard Raid",
    "Thunder Raid","Judgment","Gravity Raid","Stop Raid","Reflect Raid","Firaga Break",
    "Shock Impact","Trinity Limit")

def is_finisher(command):
    return command['id']in(9,11) or (command['id']==NATIVE and command['name']in FINISHER_NAMES)

def taps(start,end,spacing,key="A"):
    return [f"{f}-{f+1}:{key}" for f in range(start,end,spacing)]

def combo_keys(direction="",hold=1,modifier=False,magic=0):
    keys=taps(110,110+magic*8,8,"R")+["180-181:A","220-221:A","260-261:A"]
    button="A"+("+"+direction if direction else "")+("+MODIFIER"if modifier else "")
    return keys+[f"310-{310+hold}:{button}"]

def scenarios():
    result=[
        dict(name="dense-ground",preset="training",frames=1200,keys=taps(180,1080,6),shots=[180,198,230,270,350,550,900,1150]),
        dict(name="spam-full-animations",preset="training",frames=1300,keys=taps(180,1180,4),shots=[180,200,240,280,400,650,1000,1250]),
        dict(name="one-press-one-swing",preset="training",frames=420,keys=["180-181:A"],shots=[180,200,250,400]),
        dict(name="held-one-swing",preset="training",frames=420,keys=["180-300:A"],shots=[180,200,250,400]),
        dict(name="air-land-finisher",preset="training",frames=700,keys=["180-181:B","192-193:A","220-221:A","275-276:A","320-321:A"],shots=[200,235,280,340,650]),
        dict(name="ground-jump-finisher",preset="training",frames=900,keys=["180-181:A","210-211:A","250-251:B","265-266:A","310-323:A+RIGHT"],shots=[195,230,275,330,800]),
        dict(name="elemental-finisher-waits-for-landing",preset="training",frames=900,keys=["180-181:A","210-211:A","250-251:A","300-380:B","315-316:A+MODIFIER"],shots=[270,300,330,360,500,850]),
        dict(name="roll-startup-cancel",preset="training",frames=420,keys=["180-181:A","183-184:A","185-186:DODGE+LEFT","230-231:A"],shots=[184,190,200,245,350]),
        dict(name="guard-startup-cancel",preset="training",frames=420,keys=["180-181:A","183-184:A","185-205:GUARD","230-231:A"],shots=[184,190,200,245,350]),
        dict(name="guard-front",entry=0,frames=850,keys=["60-63:A","150-650:GUARD"],shots=[200,350,500,700,820]),
        dict(name="guard-back",entry=0,frames=850,keys=["60-63:A","100-102:LEFT","150-650:GUARD"],shots=[200,350,500,700,820]),
        dict(name="whiff-completes-in-place",preset="training",frames=650,keys=["100-140:LEFT","180-181:A","220-221:A","270-271:A"],shots=[170,220,280,350,600]),
        dict(name="hitstop-reference",preset="training",frames=280,keys=["180-181:A"],shots=[195,240]),
        dict(name="hitstop-input",preset="training",frames=350,keys=[],shots=[195,220,300]),
        dict(name="cycle-without-cast",preset="training",frames=450,keys=taps(180,251,10,"R"),shots=[180,205,230,270,400]),
        dict(name="cast-tap",preset="training",frames=450,keys=["180-181:CAST"],shots=[180,185,195,210,400]),
        dict(name="cast-to-melee",preset="training",frames=600,keys=["180-181:CAST"]+taps(190,550,6),shots=[180,195,205,220,250,450]),
        dict(name="cast-queued",preset="training",frames=650,keys=["180-181:CAST","198-199:CAST"],shots=[195,200,215,240,300,600]),
        dict(name="charged-spell-after-fire",preset="training",frames=900,keys=["180-181:CAST","198-211:CAST","335-336:A"],shots=[195,200,215,240,400,800],effect_reference="cast-queued"),
        dict(name="cast-queue-cancel",preset="training",frames=600,keys=["180-181:CAST","198-199:CAST","204-205:A"],shots=[195,200,210,240,300,450]),
        dict(name="cast-queue-cycle-cancel",preset="training",frames=600,keys=["180-181:CAST","198-199:CAST","204-205:R"],shots=[195,200,210,240,300,450]),
        dict(name="aerial-cast",preset="training",frames=600,keys=["180-181:B","194-195:CAST"]+taps(206,550,6),shots=[195,209,220,250,450]),
        dict(name="spell-weave-preserves-three-normals",preset="training",frames=1000,
             keys=["180-181:A","198-199:A","260-261:CAST","290-291:A","345-346:A+MODIFIER"],
             shots=[220,265,285,310,355,450,900]),
        dict(name="move-dodge",preset="training",frames=850,keys=["180-215:RIGHT","230-260:LEFT","280-281:A","300-301:DODGE+LEFT","360-361:B","430-431:DODGE+RIGHT"]+taps(500,750,8),shots=[200,245,310,380,650]),
        dict(name="crowd-pressure",preset="mixed",frames=1500,keys=taps(180,1450,8)+taps(260,1400,220,"CAST")+[f"{f}-{f+18}:{('RIGHT','LEFT','UP','DOWN')[i%4]}"for i,f in enumerate(range(220,1400,120))],shots=[300,550,1000,1400]),
        dict(name="practice-retry",preset="training",frames=850,keys=["350-351:SELECT","500-501:A"]+taps(600,780,8),shots=[300,430,700]),
        dict(name="practice-switch",preset="training",frames=850,keys=["350-351:SELECT","440-441:DOWN","500-501:A"],shots=[300,470,700]),
        dict(name="practice-title",preset="training",frames=850,keys=["350-351:SELECT","500-501:B"],shots=[300,430,750]),
        dict(name="practice-spell-exit",preset="training",frames=550,keys=["180-181:CAST","198-199:CAST","205-206:SELECT","350-351:B"],shots=[200,280,500]),
        dict(name="practice-queued-retry",preset="training",frames=650,keys=["180-181:CAST","198-199:CAST","205-206:SELECT","350-351:A"],shots=[200,280,550]),
    ]
    for name,direction,hold,segments,minimum in (("blitz","",1,3,150),("sonic","RIGHT",13,7,240)):
        reference="native-"+name+"-complete"
        base=combo_keys(direction,hold)
        result.append(dict(name=reference,preset="training",frames=1000,keys=base,shots=[180,270,330,450,700,950],expected_segments=segments,minimum_native_ticks=minimum))
        result.append(dict(name="native-"+name+"-offensive-lock",preset="training",frames=1300,keys=base+["326-327:CAST","340-341:B"]+taps(348,1150,6),shots=[270,340,450,700,1200],expected_segments=segments,minimum_native_ticks=minimum))
        result.append(dict(name="native-"+name+"-early-defense",preset="training",frames=1000,keys=base+["335-336:DODGE+LEFT","346-360:GUARD"],shots=[270,340,450,700,950],expected_segments=segments,minimum_native_ticks=minimum))
        for kind in ("guard","dodge"):
            result.append(dict(name="native-"+name+"-late-"+kind,preset="training",frames=1000,keys=list(base),shots=[270,350,450,700,950],reference=reference,defense=kind,contact_recovery=12 if name=="blitz"else 8,expected_segments=segments))
    branches=(("tap-neutral","",1,"Blitz",True),("tap-toward","RIGHT",1,"Sliding Dash",True),("tap-away","LEFT",1,"Strike Raid",False),("tap-depth-up","UP",1,"Ragnarok",True),("tap-depth-down","DOWN",1,"Stun Impact",True),("hold-neutral","",13,"Ars Arcanum",True),("hold-toward","RIGHT",13,"Sonic Blade / Air Rush",True),("hold-away","LEFT",13,"Zantetsuken",False))
    for name,direction,hold,expected,damage in branches:
        result.append(dict(name="branch-"+name,preset="training",frames=1200,keys=combo_keys(direction,hold),shots=[270,310,350,450,700,1100],expected_native=expected,combo_route=True,requires_damage=damage))
    raids=("Fire Raid","Blizzard Raid","Thunder Raid","Judgment","Gravity Raid","Stop Raid","Reflect Raid")
    for index,spell in enumerate(SPELLS):
        result.append(dict(name="spell-"+spell.lower(),preset="training",frames=550,keys=taps(120,120+index*10,10,"R")+["240-241:CAST"],expected_spell=spell,shots=[200,240,255,280,500]))
        result.append(dict(name="elemental-"+spell.lower()+"-raid",preset="training",frames=1200,keys=combo_keys(modifier=True,magic=index),shots=[270,310,350,450,700,1100],expected_native=raids[index],combo_route=True,requires_damage=index not in(5,)))
    for index,expected in ((0,"Firaga Break"),(2,"Shock Impact"),(3,"Trinity Limit")):
        result.append(dict(name="elemental-held-"+SPELLS[index].lower(),preset="training",frames=1400,keys=combo_keys(hold=13,modifier=True,magic=index),shots=[270,325,380,550,850,1300],expected_native=expected,combo_route=True,requires_damage=index!=2,requires_status=108 if index==2 else 0))
    tiers=(("Fira","Firaga"),("Blizzara","Blizzaga"),("Thundara","Thundaga"),("Cura","Curaga"),("Gravira","Graviga"),("Stopra","Stopga"),("Aerora","Aeroga"))
    for index,names in enumerate(tiers):
        for tier,(duration,expected) in enumerate(zip((13,31),names)):
            result.append(dict(name="charged-"+expected.lower(),preset="training",frames=1000,keys=taps(110,110+index*8,8,"R")+[f"240-{240+duration}:CAST"],shots=[240,255,280,350,600,950],expected_native=expected,requires_damage=index not in(3,5),spell_route=True,expected_mp=100-([12,14,22,35,22,30,24][index]+(5 if tier==0 else 10)),charge_minimum=12 if tier==0 else 30))
    advanced=((0,"","Homing Fira"),(0,"UP","Mega Flare"),(1,"","Homing Blizzara"),(1,"UP","Aqua Splash"),(2,"","Thundaga"),(3,"","Gifted Miracle"),(3,"UP","Holy"),(4,"","Synchro"),(4,"UP","Quake"),(4,"DOWN","Warp"),(4,"LEFT","Warpinator"),(5,"","Bind"),(5,"UP","Terror"),(5,"DOWN","Teleport"),(5,"LEFT","Confuse"),(6,"","Tornado"))
    for index,direction,expected in advanced:
        button="CAST+MODIFIER"+("+"+direction if direction else "")
        result.append(dict(name="advanced-"+expected.lower().replace(" ","-"),preset="training",frames=1400,keys=taps(110,110+index*8,8,"R")+[f"240-241:{button}"],shots=[240,260,310,450,800,1300],expected_native=expected,requires_damage=expected in("Homing Fira","Mega Flare","Homing Blizzara","Aqua Splash","Thundaga","Holy","Quake","Tornado"),spell_route=True,expected_mp=100-[12,14,22,35,22,30,24][index]-12))
    # Aqua Splash is an elevated, manually aimed stream. The 16px training
    # barrel lies below its native collision plane; use authored Air Soldiers
    # and ordinary stick aiming to require genuine positive damage instead.
    aqua=next(case for case in result if case['name']=="advanced-aqua-splash")
    aqua.pop('preset');aqua['entry']=63
    aqua['keys']=["60-63:A","180-181:R","210-211:CAST+MODIFIER+UP","275-305:RIGHT"]
    aqua['shots']=[180,220,260,320,450,900]
    for preset in ("crowd","mixed","axel","jafar"):
        result.append(dict(name="boot-"+preset,preset=preset,frames=750,keys=taps(180,680,8),shots=[200,350,700]))
    result.extend([
        dict(name="story-title",frames=800,keys=[],shots=[300,550,780]),
        dict(name="classic-fallback",entry=0,action=False,frames=650,keys=["60-63:A"]+taps(180,600,18),shots=[200,350,600]),
        dict(name="riku-fallback",entry=0,riku=True,frames=650,keys=["60-63:A"]+taps(180,600,18),shots=[200,350,600]),
        dict(name="visual-combo",visual=True,preset="training",frames=740,keys=taps(180,700,6),shots=list(range(180,661,4))),
        dict(name="visual-crowd",visual=True,preset="mixed",frames=450,keys=["100-101:CAST"]+taps(150,410,6),shots=list(range(90,391,4))),
        dict(name="visual-menu",visual=True,preset="training",frames=450,keys=["350-351:SELECT"],shots=[300,430]),
        dict(name="visual-cast-wait",visual=True,preset="training",frames=300,keys=["180-181:CAST","198-199:CAST"],shots=[180,195,200,215,240,280]),
    ])
    return result

def parse(log):
    command_re=(r'action: accepted frame=(\d+) id=(\d+) command="([^"]+)" '
                r'mp=(\d+) combo=(\d+) air=(\d+)')
    commands=[dict(zip(("frame","id","name","mp","combo","air"),
        (int(a),int(b),c,int(d),int(e),int(f)))) for a,b,c,d,e,f in re.findall(command_re,log)]
    positions=[tuple(map(int,row)) for row in re.findall(r'action: accepted [^\n]* at=(-?\d+),(-?\d+),(-?\d+)',log)]
    completions=[dict(zip(('frame','command','timer','anim','pose'),map(int,row))) for row in
        re.findall(r'action: animation-complete frame=(\d+) command=(\d+) timer=(\d+) anim=(\d+) pose=(\d+)',log)]
    cancels=[dict(frame=int(frame),kind=kind)for frame,kind in
        re.findall(r'action: cancel frame=(\d+) kind=(jump|dodge|guard)',log)]
    blocks=[dict(frame=int(frame),attack=int(attack))for frame,attack in
        re.findall(r'action: blocked frame=(\d+) attack=(\d+)',log)]
    guard=[dict(frame=int(frame),held=int(held))for frame,held in
        re.findall(r'action: guard frame=(\d+) held=([01])',log)]
    contacts=[dict(frame=int(a),hits=int(b),damage=int(c)) for a,b,c in
        re.findall(r'action: chain frame=(\d+) hits=(\d+) damage=(\d+)',log)]
    strikes=[dict(frame=int(a),command=int(b),timer=int(c)) for a,b,c in
        re.findall(r'action: strike frame=(\d+) command=(\d+) timer=(\d+)',log)]
    recovery_links=[dict(zip(('frame','command','next','ticks','anim','pose'),map(int,row))) for row in
        re.findall(r'action: recovery-link frame=(\d+) command=(\d+) next=(\d+) ticks=(\d+) anim=(\d+) pose=(\d+)',log)]
    def finished_or_linked(command, following):
        if any(command['frame']<=end['frame']<following['frame'] and end['command']==command['id']
               for end in completions): return True
        # Only performed normal strikes may use an authored recovery link.
        # Full native techniques still require their own completion/cancel.
        return command['id'] in NORMALS and any(
            command['frame']<link['frame']<=following['frame'] and link['command']==command['id'] and
            link['next']==following['id'] and any(command['frame']<=strike['frame']<link['frame'] and
                strike['command']==command['id'] for strike in strikes) for link in recovery_links)
    native_segments=[dict(frame=int(a),move=int(b),segment=int(c)) for a,b,c in
        re.findall(r'action: native-segment frame=(\d+) move=(\d+) segment=(\d+)',log)]
    native_completions=[dict(zip(('frame','command','move','elapsed','state'),map(int,row))) for row in
        re.findall(r'action: native-complete frame=(\d+) command=(\d+) move=(\d+) elapsed=(\d+) state=(\d+)',log)]
    native_contacts=[dict(zip(('frame','command','move','attack','result','pose'),map(int,row))) for row in
        re.findall(r'action: finisher-contact frame=(\d+) command=(\d+) move=(\d+) attack=(\d+) result=(\d+) pose=(\d+)',log)]
    status_contacts=[dict(frame=int(a),attack=int(b),status=c,target=int(d),duration=int(e),move=int(f)) for a,b,c,d,e,f in
        re.findall(r'action: status-contact frame=(\d+) attack=(\d+) status=(\w+) target=(\d+) duration=(\d+) move=(\d+)',log)]
    starts=[dict(frame=int(a),preset=int(b),entry=int(c),name=d) for a,b,c,d in
        re.findall(r'practice: start frame=(\d+) preset=(\d+) entry=(\d+) name=([^\n]+)',log)]
    hp=[tuple(map(int,row)) for row in re.findall(r'action: hp frame=(\d+) old=(-?\d+) new=(-?\d+)',log)]
    basic=[c for c in commands if c['id']in NORMALS]
    delays=[]
    contact_commands=[]
    for c in commands:
        next_command=next((d['frame'] for d in commands if d['frame']>c['frame']),10**9)
        hit=next((h for h in contacts if c['frame']<=h['frame']<next_command),None)
        contact_commands.append(dict(command=c['id'],name=c['name'],accepted=c['frame'],
            contact=hit['frame']if hit else None,delay=hit['frame']-c['frame']if hit else None))
    for c in basic:
        next_command=next((d['frame'] for d in commands if d['frame']>c['frame']),10**9)
        hit=next((h for h in contacts if c['frame']<=h['frame']<next_command),None)
        if hit:delays.append(hit['frame']-c['frame'])
    finisher_gaps=[]
    for i,c in enumerate(commands[:-1]):
        if is_finisher(c):
            following=next((d for d in commands[i+1:] if d['id']in NORMALS),None)
            if following:finisher_gaps.append(following['frame']-c['frame'])
    contact_gaps=[b['frame']-a['frame'] for a,b in zip(contacts,contacts[1:])]
    releases=[dict(frame=int(a),command=int(b),timer=int(c)) for a,b,c in
        re.findall(r'action: cast-release frame=(\d+) command=(\d+) timer=(\d+)',log)]
    recoveries=[dict(frame=int(a),command=int(b),timer=int(c)) for a,b,c in
        re.findall(r'action: cast-recovery frame=(\d+) command=(\d+) timer=(\d+)',log)]
    first_cast=next((c for c in commands if 13<=c['id']<=19),None)
    first_after_cast=next((c for c in basic if first_cast and c['frame']>first_cast['frame']),None)
    native_sequences=[]
    for i,c in enumerate(commands):
        if c['id']!=NATIVE and not is_finisher(c):continue
        next_frame=commands[i+1]['frame']if i+1<len(commands)else 10**9
        completion=next((end for end in native_completions if c['frame']<=end['frame']<next_frame),None)
        native_sequences.append(dict(accepted=c['frame'],name=c['name'],
            completion=completion,
            segments=[s['segment']for s in native_segments if c['frame']<=s['frame']<next_frame],
            contacts=[h for h in native_contacts if c['frame']<=h['frame']<next_frame],
            statuses=[h for h in status_contacts if c['frame']<=h['frame']<next_frame],
            damage=any(h['damage']>0 for h in contacts if c['frame']<=h['frame']<next_frame) or
                any(c['frame']<=h['frame']<next_frame for h in native_contacts)))
    return dict(commands=commands,contacts=contacts,strikes=strikes,starts=starts,recovery_links=recovery_links,
        native_segments=native_segments,native_completions=native_completions,
        native_contacts=native_contacts,native_sequences=native_sequences,
        status_contacts=status_contacts,
        native_positive_contacts=len(native_contacts),status_positive_contacts=len(status_contacts),
        attack_positions=positions,
        animation_completions=completions,
        blocks=blocks,guard=guard,
        deliberate_cancels=cancels,
        complete_before_next=all(any(c['frame']<=end['frame']<following['frame'] and end['command']==c['id']
            for end in completions) for c,following in zip(commands,commands[1:]) if c['id']in MELEE),
        complete_or_linked=all(finished_or_linked(c,following)
            for c,following in zip(commands,commands[1:]) if c['id']in MELEE),
        complete_or_defended=all(finished_or_linked(c,following) or any(c['frame']<cancel['frame']<following['frame']for cancel in cancels)
            for c,following in zip(commands,commands[1:]) if c['id']in MELEE),
        menus=list(map(int,re.findall(r'practice: menu frame=(\d+)',log))),
        hp_changes=hp,hp_loss=sum(old-new for _,old,new in hp if old>new),
        movement=re.findall(r'action: (?:jump|dodge|cancel)[^\n]*',log),
        perfect_dodges=re.findall(r'action: perfect dodge[^\n]*',log),
        counters=re.findall(r'action: counter[^\n]*',log),
        finishers=re.findall(r'action: (?:followup|ender|finisher ready)[^\n]*',log),
        targets=re.findall(r'action: target[^\n]*',log),
        contact_commands=contact_commands,
        ender_contacts=[c for c in contact_commands if
            (c['command']in(9,11)or(c['command']==NATIVE and c['name']in FINISHER_NAMES))and c['contact']is not None],
        best_chain=max([c['hits'] for c in contacts]or[0]),
        accepted_to_contact=delays,
        median_contact_delay=statistics.median(delays) if delays else None,
        finisher_to_next_basic=finisher_gaps,
        median_finisher_gap=statistics.median(finisher_gaps) if finisher_gaps else None,
        contact_gaps=contact_gaps,
        median_contact_gap=statistics.median(contact_gaps) if contact_gaps else None,
        p90_contact_gap=sorted(contact_gaps)[min(len(contact_gaps)-1,int(len(contact_gaps)*0.9))] if contact_gaps else None,
        cast_releases=releases,cast_recoveries=recoveries,
        first_cast_to_attack=(first_after_cast['frame']-first_cast['frame']) if first_after_cast else None)

def run_case(case,args,guest,host,rom):
    folder=args.out.resolve()/case['name'];folder.mkdir(parents=True,exist_ok=True)
    for name,target in [('B8CE.gba',rom),('assets',ROOT/'port/assets')]:
        path=folder/name
        if not path.exists():
            if sys.platform=='win32':
                if target.is_dir(): path.mkdir()
                else: shutil.copyfile(target,path)
            else: path.symlink_to(target,target_is_directory=target.is_dir())
    config=args.preset.read_text() if args.preset else (ROOT/'mod/action-edition/kh_com.ini').read_text()
    config=re.sub(r'(?m)^window_scale=.*$',f'window_scale={args.scale}',config)
    if not case.get('action',True):config=re.sub(r'(?m)^action_combat=.*$','action_combat=0',config)
    fingerprint=hashlib.sha256(json.dumps(dict(suite_version=10,
        suite_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),case=case,config=config,baseline=args.baseline,
        guest=hashlib.sha256(guest.read_bytes()).hexdigest(),host=hashlib.sha256(host.read_bytes()).hexdigest()),
        sort_keys=True).encode()).hexdigest()
    reportfile=folder/'report.json'
    if args.resume and reportfile.exists():
        previous=json.loads(reportfile.read_text())
        if previous.get('fingerprint')==fingerprint and previous['passed']and all(Path(p).exists()for p in previous['screenshots']):
            print(json.dumps(dict(name=case['name'],passed=True,resumed=True)),flush=True)
            return previous
    for old in folder.glob('host_shot*.bmp'):old.unlink()
    (folder/'kh_com.ini').write_text(config)
    # Match the packaged launcher's already-acknowledged local notice.
    # Action mode itself still uses the unchanged achievement safeguards.
    (folder/'rebel_mode.txt').write_text('unlocked\n')
    env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',
        KHCOM_FAST='1',KHCOM_PPU_THREADS='1',KHCOM_ACTION_TRACE='1',
        KHCOM_ASSETS=str(ROOT/'port/assets'),
        KHCOM_KEYS=','.join(case['keys']),KHCOM_QUIT=str(case['frames']+2),
        KHCOM_HOST_SHOT=','.join(map(str,case['shots'])))
    if case.get('preset'):env['KHCOM_PRACTICE']=case['preset']
    if 'catalog_index'in case:env['KHCOM_CATALOG_INDEX']=str(case['catalog_index'])
    if 'entry'in case:env['KHCOM_BATTLE']=f"{case['entry']}:0:{int(case.get('riku',False))}"
    began=time.monotonic()
    with (folder/'host.log').open('w') as log:
        try:
            code=subprocess.run([str(host),str(guest),'--data',str(folder),'--rom',str(rom)],env=env,cwd=ROOT,
                stdout=log,stderr=log,timeout=300).returncode
        except subprocess.TimeoutExpired:code='timeout'
    logfile=folder/'kh_com.log';log=logfile.read_text(errors='replace') if logfile.exists() else ''
    data=parse(log)
    passed=code==0 and 'CRASH'not in log
    if case.get('preset'):passed=passed and bool(data['starts']) and data['starts'][0]['frame']<100
    elif case['name']=='story-title':passed=passed and 'mode: mode_title'in log
    else:passed=passed and 'mode: mode_battle'in log
    if not args.baseline:
        if case['name']in('dense-ground','spam-full-animations'):
            enders=[c for c in data['contact_commands']if c['command']==NATIVE and c['name']=='Blitz']
            post_ender=[]
            for i,c in enumerate(data['contact_commands']):
                following=data['contact_commands'][i+1:i+4]
                if c['command']==NATIVE and len(following)==3:
                    post_ender.extend(following)
            complete=[s for s in data['native_sequences']if s['completion']]
            passed=passed and data['best_chain']>=6 and len(complete)>=2 and all(c['contact']is not None for c in enders+post_ender) and data['complete_or_linked'] and all(c['id']==(1,2,3,NATIVE)[i%4] for i,c in enumerate(data['commands'])) and all(s['segments']==[1,2,3] and s['contacts'] and s['damage'] and s['completion']['elapsed']>=150 for s in complete)
        elif case['name']in('one-press-one-swing','held-one-swing'):
            passed=passed and len(data['commands'])==1 and data['commands'][0]['id']==1 and len(data['animation_completions'])==1
        elif case['name']=='aerial-contact':passed=passed and any(c['air'] for c in data['commands']) and data['best_chain']>=3 and sum(s['completion']is not None and s['segments']==list(range(1,8)) and bool(s['contacts']) and s['damage'] for s in data['native_sequences'])>=3 and data['complete_or_defended']
        elif case['name']=='whiff-completes-in-place':passed=passed and [c['id']for c in data['commands']]==[1,2,3] and [s['command']for s in data['strikes']]==[1,2,3] and [s['command']for s in data['animation_completions']]==[1,2,3] and not data['contacts'] and not data['native_contacts'] and data['complete_or_linked'] and len(set((x,y)for x,y,z in data['attack_positions']))==1
        elif case['name']=='air-land-finisher':passed=passed and [c['id']for c in data['commands']]==[4,5,3,NATIVE] and bool(data['ender_contacts']) and any(s['segments']==[1,2,3] and s['completion']for s in data['native_sequences'])
        elif case['name']=='ground-jump-finisher':passed=passed and [c['id']for c in data['commands']]==[1,2,6,NATIVE] and bool(data['ender_contacts']) and any(s['segments']==list(range(1,8)) and s['completion']for s in data['native_sequences'])
        elif case['name']=='elemental-finisher-waits-for-landing':
            commands=data['commands'];completion=next((s for s in data['animation_completions']if s['command']==3),None)
            passed=passed and [c['id']for c in commands]==[1,2,3,NATIVE] and commands[3]['name']=='Fire Raid' and commands[3]['air']==0 and commands[3]['frame']>327 and completion is not None and bool(data['ender_contacts']) and any(s['completion']for s in data['native_sequences'])
            if completion and len(commands)==4:passed=passed and commands[3]['frame']-completion['frame']>36
        elif case['name']=='charged-spell-after-fire':
            sequences=data['native_sequences']
            passed=passed and [c['name']for c in data['commands']]==['Fire','Fira','Attack'] and len(data['cast_releases'])==1 and bool(data['cast_recoveries']) and len(sequences)==1
            if sequences and len(data['commands'])==3:
                sequence=sequences[0]
                # The BGFX hit happens after Sora's callback, so its own chain
                # delta has no synchronous trace. With only these two actions,
                # the followup's synchronous chain delta exposes both earlier
                # spell hits. The normal strike must also genuinely connect.
                passed=passed and data['commands'][1]['frame']>=case['earliest_native'] and data['commands'][1]['mp']==71 and sequence['completion']is not None and bool(sequence['contacts']) and data['contact_commands'][-1]['contact']is not None and data['best_chain']>len(sequence['contacts'])+1
        elif case['name'].startswith(('native-blitz-','native-sonic-')):
            sequence=data['native_sequences'][0]if data['native_sequences']else None
            passed=passed and sequence is not None and bool(sequence['contacts']) and sequence['damage']
            if sequence is None:sequence=dict(contacts=[],segments=[],completion=None,damage=False)
            if case.get('defense'):
                cancellation=next((c for c in data['deliberate_cancels']if c['kind']==case['defense']),None)
                passed=passed and cancellation is not None and sequence['segments']==list(range(1,case['expected_segments'])) and sequence['completion']is None
                if cancellation:
                    last_contact=max((h['frame']for h in sequence['contacts']if h['frame']<=cancellation['frame']),default=10**9)
                    passed=passed and case['probe_frame']<=cancellation['frame']<=case['probe_frame']+2 and cancellation['frame']<case['reference_completion'] and cancellation['frame']-last_contact>=case['contact_recovery']
            else:
                passed=passed and sequence['segments']==list(range(1,case['expected_segments']+1)) and sequence['completion']is not None
                if sequence and sequence['completion']:
                    finish=sequence['completion']['frame']
                    passed=passed and sequence['completion']['elapsed']>=case['minimum_native_ticks'] and not any(c['frame']<finish for c in data['deliberate_cancels']) and data['complete_or_linked']
                    if case['name']=='native-sonic-offensive-lock':
                        follow=[c for c in data['contact_commands']if c['accepted']>finish][:3]
                        passed=passed and [c['command']for c in follow]==[1,2,3] and all(c['contact']is not None for c in follow)
                if case['name'].endswith('-complete'):passed=passed and [c['id']for c in data['commands']]==[1,2,3,NATIVE] and all(c['contact']is not None for c in data['contact_commands'][:3])
        elif case.get('expected_native'):
            expected=case['expected_native']
            natives=[c for c in data['commands']if c['id']==NATIVE]
            sequence=data['native_sequences'][0]if data['native_sequences']else None
            passed=passed and [c['name']for c in natives]==[expected] and sequence is not None
            if sequence:
                passed=passed and sequence['completion']is not None
                if case.get('requires_damage'):passed=passed and bool(sequence['contacts'])
                if case.get('requires_status'):passed=passed and any(h['attack']==case['requires_status'] and h['status']=='stop' and h['duration']>0 for h in sequence['statuses'])
                if expected=='Blitz':passed=passed and sequence['segments']==[1,2,3]
                if expected=='Sonic Blade / Air Rush':passed=passed and sequence['segments']==list(range(1,8))
            if case.get('combo_route'):
                passed=passed and [c['id']for c in data['commands']]==[1,2,3,NATIVE] and all(c['contact']is not None for c in data['contact_commands'][:3]) and data['complete_or_linked']
            if case.get('spell_route') and natives:
                passed=passed and len(data['commands'])==1 and natives[0]['mp']==case['expected_mp']
                if 'charge_minimum'in case:passed=passed and natives[0]['frame']>=240+case['charge_minimum']-1
        elif case['name']in('roll-startup-cancel','guard-startup-cancel'):
            kind='dodge'if case['name'].startswith('roll')else'guard'
            passed=passed and any(c['kind']==kind and 181<c['frame']<=190 for c in data['deliberate_cancels']) and [c['id']for c in data['commands']]==[1,1] and not any(s['frame']<230 for s in data['strikes'])
            if kind=='guard':passed=passed and any(g['held']==0 and g['frame']>205 for g in data['guard'])
        elif case['name']=='guard-front':passed=passed and any(150<=b['frame']<=650 for b in data['blocks']) and any(f>650 and old>new for f,old,new in data['hp_changes'])
        elif case['name']=='guard-back':passed=passed and any(150<=f<=650 and old>new for f,old,new in data['hp_changes'])
        elif case['name']=='hitstop-reference':passed=passed and len(data['commands'])==1 and data['best_chain']==1
        elif case['name']=='hitstop-input':passed=passed and [c['id']for c in data['commands']]==[1,2] and data['best_chain']==2
        elif case['name']=='cycle-without-cast':passed=passed and not data['commands']
        elif case['name']=='cast-tap':passed=passed and [(c['name'],c['frame'])for c in data['commands']]==[('Fire',183)] and data['commands'][0]['mp']==88
        elif case['name']=='cast-to-melee':passed=passed and bool(data['cast_releases']) and bool(data['cast_recoveries']) and data['first_cast_to_attack'] is not None and data['first_cast_to_attack']<=22
        elif case['name']=='cast-queued':passed=passed and [c['name']for c in data['commands']]==['Fire','Fire'] and data['commands'][1]['frame']>201 and len(data['cast_releases'])==2
        elif case['name']=='cast-queue-cancel':passed=passed and [c['name']for c in data['commands']]==['Fire','Attack'] and len(data['cast_releases'])==1
        elif case['name']=='cast-queue-cycle-cancel':passed=passed and [c['name']for c in data['commands']]==['Fire'] and len(data['cast_releases'])==1
        elif case['name']=='aerial-cast':passed=passed and any(c['name']=='Fire'and c['air']for c in data['commands']) and bool(data['cast_releases']) and data['first_cast_to_attack'] is not None and data['first_cast_to_attack']<=22
        elif case['name']=='spell-weave-preserves-three-normals':
            commands=data['commands'];sequence=data['native_sequences'][0]if data['native_sequences']else None
            passed=passed and [c['id']for c in commands]==[1,2,13,3,NATIVE] and commands[-1]['name']=='Fire Raid' and [c['id']for c in commands if c['id']in NORMALS]==[1,2,3] and data['complete_or_linked']
            passed=passed and all(c['contact']is not None for c in data['contact_commands']if c['command']in NORMALS) and len(data['cast_releases'])==1 and data['cast_releases'][0]['timer']==14 and len(data['cast_recoveries'])==1 and data['cast_recoveries'][0]['timer']==20
            # Only three normal hits and one Fire are possible before the raid;
            # the third swing's synchronous delta must expose all four hits.
            if sequence:passed=passed and sequence['completion']is not None and bool(sequence['contacts']) and any(h['frame']<sequence['accepted'] and h['hits']>=4 for h in data['contacts'])
            else:passed=False
        elif case.get('expected_spell'):passed=passed and [c['name']for c in data['commands']]==[case['expected_spell']] and bool(data['cast_releases']) and data['cast_releases'][0]['timer']==14 and bool(data['cast_recoveries']) and data['cast_recoveries'][0]['timer']==20
        elif case['name']=='move-dodge':passed=passed and any('action: dodge'in x and 'left'in x for x in data['movement']) and any('action: dodge'in x and 'right'in x for x in data['movement']) and any('action: jump'in x for x in data['movement']) and any('action: cancel'in x for x in data['movement'])
        elif case['name']=='crowd-pressure':passed=passed and data['hp_loss']>0 and data['best_chain']>=3
        elif case['name']=='practice-retry':passed=passed and len(data['starts'])>=2 and len(data['menus'])>=2
        elif case['name']=='practice-switch':passed=passed and len(data['starts'])>=2 and data['starts'][-1]['preset']==1
        elif case['name']=='practice-title':passed=passed and 'practice: title'in log and 'mode: mode_title'in log
        elif case['name']=='practice-spell-exit':passed=passed and 'practice: title'in log and 'mode: mode_title'in log and len(data['commands'])==1
        elif case['name']=='practice-queued-retry':passed=passed and len(data['starts'])>=2 and len(data['commands'])==1
        elif case['name']=='visual-combo':passed=passed and data['best_chain']>=4 and bool(data['ender_contacts'])
        elif case['name']=='visual-crowd':passed=passed and data['best_chain']>=1 and any(c['name']=='Fire'for c in data['commands']) and bool(data['cast_releases'])
        elif case['name']=='visual-menu':passed=passed and bool(data['menus'])
        elif case['name']=='visual-cast-wait':passed=passed and [c['name']for c in data['commands']]==['Fire','Fire']
        elif case['name']=='native-counter':passed=passed and bool(data['perfect_dodges']) and bool(data['counters'])
        elif 'fallback'in case['name']:passed=passed and not data['commands']
    shots=[folder/f'host_shot_{n}.bmp'for n in case['shots']]
    missing=[p.name for p in shots if not p.exists()];passed=passed and not missing
    result=dict(name=case['name'],passed=bool(passed),exit=code,elapsed_seconds=round(time.monotonic()-began,3),
        fingerprint=fingerprint,cheats=False,keys=case['keys'],frames=case['frames'],missing_captures=missing,
        screenshots=[str(p)for p in shots],**data)
    (folder/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:result[k]for k in ('name','passed','exit','best_chain','median_contact_delay','median_finisher_gap')}),flush=True)
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--guest',type=Path,default=ROOT/'build/windows64/out/guest-us.bin')
    p.add_argument('--host',type=Path,default=ROOT/'build/windows64/out/rekhcom-linux-test')
    p.add_argument('--rom',type=Path,default=ROOT/'roms/B8CE.gba')
    p.add_argument('--out',type=Path,default=ROOT/'build/cohesion-final')
    p.add_argument('--only');p.add_argument('--jobs',type=int,default=1)
    p.add_argument('--scale',type=int,default=2,choices=(1,2,3,4))
    p.add_argument('--preset',type=Path);p.add_argument('--baseline',action='store_true')
    p.add_argument('--resume',action='store_true',help='reuse passing identical-binary/config/input cases')
    args=p.parse_args();available={c['name']:c for c in scenarios()}
    wanted=list(dict.fromkeys(args.only.split(',')))if args.only else [n for n,c in available.items()if not c.get('visual')]
    if any(n not in available for n in wanted):p.error('unknown scenario')
    args.out.mkdir(parents=True,exist_ok=True);guest=args.guest.resolve();host=args.host.resolve();rom=args.rom.resolve()
    sha=hashlib.sha256(guest.read_bytes()).hexdigest()
    references=[]
    for reference_name in dict.fromkeys(available[n]['effect_reference']for n in wanted if 'effect_reference'in available[n]):
        reference=run_case(available[reference_name],args,guest,host,rom)
        references.append(reference)
        if not reference['passed']or len(reference['commands'])<2:
            raise RuntimeError('cannot measure existing effect-channel queue: '+reference_name)
        for name in wanted:
            if available[name].get('effect_reference')==reference_name:
                available[name]['earliest_native']=reference['commands'][1]['frame']
        wanted=[n for n in wanted if n!=reference_name]
    if 'hitstop-input'in wanted:
        # Establish the actual collision frame in a separate native run, then
        # inject the next one-frame press during the ensuing two-frame pause.
        # This adapts to real animation timing rather than assuming a startup.
        reference=run_case(available['hitstop-reference'],args,guest,host,rom)
        references.append(reference)
        if not reference['passed']or not reference['contacts']:
            raise RuntimeError('cannot measure the real first-contact frame for hitstop input')
        press=reference['contacts'][0]['frame']+1
        available['hitstop-input']['keys']=['180-181:A',f'{press}-{press+1}:A']
        wanted=[n for n in wanted if n!='hitstop-reference']
    # Measure this binary's actual final-contact and completion frames before
    # probing its late defense window. A roll after neutral is not a cancel.
    for reference_name in dict.fromkeys(available[n]['reference']for n in wanted if 'reference'in available[n]):
        reference=run_case(available[reference_name],args,guest,host,rom)
        references.append(reference)
        if not reference['passed']or not reference['native_sequences']:
            raise RuntimeError('cannot measure complete native finisher: '+reference_name)
        sequence=reference['native_sequences'][0]
        completion=sequence['completion']['frame']
        last_contact=max(h['frame']for h in sequence['contacts'])
        for name in wanted:
            case=available[name]
            if case.get('reference')!=reference_name:continue
            # Account for the native one-frame input sampling boundary.
            press=max(last_contact+case['contact_recovery']-1,completion-4)
            case['probe_frame']=press
            case['reference_completion']=completion
            button='GUARD'if case['defense']=='guard'else'DODGE+LEFT'
            case['keys']+= [f'{press}-{press+(8 if case["defense"]=="guard"else 1)}:{button}']
        wanted=[n for n in wanted if n!=reference_name]
    with ThreadPoolExecutor(max_workers=args.jobs)as pool:
        reports=list(pool.map(lambda c:run_case(c,args,guest,host,rom),[available[n]for n in wanted]))
    reports=references+reports
    result=dict(guest_sha256=sha,host_sha256=hashlib.sha256(host.read_bytes()).hexdigest(),
        guest_unchanged=sha==hashlib.sha256(guest.read_bytes()).hexdigest(),
        tests=reports,passed=all(x['passed']for x in reports),baseline=args.baseline,
        window_scale=args.scale,preset=str(args.preset)if args.preset else 'shipping preset with scale override',
        scope=('Actual x32 game image through Windows SDL host' if sys.platform=='win32' else
               'Actual x32 game image through Linux SDL host')+'; isolated profiles, no full campaign claim')
    (args.out/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    return 0 if result['passed']and result['guest_unchanged']else 1
if __name__=='__main__':raise SystemExit(main())
