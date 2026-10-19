"""Portable guest contracts, host framing, and independently installed resources."""
import binascii
import importlib.util
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
SDK='sgdk' if (ROOT/'skills/sgdk-testing').is_dir() else 'megadev'
SKILL=ROOT/'skills'/f'{SDK}-testing'
CORE=SKILL/'assets/usb-input'
sys.path.insert(0,str(SKILL/'scripts'))
# Unique module name avoids collisions with other skills' transport tests.
spec=importlib.util.spec_from_file_location('remote_usb_input',SKILL/'scripts/usb_input.py')
remote=importlib.util.module_from_spec(spec);spec.loader.exec_module(remote)


def vectors():
    session=0x12345678
    ports={1:remote.pad(0x50),2:remote.mouse(20,-10,5)}
    state=remote.packet(2,session,1,ports,3)
    corrupt=bytearray(state);corrupt[-1]^=1
    invalid=bytearray(remote.packet(2,session,2,ports,30));invalid[24]=3
    invalid[38:]=struct.pack('>H',binascii.crc_hqx(invalid[:38],0xffff))
    legacy=b'HLV1'+struct.pack('>HI',1,123)
    legacy+=struct.pack('>H',binascii.crc_hqx(legacy,0xffff))
    return {'open_session':remote.packet(1,session,0),'two_ports':state,'corrupt':bytes(corrupt),
            'invalid_second':bytes(invalid),'mouse_one':remote.packet(2,session,2,{1:remote.mouse(12,8,1)},120),
            'release_one':remote.packet(3,session,3,mask=1),'wrong_session':remote.packet(4,session+1,4),
            'status_request':remote.packet(4,session,4),'legacy_link':legacy,
            'long_both':remote.packet(2,session,5,ports,120),'release_first':remote.packet(3,session,6,mask=1),
            'only_first_pad':remote.packet(2,session,7,{1:remote.pad(0x10)},30),
            'release_second':remote.packet(3,session,8,mask=2),
            'mouse_start':remote.packet(2,session,9,{2:remote.mouse(0,0,15)},120),
            'sequence_max':remote.packet(4,session,0xffffffff)}


