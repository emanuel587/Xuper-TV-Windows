#!/usr/bin/env python3
"""Extract this APK's portal configuration locally; never contacts its servers."""
import argparse
import json
import re
import uuid
from pathlib import Path
from loguru import logger
logger.remove()
from androguard.core.apk import APK
from androguard.core.dex import DEX
from cryptography.hazmat.primitives.ciphers import Cipher,modes
from cryptography.hazmat.primitives.padding import PKCS7
try:
    from cryptography.hazmat.decrepit.ciphers.algorithms import TripleDES
except ImportError:
    from cryptography.hazmat.primitives.ciphers.algorithms import TripleDES

def android_base64(value):
    alphabet='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
    value=value.replace('\r','').replace('\n','');out=bytearray()
    for offset in range(0,len(value),4):
        block=value[offset:offset+4]
        if len(block)<2:break
        v=[alphabet.find(c) for c in block]+[-1]*4
        out.append(((v[0]<<2)&252)|((v[1]>>4)&3))
        if len(block)>2 and block[2]!='=':out.append(((v[1]<<4)&240)|((v[2]>>2)&15))
        if len(block)>3 and block[3]!='=':out.append(((v[2]<<6)&192)|(v[3]&63))
    return bytes(out)

def decrypt(value,key):
    cipher=android_base64(bytes.fromhex(value).decode('ascii'))
    decoded=android_base64(key)
    if len(decoded)<24:raise ValueError('Invalid extracted key')
    dec=Cipher(TripleDES(decoded[:24]),modes.ECB()).decryptor()
    padded=dec.update(cipher)+dec.finalize();unpad=PKCS7(64).unpadder()
    return (unpad.update(padded)+unpad.finalize()).decode('utf-8')

def configure(apk_path,output):
    apk=APK(str(apk_path));root=None;app_id=None;salt=None;aes_key=None;api_version=None
    for raw in apk.get_all_dex():
        dex=DEX(raw)
        for cls in dex.get_classes():
            wanted={'Lk2/b;':{'g','i'},'Lj7/a;':{'i'},'Lk2/a;':{'b'},'Lo7/a;':{'i'}}.get(cls.get_name())
            if wanted is None:continue
            for method in cls.get_methods():
                if method.get_name() not in wanted or not method.get_code():continue
                if cls.get_name()=='Lo7/a;':
                    values=[v for i in method.get_instructions() if i.get_name().startswith('const') and hasattr(i,'get_literals') for v in i.get_literals()]
                    api_version=next((v for v in values if isinstance(v,int) and v>10000),None)
                    continue
                strings=[i.get_string() for i in method.get_instructions() if i.get_name().startswith('const-string')]
                if cls.get_name()=='Lk2/b;' and method.get_name()=='g':root=next((s for s in strings if len(s)>24),None)
                elif cls.get_name()=='Lk2/b;':salt=next((s for s in strings if len(s)>24),None)
                elif cls.get_name()=='Lk2/a;':aes_key=next((s for s in strings if len(s)==16),None)
                else:app_id=next((s for s in strings if s.startswith('com.')),None)
    if not root or not app_id or not salt or not aes_key or not api_version:raise ValueError('Unsupported APK: expected configuration code was not found')
    ns='{http://schemas.android.com/apk/res/android}'
    meta=next((x.get(ns+'value') for x in apk.get_android_manifest_xml().findall('.//meta-data')
               if x.get(ns+'name')=='PORTAL_KEY'),None)
    if not meta:raise ValueError('Missing PORTAL_KEY')
    resources=apk.get_android_resources();hosts=[]
    for name in ['portal_main','portal_backup']:
        value=resources.get_string(apk.get_package(),name)
        if not value:raise ValueError('Missing '+name)
        host=decrypt(value[1],root)
        if not re.fullmatch(r'[A-Za-z0-9.-]+(?::[0-9]+)?',host):raise ValueError('Unrecognized portal hostname')
        hosts.append(host)
    device=bytearray(uuid.uuid4().bytes[:6]);device[0]=2
    device_id=':'.join(f'{v:02x}' for v in device)
    installation_serial=uuid.uuid4().hex
    if output.exists():
        try:
            old=json.loads(output.read_text());device_id=old.get('device_id',device_id)
            installation_serial=old.get('installation_serial',installation_serial)
        except (ValueError,OSError):pass
    config={'portal_hosts':hosts,'key_base64':decrypt(meta,root),'app_id':app_id,
        'apk_version':str(api_version),'manifest_version':apk.get_androidversion_code(),
        'installation_serial':installation_serial,'account_type':'auto','login_type':'2',
        'password_salt':salt,'password_aes_key':aes_key,
        'language':'es','device_id':device_id,'ca_file':'sdmc:/switch/xuperswitch/cacert.pem'}
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(config,indent=2)+'\n',encoding='utf-8')
    output.chmod(0o600)
    print('Configuracion generada. No contiene usuario ni contrasena.')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk',type=Path);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();configure(args.apk,args.output)
