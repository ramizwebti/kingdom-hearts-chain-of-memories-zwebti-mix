#!/usr/bin/env python3
"""Measure independent native assists with ordinary inputs and an owned ROM.

Includes natural contact-earned gauge, shared spell-channel scheduling,
Sora combat/defense beside real companions, healing that preserves damage,
two-encounter session carry, serialization and native cosmetic cleanup.
All profiles and captures stay in this checkout's ignored build directory.
"""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import argparse,hashlib,json,os,re,struct,subprocess,time

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'build/assist-native'
HOST=ROOT/'build/windows64/out/rekhcom64.exe'
GUEST=ROOT/'build/windows64/out/guest-us.bin'
ROM=ROOT/'roms/B8CE.gba'
TIMEOUT=360
CONFIG=(ROOT/'mod/action-edition/kh_com.ini').read_text()
for key,value in dict(action_combat=1,window_scale=2,cheat_one_hit=0,cheat_infinite_hp=0,cheat_savestates=0).items():
    CONFIG=re.sub(r'(?m)^'+key+r'=.*$',key+'='+str(value),CONFIG)

CASES=[
 dict(name='cloud-fight-finisher',index=73,preset='training',frames=1800,
      keys='180-181:TECHNIQUE,200-201:A,230-231:A,265-266:A,310-311:A,550-565:GUARD,600-601:DODGE+LEFT,650-680:RIGHT,710-711:A',shots=[220,285,400,565,605,740,1250]),
 dict(name='mushu-fight-defenses',index=70,preset='training',frames=1800,
      keys='180-181:TECHNIQUE,200-201:A,230-231:A,265-266:A,320-335:GUARD,350-351:DODGE+LEFT,390-410:RIGHT,440-441:A,470-471:A,510-511:A,555-556:B,570-571:A',shots=[220,285,325,355,400,580,1000]),
 dict(name='tink-real-hurt',index=67,preset='mixed',frames=1500,
      keys='180-181:TECHNIQUE,500-520:GUARD,550-551:A,580-581:A,620-621:A',shots=[200,300,400,500,650,1000]),
 dict(name='genie-real-hurt',index=58,preset='mixed',frames=1800,
      keys='180-181:TECHNIQUE,200-201:A,230-231:A,270-271:A,310-325:GUARD,350-351:DODGE+LEFT,390-410:RIGHT',shots=[220,285,315,355,500,850,1250]),
 dict(name='bambi-serialized-after-hurt',index=61,preset='mixed',frames=1800,
      keys='180-181:TECHNIQUE,400-401:TECHNIQUE,500-501:A,550-560:GUARD,600-601:DODGE+LEFT',shots=[200,350,450,550,650,1000,1500]),
 dict(name='native-finisher-accent',index=2,preset='training',frames=850,
      keys='180-181:A,210-211:A,250-251:A,300-301:A',shots=[195,230,280,340,430,550,800]),
 dict(name='native-magic-accent',index=18,preset='training',frames=650,
      keys='180-181:TECHNIQUE',shots=[190,210,230,260,400,600]),
 dict(name='cloud-trinity-queued',index=73,preset='training',frames=1800,
      keys='120-121:R,140-141:R,160-161:R,180-181:TECHNIQUE,300-301:A,330-331:A,365-366:A',shots=[220,350,440,490,600,850,1250]),
]
earned_keys=[]
for start in range(180,901,180):
    earned_keys += [f'{start}-{start+1}:A',f'{start+30}-{start+31}:A',
                   f'{start+65}-{start+66}:A',f'{start+105}-{start+165}:GUARD']
earned_keys += ['1100-1135:ASSIST','1170-1180:GUARD','1190-1200:RIGHT','1240-1241:A',
               '1280-1290:GUARD','1320-1321:DODGE+LEFT']
CASES.append(dict(name='simba-earned-gauge',index=None,preset='training',frames=1800,
    keys=','.join(earned_keys),shots=[200,400,800,1090,1150,1250,1450,1750]))
CASES.append(dict(name='session-carry',index=None,preset='training',frames=1000,
    keys='180-181:A,210-211:A,245-246:A,300-301:ASSIST_NEXT,310-311:ASSIST_NEXT,320-321:R,330-331:R,340-341:WEAPON_NEXT,350-351:WEAPON_NEXT,360-361:WEAPON_NEXT,450-451:SELECT,550-551:A',
    shots=[200,280,400,500,600,800,950]))
