#!/usr/bin/env python3
"""Send bounded logical pad/mouse input to a cooperating EverDrive ROM.

No reset, ROM/SD/save writes, BIOS input or electrical controller emulation.
"""
import argparse
import binascii
import json
import math
import secrets
import struct
import sys
import time
from usb_transport import USBTransport, ProtocolError, open_serial, positive_seconds

BUTTONS={'UP':1,'DOWN':2,'LEFT':4,'RIGHT':8,'B':0x10,'C':0x20,'A':0x40,
         'START':0x80,'Z':0x100,'Y':0x200,'X':0x400,'MODE':0x800}
MOUSE={'LEFT':1,'RIGHT':2,'MIDDLE':4,'START':8}
ERRORS={1:'unknown command',2:'wrong/unopened session',3:'stale/duplicate sequence',4:'invalid arguments'}


def buttons(text, mapping=BUTTONS):
    if text.upper() in ('NONE','0',''):return 0
    value=0
    for name in text.upper().replace('+',',').split(','):
        if name not in mapping:raise ValueError('Unknown button: '+name)
        value|=mapping[name]
    return value


def pad(held):
    if not 0<=held<=0xfff:raise ValueError('pad buttons need a 12-bit mask')
    return (1,held,0,0)


def mouse(dx=0,dy=0,held=0):
    if not -2047<=dx<=2047 or not -2047<=dy<=2047 or not 0<=held<=15:
        raise ValueError('mouse needs deltas -2047..2047 and button mask 0..15')
    return (2,held,dx,dy)


def packet(kind,session,sequence,ports=None,ticks=0,mask=0):
    ports={} if ports is None else ports
    if not 0<session<=0xffffffff or not 0<=sequence<=0xffffffff:raise ValueError('invalid session/sequence')
    if kind==2:
        if not 1<=ticks<=120 or not ports or any(p not in (1,2) for p in ports):raise ValueError('state needs ports 1/2 and ticks 1..120')
        mask=sum(1<<(p-1) for p in ports)
    elif kind==3:
        if mask not in (1,2,3):raise ValueError('release needs port mask 1..3')
    elif kind not in (1,4):raise ValueError('invalid command kind')
    body=bytearray(38)
    struct.pack_into('>4sBBHII',body,0,b'UIV1',kind,mask,ticks,session,sequence)
    for p,value in ports.items():
        mode,held,dx,dy=value
        if mode==1:pad(held)
        elif mode==2:mouse(dx,dy,held)
        else:raise ValueError('port kind must be pad or mouse')
        if mode==1 and (dx or dy):raise ValueError('pads cannot carry mouse movement')
        struct.pack_into('>BBHhh',body,16+(p-1)*8,mode,0,held,dx,dy)
    return bytes(body)+struct.pack('>H',binascii.crc_hqx(body,0xffff))


def reply(data,session,sequence):
    if len(data)!=40 or data[:5]!=b'UIV1\x80' or binascii.crc_hqx(data[:38],0xffff)!=int.from_bytes(data[38:],'big'):
        return None
    if struct.unpack_from('>II',data,8)!=(session,sequence):return None
    result={'session':session,'sequence':sequence,'tick':int.from_bytes(data[32:36],'big'),
            'active_mask':data[5],'remaining_ticks':int.from_bytes(data[6:8],'big'),
            'result':data[36],'capabilities':data[37],'ports':[]}
    for offset in (16,24):
        kind,remote,held,dx,dy=struct.unpack_from('>BBHhh',data,offset)
        result['ports'].append({'kind':kind,'remote':bool(remote),'held':held,'dx':dx,'dy':dy})
    return result


