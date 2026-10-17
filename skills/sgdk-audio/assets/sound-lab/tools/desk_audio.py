"""Original bounded FM/PSG/DAC authoring fixtures, not a general VGM exporter."""
import io
import struct
import wave
FNUM=(644,682,722,766,810,860,911,964,1022,1082,1147,1217)
TRACKS=('harbor','journey','danger','fanfare','bell','reed','bass','psg','drums')


def percussion(noise,length):
    state=0xACE1;out=[]
    for i in range(length):
        state=(state>>1)^(0xB400 if state&1 else 0)
        period=45+i//12;phase=i%period
        value=((state&255)-128)//2 if noise else 4*min(phase,period-phase)*88//period-88
        out.append(128+value*(length-i)//length)
    return bytes(out)


def score(name):
    if name not in TRACKS:raise ValueError('Unknown score')
    solo=name in TRACKS[4:];frames=240 if solo else 144 if name=='fanfare'else 480
    step=30 if solo else 12 if name in ('danger','fanfare')else 20 if name=='journey'else 30
    root=43 if name=='danger'else 50 if name=='harbor'else 55
    scale=(0,2,3,5,7,8,10)if name=='danger'else(0,2,4,5,7,9,11)
    melody=(0,2,4,6,4,2,1,3,5,4,2,0,1,3,2,0)
    header=bytearray(256);header[:4]=b'Vgm '
    for pos,value in [(8,0x160),(12,3579545),(0x18,frames*735),(0x24,60),(0x2c,7670454),(0x34,204)]:struct.pack_into('<I',header,pos,value)
    struct.pack_into('<H',header,0x28,9);header[0x2a]=16;data=bytearray()
    drums=name not in ('bell','reed','bass','psg','fanfare','harbor')
    kick,snare=percussion(False,1330),percussion(True,1596)
    if drums:
        bank=kick+snare;data+=b'\x67\x66\x00'+struct.pack('<I',len(bank))+bank
        data+=b'\x90\x00\x02\x00\x2a\x91\x00\x00\x01\x00\x92\x00'+struct.pack('<I',13300)
    def reg(ch,register,value):data.extend((0x52 if ch<3 else 0x53,register+ch%3,value))
    def key(ch,on):data.extend((0x52,0x28,(0xf0 if on else 0)|(ch if ch<3 else ch+1)))
    def note(ch,midi):
        key(ch,False);f=FNUM[midi%12];reg(ch,0xa4,((midi//12-1)<<3)|(f>>8));reg(ch,0xa0,f&255);key(ch,True)
    def degree(n):return scale[n%7]+12*(n//7)
    data+=bytes((0x52,0x22,0,0x52,0x27,0,0x52,0x2b,0x80 if drums else 0))
    for ch in range(6):key(ch,False)
    for value in (0x9f,0xbf,0xdf,0xff):data.extend((0x50,value))
    for ch in range(5):
        patch=name if solo else 'bell'if ch==0 else 'bass'if ch==3 else 'reed'
        for slot in (0,4,8,12):
            carrier=slot in (8,12)
            level=(32 if ch==0 else 43 if ch==3 else 55)if carrier else (42 if patch=='bell'else 59 if patch=='reed'else 72)
            for register,value in [(0x30,1 if carrier else 3 if patch=='bell'else 1),(0x40,level),(0x50,26 if patch=='bell'else 16),
                (0x60,10 if patch=='bell'else 5),(0x70,4),(0x80,0x47 if patch=='bell'else 0x36),(0x90,0)]:reg(ch,register+slot,value)
        reg(ch,0xb0,4);reg(ch,0xb4,0xc0)
    loop_at=256+len(data)
    if name!='fanfare':
        struct.pack_into('<I',header,0x1c,loop_at-0x1c);struct.pack_into('<I',header,0x20,frames*735)
    for frame in range(frames):
        if frame%step==0:
            beat=frame//step;m=melody[beat%16]
            if name not in ('psg','drums'):
                note(0,root+(0 if name=='bass'else 12)+degree(m))
                if not solo and name!='fanfare'and beat%4==0:
                    chord=(0,3,4,0)[beat//4%4];note(1,root+degree(chord));note(2,root+degree(chord)+7);note(3,root-12+degree(chord))
            if name=='psg'or not solo:
                period=[214,190,170,160,143,127,113,107][beat%8]
                data.extend((0x50,0x80|(period&15),0x50,period>>4,0x50,0x99 if name=='psg'else 0x9c))
        if frame%step==step-4:
            for ch in range(5):key(ch,False)
            data.extend((0x50,0x9f))
        if drums and frame%15==0:
            offset,length=(0,len(kick))if frame%30==0 else(len(kick),len(snare))
            data+=b'\x94\x00\x93\x00'+struct.pack('<I',offset)+b'\x01'+struct.pack('<I',length)
        data.append(0x62)
    for ch in range(6):key(ch,False)
    for value in (0x9f,0xbf,0xdf,0xff):data.extend((0x50,value))
    if drums:data+=b'\x94\x00'
    data.append(0x66);struct.pack_into('<I',header,4,256+len(data)-4)
    return bytes(header+data)


def sound(kind,clean=True):
    if kind not in range(4):raise ValueError('Unknown effect')
    length=[3724,4655,5320,8645][kind];samples=[];state=0xACE1;phase=0
    for i in range(length):
        period=14+i*8//length if kind==0 else 75-i*55//length if kind==1 else 35+i*110//length if kind==2 else (24 if i<length//2 else 18)
        phase=(phase+1)%period;triangle=4*min(phase,period-phase)*9000//period-9000
        state=(state>>1)^(0xB400 if state&1 else 0)
        v=triangle if kind!=2 else (triangle+((state&255)-128)*45)//2
        if clean:
            v=v*min(i,65)//65;v=v*(length-1-i)//length
        samples.append(v)
    buffer=io.BytesIO()
    with wave.open(buffer,'wb')as wav:
        wav.setparams((1,2,13300,0,'NONE','not compressed'));wav.writeframes(struct.pack('<'+'h'*length,*samples))
    return buffer.getvalue()


def assets():
    files={name+'.vgm':score(name)for name in TRACKS}
    for kind,name in enumerate(('coin','jump','impact','alert')):
        for clean in (True,False):files[name+(''if clean else '_raw')+'.wav']=sound(kind,clean)
    return files