magic_keys=[]
for start in range(180,901,180):
    magic_keys += [f'{start}-{start+1}:A',f'{start+30}-{start+31}:A',
                   f'{start+65}-{start+66}:A',f'{start+105}-{start+165}:GUARD']
magic_keys += [f'{frame}-{frame+1}:ASSIST_NEXT' for frame in (1055,1065,1075,1085,1095)]
magic_keys += ['1130-1165:ASSIST','1210-1211:CAST','1300-1320:LEFT','1340-1341:RIGHT','1500-1510:GUARD']
CASES.append(dict(name='mushu-earned-gauge-magic',index=None,preset='training',frames=2400,
    keys=','.join(magic_keys),shots=[1000,1120,1190,1230,1300,1500,2000,2300]))

def rows(log,kind):
    result=[]
    for line in log.splitlines():
        if 'action: '+kind+' ' not in line: continue
        row={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)(?=\s|,|$)',line)}
        row['line']=line
        result.append(row)
    return result

def run(case):
    reference=None
    if case['name']=='cloud-trinity-queued':
        # Derive a fresh, short-lived request from this exact companion and
        # normal string, rather than requiring a stale request to wait forever.
        ref=dict(case,name='cloud-trinity-reference',frames=800,
                 shots=[220,350,440,500,760])
        reference=run(ref)
        if not reference['passed'] or not reference['data']['assist-end']:
            raise RuntimeError('cannot derive Trinity queue from valid native companion reference')
        end=reference['data']['assist-end'][0]['frame']
        normal_done=[r['frame'] for r in reference['data']['animation-complete'] if r['command']<=3]
        if len(normal_done)!=3:
            raise RuntimeError('Trinity reference must finish exactly three real normals')
        release=end-20
        charge=max(normal_done[-1]+1,release-25)
        if charge>normal_done[-1]+34 or not 13<=release-charge<30:
            raise RuntimeError('no legitimate charged-finisher queue window before companion exit')
        case=dict(case,keys=case['keys']+f',{charge}-{release}:A+MODIFIER')
    folder=OUT/case['name'];folder.mkdir(parents=True,exist_ok=True)
    for name in ('host.log','kh_com.log')+tuple('host_shot_%d.bmp'%f for f in case['shots']):
        (folder/name).unlink(missing_ok=True)
    (folder/'kh_com.ini').write_text(CONFIG)
    (folder/'rebel_mode.txt').write_text('unlocked\n')
    env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',
       KHCOM_FAST='1',KHCOM_PPU_THREADS='1',KHCOM_ACTION_TRACE='1',KHCOM_ASSETS=str(ROOT/'port/assets'),
       KHCOM_PRACTICE=case['preset'],KHCOM_CATALOG_INDEX=str(case['index']),KHCOM_KEYS=case['keys'],
       KHCOM_QUIT=str(case['frames']+2),KHCOM_HOST_SHOT=','.join(map(str,case['shots'])))
    if case['index'] is None: env.pop('KHCOM_CATALOG_INDEX',None)
    began=time.monotonic()
    with (folder/'host.log').open('w') as output:
        try:
            code=subprocess.run([str(HOST),str(GUEST),'--data',str(folder),
                  '--rom',str(ROM)],cwd=ROOT,env=env,stdout=output,stderr=output,timeout=TIMEOUT).returncode
        except subprocess.TimeoutExpired:
            code='timeout'
    log=(folder/'kh_com.log').read_text(errors='replace') if (folder/'kh_com.log').exists() else ''
    data={kind:rows(log,kind) for kind in ['assist-start','assist-state','assist-pause','assist-end',
          'accepted','animation-complete','native-segment','native-complete','native-settled','hp','guard','dodge',
          'accent-start','accent-state','finisher-contact','chain','assist-contact','assist-heal','session','cast-release',
          'strike','melee-contact']}
    failures=[]
    if code: failures.append('process exit '+str(code))
    if re.search(r'CRASH|out of memory|assertion failed|runtime error:',log,re.I): failures.append('runtime fault')
    for frame in case['shots']:
        path=folder/('host_shot_%d.bmp'%frame)
        header=path.read_bytes()[:54] if path.is_file() else b''
        if len(header)<54 or header[:2]!=b'BM' or not all(struct.unpack_from('<ii',header,18)):
            failures.append('missing/invalid capture '+path.name)
    if case['name'].startswith(('cloud','mushu','tink','genie','bambi','simba')):
        if not data['assist-start']: failures.append('no independent assist start')
        if not data['assist-end']: failures.append('assist did not finish')
        if any(not row.get('visible') for row in data['assist-state']): failures.append('Sora hidden in assist snapshot')
        if 'native-dispatch' in '\n'.join(x for x in log.splitlines() if 'SUMMON' in x): failures.append('takeover dispatch')
        intervals=list(zip(data['assist-start'],data['assist-end']))
        active=lambda row:any(a['frame']<=row['frame']<=b['frame'] for a,b in intervals)
        data['active_normal_ids']=[r['id'] for r in data['accepted'] if r.get('id',0)<=6 and active(r)]
        data['active_guards']=[r for r in data['guard'] if active(r)]
        data['active_dodges']=[r for r in data['dodge'] if active(r)]
        data['active_damage']=[r for r in data['hp'] if r.get('new',0)<r.get('old',0) and active(r)]
        data['active_healing']=[r for r in data['assist-heal'] if r.get('new',0)>r.get('old',0) and active(r)]
    if case['name'] in ('cloud-fight-finisher','mushu-fight-defenses'):
        if not all(n in data['active_normal_ids'] for n in (1,2,3)): failures.append('three Sora normals not accepted while assist active')
        if not data['active_guards']: failures.append('no guard while assist active')
        if not data['active_dodges']: failures.append('no dodge while assist active')
    if case['name']=='cloud-fight-finisher':
        if not data['native-complete']: failures.append('full Sora finisher did not finish beside assist')
        if not any(r.get('paused') for r in data['assist-pause']): failures.append('assist not safely paused for player presentation')
    if case['name']=='cloud-trinity-queued':
        trinity=[r for r in data['accepted'] if 'Trinity Limit' in r['line']]
        if not trinity: failures.append('queued Trinity never accepted')
        if not data['native-complete'] or not data['finisher-contact']: failures.append('queued Trinity did not finish with real contact')
        if [r['id'] for r in data['accepted']]!=[1,2,3,20] or [r['command'] for r in data['strike']]!=[1,2,3]:
            failures.append('queued Trinity must follow three legitimate normal positions')
        # Cloud can knock the barrel outside normal reach before this string;
        # three performed whiffs must still earn the fourth-input finisher.
        # Contact-earned gauge is tested strictly in the two earned cases.
        end=reference['data']['assist-end'][0]['frame']
        if not release+1<end: failures.append('Trinity request did not actually wait for companion exit')
        if trinity and not end<=trinity[0]['frame']<=release+42:
            failures.append('Trinity overlapped companion or outlived its fresh request budget')
        if trinity and any(r['frame']>=trinity[0]['frame'] for kind in ('assist-contact','assist-state') for r in data[kind]):
            failures.append('native companion advanced or dealt damage after Trinity admission')
        # The wrapper can remain nine frames longer while observing Trinity's
        # new fade; require damage after its actual cleanup as an extra check.
        cleanup=data['assist-end'][0]['frame'] if data['assist-end'] else 0
        if any(r['frame']<max(end,cleanup) for r in data['finisher-contact']):
            failures.append('Trinity damage occurred before companion finished')
    if case['name'] in ('tink-real-hurt','genie-real-hurt','bambi-serialized-after-hurt') and not data['active_damage']:
        failures.append('no observed real Sora damage while companion active')
    if case['name']=='tink-real-hurt' and not data['active_healing']:
        failures.append('Tink did not actually heal new damage after full-HP spawn')
    if case['name'] in ('cloud-fight-finisher','mushu-fight-defenses','genie-real-hurt') and not data['assist-contact']:
        failures.append('no actual offensive companion collision damage')
    if case['name']=='bambi-serialized-after-hurt' and len(data['assist-start'])!=2:
        failures.append('two serialized independent assists not completed')
    if case['name'] in ('simba-earned-gauge','mushu-earned-gauge-magic'):
        before=1100 if case['name']=='simba-earned-gauge' else 1130
        normals=[r['id'] for r in data['accepted'] if r['frame']<before]
        contacts=[r['command'] for r in data['melee-contact'] if r['frame']<before]
        if normals!=[1,2,3]*5 or contacts!=[1,2,3]*5:
            failures.append('ordinary contact string not played before earned assist')
        if not data['session'] or data['session'][0]['gauge']!=0:
            failures.append('earned companion must start from zero session gauge')
        if not data['assist-start'] or data['assist-start'][0]['frame']<before:
            failures.append('assist did not wait for real earned gauge and summon input')
    if case['name']=='simba-earned-gauge':
        if not data['assist-contact']: failures.append('earned Simba has no actual damage')
    if case['name']=='session-carry':
        if len(data['session'])<2: failures.append('two fresh native encounters were not entered')
        else:
            first,second=data['session'][:2]
            if first.get('carry') or first.get('gauge'): failures.append('fresh process starts with earned meter')
            expected=dict(carry=1,gauge=24,family=2,magic=2,keyblade=3,queued=0)
            if any(second.get(k)!=v for k,v in expected.items()): failures.append('session resources/selections did not carry with clean queue')
    if case['name']=='mushu-earned-gauge-magic':
        if not any(r.get('command')==13 for r in data['cast-release']): failures.append('production Fire never actually released while assist active')
        if not any(r.get('paused') and r.get('player') for r in data['assist-pause']): failures.append('assist did not safely yield its shared channel to player magic')
        if not data['assist-contact']: failures.append('Mushu did not resume with actual damage after player magic')
    if 'accent' in case['name']:
        if not data['accent-start']: failures.append('no native accent')
        if not data['accent-state'] or data['accent-state'][-1].get('tasks')!=0: failures.append('accent pool did not clean')
        if any(r.get('tasks',0)>2 for r in data['accent-state']): failures.append('cosmetic pool exceeded two native tasks')
    result=dict(case=case,exit=code,seconds=round(time.monotonic()-began,3),passed=not failures,
       failures=failures,data=data,screenshots=[str(folder/('host_shot_%d.bmp'%f)) for f in case['shots']],
       cheats=False,diagnostic_catalog=case['index'] is not None,
       guest_sha256=hashlib.sha256(GUEST.read_bytes()).hexdigest())
    if reference is not None:
        result['queue_reference']=dict(case=reference['case']['name'],end=reference['data']['assist-end'][0]['frame'],
                                     charge=charge,release=release,guest_sha256=reference['guest_sha256'])
    (folder/'report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(name=case['name'],passed=result['passed'],failures=failures,
       start=[r['frame'] for r in data['assist-start']],end=[r['frame'] for r in data['assist-end']])),flush=True)
    return result