class UsbInputTests(unittest.TestCase):
    def test_guest_compiled_contracts_o0_o3(self):
        cc=next((p for name in ['clang','cc','gcc'] if (p:=shutil.which(name))),None)
        if not cc:self.skipTest('C compiler unavailable')
        with tempfile.TemporaryDirectory(prefix='USB input ') as folder:
            folder=Path(folder)
            (folder/'vectors.h').write_text('\n'.join('static const unsigned char '+name+'[]={'+','.join(str(b) for b in value)+'};' for name,value in vectors().items()))
            for opt in ['-O0','-O3']:
                binary=folder/('test'+opt+('.exe' if os.name=='nt' else ''))
                args=[cc,'-std=c17',opt,'-Wall','-Wextra','-Werror','-I',str(CORE),'-I',str(folder),
                      str(CORE/'usb_input.c'),str(ROOT/'tests/fixtures/usb_input.c'),'-o',str(binary)]
                if os.name!='nt':args+=['-fsanitize=undefined','-fno-sanitize-recover=all']
                r=subprocess.run(args,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)
                r=subprocess.run([str(binary)],capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr)

    def test_lab_snapshot_matches_installed_reusable_module(self):
        lab=ROOT/'skills'/f'{SDK}-hardware/assets/hardware-lab'
        for name in ['usb_input.c','usb_input.h','usb_input_ed.h']:
            where='inc' if SDK=='sgdk' and name.endswith('.h') else 'src'
            self.assertEqual((CORE/name).read_bytes(),(lab/where/name).read_bytes())

    def test_independently_copied_installer_and_no_overwrite(self):
        with tempfile.TemporaryDirectory(prefix='installed input ') as folder:
            folder=Path(folder);installed=folder/'skill'
            shutil.copytree(SKILL,installed,ignore=shutil.ignore_patterns('__pycache__'))
            project=folder/'my game';project.mkdir()
            command=[sys.executable,str(installed/'scripts/install_usb_input.py'),str(project),'--headers','src' if SDK=='megadev' else 'inc']
            r=subprocess.run(command,capture_output=True,text=True,cwd=folder);self.assertEqual(r.returncode,0,r.stderr)
            source=project/'src/usb_input.c';original=source.read_bytes();source.write_bytes(original+b'/* user */')
            r=subprocess.run(command,capture_output=True,text=True,cwd=folder);self.assertNotEqual(r.returncode,0)
            self.assertEqual(source.read_bytes(),original+b'/* user */')
            r=subprocess.run([sys.executable,str(installed/'scripts/usb_input.py'),'--help'],capture_output=True,text=True,cwd=folder)
            self.assertEqual(r.returncode,0,r.stderr)

    def test_button_mapping_and_wire_golden_fields(self):
        self.assertEqual(remote.buttons('A+B+X+MODE'),0xc50)
        self.assertEqual(remote.buttons('left+middle',remote.MOUSE),5)
        self.assertEqual(remote.buttons('START',remote.MOUSE),8)
        data=vectors()['two_ports']
        self.assertEqual(data[:16],bytes.fromhex('55495631020300031234567800000001'))
        self.assertEqual(data[16:32],bytes.fromhex('0100005000000000020000050014fff6'))
        self.assertEqual(int.from_bytes(data[38:],'big'),binascii.crc_hqx(data[:38],0xffff))

    def test_invalid_cli_never_opens_a_port(self):
        cases=[['--pad1','INVALID'],['--pad1','A','--mouse1','0,0'],['--mouse2','2048,0'],
               ['--pad1','A','--frames','0'],['--pad1','A','--seconds','nan'],['--release','--pad2','B']]
        import contextlib,io
        for args in cases:
            with self.subTest(args=args),patch.object(remote,'open_serial') as opening,contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(remote.main(['--port','unused',*args]),1)
                opening.assert_not_called()

    def test_fragmented_transport_identity_and_filtered_rpc(self):
        from test_everdrive import TranscriptPort, identity, legacy_identity, memory
        def ack(session,seq,result=0):
            data=bytearray(40)
            struct.pack_into('>4sBBHII',data,0,b'UIV1',128,0,0,session,seq)
            data[36]=result;data[37]=3
            data[38:]=struct.pack('>H',binascii.crc_hqx(data[:38],0xffff))
            return bytes(data)
        for discovery in (identity(),legacy_identity()):
            bad=bytearray(ack(123,1));bad[-1]^=1
            steps=discovery+[
                (memory(0x1810000,remote.packet(1,123,0)),b'noise'+ack(124,0)+ack(123,0)),
                (memory(0x1810000,remote.packet(2,123,1,{2:remote.mouse(-7,9,8)},60)),bytes(bad)+ack(123,1)),
                (memory(0x1810000,remote.packet(3,123,2,mask=2)),ack(123,2,4)),
                (memory(0x1810000,remote.packet(1,124,0)),ack(124,0))]
            port=TranscriptPort(steps,read_fragment=1,write_fragment=3)
            transport=remote.USBTransport(port,30);transport.identify()
            client=remote.RemoteInput(transport)
            with patch.object(remote.secrets,'randbelow',return_value=122):
                self.assertEqual(client.open()['session'],123)
                self.assertEqual(client.state({2:remote.mouse(-7,9,8)},60)['sequence'],1)
                with self.assertRaisesRegex(remote.ProtocolError,'invalid arguments'):client.release(2)
                # A repeated random candidate must still open a fresh nonce.
                self.assertEqual(client.open()['session'],124)
            self.assertFalse(port.steps);self.assertFalse(port.responses)
        for discovery in (identity(device=0x25),legacy_identity(device=0x25)):
            port=TranscriptPort(discovery)
            with self.assertRaisesRegex(remote.ProtocolError,'expected Mega EverDrive Pro'):
                remote.USBTransport(port).identify()
            self.assertNotIn(bytes.fromhex('2bd41ae5'),port.received)

    def test_crc_nonce_sequence_and_error_ack(self):
        data=bytearray(40);struct.pack_into('>4sBBHII',data,0,b'UIV1',128,1,30,123,4)
        data[36]=3;data[37]=3;data[38:]=struct.pack('>H',binascii.crc_hqx(data[:38],0xffff))
        self.assertEqual(remote.reply(data,123,4)['result'],3)
        self.assertIsNone(remote.reply(data,124,4));self.assertIsNone(remote.reply(data,123,5))
        data[-1]^=1;self.assertIsNone(remote.reply(data,123,4))


if __name__=='__main__':unittest.main()
