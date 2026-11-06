"""Original PCM cues and a short three-theme score; no external recordings."""
from pathlib import Path
import json, math, random, struct, wave
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets/audio';OUT.mkdir(parents=True,exist_ok=True)
RATE=22050
def save(name,samples):
    peak=max(1.,max(abs(x) for x in samples))
    data=b''.join(struct.pack('<h',int(max(-1,min(1,x/peak))*.85*32767)) for x in samples)
    with wave.open(str(OUT/(name+'.wav')),'wb') as out:out.setnchannels(1);out.setsampwidth(2);out.setframerate(RATE);out.writeframes(data)
def tone(freq,t):return math.sin(2*math.pi*freq*t)+.16*math.sin(2*math.pi*freq*2*t)
def cue(name,duration,fn):save(name,[fn(i/RATE) for i in range(int(RATE*duration))])
rng=random.Random(7831)
cue('hit',.18,lambda t:(rng.uniform(-1,1)*.20+tone(120+900*math.exp(-t*45),t)*.3)*math.exp(-t*27))
cue('block',.26,lambda t:(tone(540,t)+.4*tone(810,t))*.3*math.exp(-t*18))
cue('heal',.65,lambda t:tone(440+330*t,t)*.22*math.sin(math.pi*t/.65)**2)
cue('boss',.7,lambda t:(tone(110,t)+.5*tone(165,t))*.25*math.sin(math.pi*t/.7)**2)
cue('ultimate',1.1,lambda t:tone(180+900*t*t,t)*.25*math.sin(math.pi*t/1.1)**2)
cue('reward',.65,lambda t:tone(659 if t<.3 else 880,t)*.22*math.exp(-((t% .3)*8)))
cue('select',.1,lambda t:tone(660,t)*.16*math.sin(math.pi*t/.1)**2)
for name,notes in [('victory',[523,659,784,1046]),('defeat',[392,349,294,262])]:
    cue(name,1.2,lambda t,notes=notes:tone(notes[min(3,int(t/.3))],t)*.22*math.sin(math.pi*(t%.3)/.3)**2)
melodies=[([60,64,67,72,67,64,62,67],[48,53,55,48]),([57,60,64,67,64,60,59,64],[45,50,52,45]),([62,65,69,74,69,65,64,69],[50,46,48,50])]
for act,(notes,bass) in enumerate(melodies):
    samples=[];beat=.4;duration=25.6
    for i in range(int(RATE*duration)):
        t=i/RATE;step=int(t/beat);local=t%beat;freq=440*2**((notes[step%8]-69)/12)
        lead=tone(freq,t)*.10*math.sin(math.pi*local/beat)**2
        root=440*2**((bass[(step//8)%4]-69)/12)
        pad=(math.sin(2*math.pi*root*t)+.35*math.sin(2*math.pi*root*1.5*t))*.045
        # Smooth loop boundaries; the harmony resolves to its root.
        fade=min(1,t/.2,(duration-t)/.2);samples.append((lead+pad)*max(0,fade))
    save('music_'+str(act),samples)
definitions=[dict(id=name,file='assets/audio/'+name+'.wav',volume=155 if name not in ('boss','ultimate') else 120) for name in ['hit','block','heal','boss','ultimate','reward','victory','defeat','select']]
definitions += [dict(id='music_'+str(i),file=f'assets/audio/music_{i}.wav',volume=95,loop=True) for i in range(3)]
(ROOT/'data/audio.json').write_text(json.dumps(definitions,indent=2)+'\n',encoding='utf-8')
print('Audio: 9 cues, 3 original looping themes, 22050 Hz mono PCM16.')