def main():
    global OUT,HOST,GUEST,ROM,TIMEOUT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host',type=Path,default=HOST)
    parser.add_argument('--guest',type=Path,default=GUEST)
    parser.add_argument('--rom',type=Path,default=ROM)
    parser.add_argument('--out',type=Path,default=OUT)
    parser.add_argument('--jobs',type=int,choices=(1,2),default=2)
    parser.add_argument('--timeout',type=int,default=TIMEOUT)
    parser.add_argument('--only',help='comma-separated exact probe names')
    args=parser.parse_args()
    HOST=args.host.resolve();GUEST=args.guest.resolve();ROM=args.rom.resolve();OUT=args.out.resolve()
    TIMEOUT=args.timeout
    for name,path in (('host',HOST),('guest',GUEST),('rom',ROM)):
        if not path.is_file(): parser.error('missing --'+name+': '+str(path))
    if not OUT.is_relative_to((ROOT/'build').resolve()): parser.error('--out must stay inside this checkout build directory')
    if TIMEOUT<1: parser.error('--timeout must be positive')
    cases=CASES
    if args.only:
        requested=args.only.split(',')
        unknown=set(requested)-{case['name'] for case in CASES}
        if unknown: parser.error('unknown probe: '+', '.join(sorted(unknown)))
        cases=[case for case in CASES if case['name'] in requested]
    fingerprint={name:hashlib.sha256(path.read_bytes()).hexdigest()
        for name,path in (('host_sha256',HOST),('guest_sha256',GUEST))}
    OUT.mkdir(parents=True,exist_ok=True)
    with ThreadPoolExecutor(max_workers=args.jobs) as pool: results=list(pool.map(run,cases))
    unchanged=all(fingerprint[name]==hashlib.sha256(path.read_bytes()).hexdigest()
        for name,path in (('host_sha256',HOST),('guest_sha256',GUEST)))
    report=dict(**fingerprint,passed=all(r['passed'] for r in results) and unchanged,
        binaries_unchanged=unchanged,tested=len(results),cheats=False,rom_copied=False,tests=results)
    (OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],tested=len(results),binaries_unchanged=unchanged)),flush=True)
    return 0 if report['passed'] else 1

if __name__=='__main__':
    raise SystemExit(main())