class RemoteInput:
    def __init__(self,transport,ack_timeout=5.0):
        if not math.isfinite(ack_timeout) or ack_timeout<=0:raise ValueError("ACK timeout must be positive and finite")
        self.transport=transport;self.ack_timeout=ack_timeout;self.session=0;self.sequence=0

    def _exchange(self,data,sequence):
        self.transport._require_identity()
        self.transport._fifo(data)
        end=self.transport._deadline(self.ack_timeout)
        window=bytearray()
        while True:
            window.extend(self.transport._read(1,'USB input ACK',self.transport._remaining(end,'USB input ACK')))
            if len(window)>40:del window[0]
            result=reply(bytes(window),self.session,sequence)
            if result is not None:
                if result['result']:raise ProtocolError('USB input rejected: '+ERRORS.get(result['result'],str(result['result'])))
                return result

    def open(self):
        previous=self.session
        self.session=secrets.randbelow(0xffffffff)+1
        if self.session==previous:self.session=self.session%0xffffffff+1
        self.sequence=0
        return self._exchange(packet(1,self.session,0),0)

    def send(self,kind,ports=None,ticks=0,mask=0):
        if self.sequence==0xffffffff:raise ValueError('sequence exhausted; start a fresh session')
        self.sequence+=1
        return self._exchange(packet(kind,self.session,self.sequence,ports,ticks,mask),self.sequence)

    def state(self,ports,ticks=120):return self.send(2,ports,ticks)
    def release(self,mask=3):return self.send(3,mask=mask)
    def status(self):return self.send(4)


def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True)
    parser.add_argument('--timeout',type=positive_seconds,default=30.0)
    parser.add_argument('--ack-timeout',type=positive_seconds,default=5.0)
    parser.add_argument('--pad1');parser.add_argument('--pad2')
    parser.add_argument('--mouse1',help='DX,DY[,LEFT+RIGHT+MIDDLE+START]; use --mouse1=-20,10,left for negatives')
    parser.add_argument('--mouse2')
    parser.add_argument('--frames',type=int,default=120,help='Guest VInt lease, 1..120; automatic expiry')
    parser.add_argument('--seconds',type=float,default=0,help='Renew for up to 300 seconds; then release; default is one pulse')
    parser.add_argument('--release',action='store_true',help='Open a fresh session and release both ports')
    args=parser.parse_args(argv)
    try:
        ports={}
        for p in (1,2):
            keys=getattr(args,'pad'+str(p));move=getattr(args,'mouse'+str(p))
            if keys is not None and move is not None:raise ValueError('choose pad or mouse per port')
            if keys is not None:ports[p]=pad(buttons(keys))
            if move is not None:
                values=move.split(',')
                if len(values) not in (2,3):raise ValueError('mouse needs DX,DY[,buttons]')
                ports[p]=mouse(int(values[0]),int(values[1]),buttons(values[2],MOUSE) if len(values)==3 else 0)
        if not math.isfinite(args.seconds) or not 0<=args.seconds<=300:raise ValueError('seconds must be finite and 0..300')
        if not 1<=args.frames<=120:raise ValueError('frames must be 1..120')
        if args.seconds and (args.frames<30 or args.timeout<args.seconds+4):raise ValueError('renewal needs >=30 frames and timeout >= seconds+4')
        if args.release and ports:raise ValueError('release cannot include injected inputs')
        if not args.release and not ports:raise ValueError('select at least one pad/mouse, or --release')
        with open_serial(args.port) as connection:
            transport=USBTransport(connection,args.timeout);identity=transport.identify();client=RemoteInput(transport,args.ack_timeout)
            client.open()
            if args.release:result=client.release()
            else:
                first=result=client.state(ports,args.frames)
                if args.seconds:
                    until=time.monotonic()+args.seconds
                    renewal={p:(kind,held,0,0) for p,(kind,held,dx,dy) in ports.items()}
                    try:
                        while time.monotonic()<until:
                            remaining=until-time.monotonic()
                            if remaining<=0:break
                            time.sleep(min(0.5,args.frames/120,remaining))
                            result=client.state(renewal,args.frames)
                    finally:
                        # No blind retry on loss; the guest lease releases within 120 ticks.
                        client.release(sum(1<<(p-1) for p in ports))
                result={'first':first,'last':result,'automatic_expiry_ticks':args.frames,
                        'renewed_seconds':args.seconds,'released_on_host':bool(args.seconds)}
            print(json.dumps(dict(identity,input=result),sort_keys=True))
        return 0
    except (OSError,ValueError,ProtocolError) as error:
        print('USB input: '+str(error),file=sys.stderr);return 1
    except KeyboardInterrupt:
        print('USB input: interrupted; guest overrides expire automatically',file=sys.stderr);return 130


if __name__=='__main__':raise SystemExit(main())
