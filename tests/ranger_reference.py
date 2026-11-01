"""Independent Python reference; matched to original ARM64 vectors before use."""
import struct,math,json
mask=0xffffffff
rot=lambda x,n:((x<<n)|(x>>(32-n)))&mask
T=[int(abs(math.sin(i+1))*2**32)&mask for i in range(64)]
T[42]=0xd46f3085;T[45]=0xe6bd99e5;T[62]=0x2da7d2bb;T[54]=0xffecc47d
shifts=[7,12,17,22]*4+[5,9,14,20]*4+[4,11,16,23]*4+[6,10,15,21]*4
def md5variant(data,offset):
 n=len(data);data+=b'\x80';data+=b'\0'*((56-len(data))%64)+struct.pack('<Q',n*8)
 state=[0x67452301,0xefcdab89,0x98badcfe,0x10325476]
 for start in range(0,len(data),64):
  words=struct.unpack('<16I',data[start:start+64]);a,b,c,d=state
  for i in range(64):
   if i<16:f=(b&c)|(~b&d);g=i
   elif i<32:f=(d&b)|(~d&c);g=(5*i+1)%16
   elif i<48:f=b^c^d;g=(3*i+5)%16
   else:f=c^(b|~d);g=(7*i)%16
   a,d,c,b=d,c,b,(b+rot((a+f+T[i]+words[([10,11,12,13,14,15,6,7,8,9,0,1,2,3,4,5][g] if offset==10 and i<16 else g)])&mask,shifts[i]))&mask
  state=[(v+w)&mask for v,w in zip(state,[a,b,c,d])]
 return struct.pack('<4I',*state).hex()

import struct,json
M=(1<<64)-1
rol=lambda x,n:((x<<n)|(x>>(64-n)))&M
def fmix(k):
 k^=k>>33;k=k*0xff51afd7ed558ccd&M;k^=k>>33;k=k*0xc4ceb9fe1a85ec53&M;return k^(k>>33)
def murmur(data,seed):
 h1=h2=seed;c1=0x87c37b91114253d5;c2=0x4cf5ad432745937f
 for pos in range(0,len(data)//16*16,16):
  k1,k2=struct.unpack('<QQ',data[pos:pos+16]);k1=k1*c1&M;k1=rol(k1,31);k1=k1*c2&M;h1^=k1
  h1=rol(h1,27);h1=(h1+h2)&M;h1=(h1*5+0x52dce729)&M
  k2=k2*c2&M;k2=rol(k2,33);k2=k2*c1&M;h2^=k2;h2=rol(h2,31);h2=(h2+h1)&M;h2=(h2*5+0x38495ab5)&M
 tail=data[len(data)//16*16:];k1=int.from_bytes(tail[:8],'little');k2=int.from_bytes(tail[8:],'little')
 if len(tail)>8:k2=k2*c2&M;k2=rol(k2,33);k2=k2*c1&M;h2^=k2
 if tail:k1=k1*c1&M;k1=rol(k1,31);k1=k1*c2&M;h1^=k1
 h1^=len(data);h2^=len(data);h1=(h1+h2)&M;h2=(h2+h1)&M;h1=fmix(h1);h2=fmix(h2);h1=(h1+h2)&M;h2=(h2+h1)&M
 return struct.pack('<QQ',h1,h2)

import base64,hashlib
from cryptography.hazmat.primitives.ciphers import Cipher,algorithms,modes
ALPHABET='jWB7YtC3n9iXbEkUcJl1VxF4STpQoOIaRmh2M-efAgLwPqGr6uyD5vNsdH_Kz0Z8'
STANDARD='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
def material(seed,mode,response):
    salt='93DDB237F6944625B0A1914C5ED4A15F' if mode=='live' else '9B4F4CE2168E11EF8DCE000C297524BB'
    key=md5variant((seed+salt).encode(),10)
    route='/slb/v9/live' if mode=='live' else '/slb/v11/vod'
    resource=('resp:' if response else 'req:')+route
    iv=murmur(('http://encrypt.io?scheme=aes-cbc-128-o2&key='+key+'&resource='+resource).encode(),1597135542)
    return bytes.fromhex(key),iv
def wire_encode(seed,mode,value,response):
    key,iv=material(seed,mode,response);raw=value.encode();padding=16-len(raw)%16
    cipher=Cipher(algorithms.AES(key),modes.CBC(iv)).encryptor();data=cipher.update(raw+bytes([padding])*padding)+cipher.finalize()
    return base64.b64encode(data).decode().translate(str.maketrans(STANDARD,ALPHABET)).rstrip('=')
def wire_decode(seed,mode,value,response):
    encoded=value.translate(str.maketrans(ALPHABET,STANDARD));encoded+='='*((-len(encoded))%4)
    data=base64.b64decode(encoded);key,iv=material(seed,mode,response)
    cipher=Cipher(algorithms.AES(key),modes.CBC(iv)).decryptor();raw=cipher.update(data)+cipher.finalize()
    padding=raw[-1];assert 1<=padding<=16 and raw[-padding:]==bytes([padding])*padding
    return raw[:-padding].decode()


def decode_request_id(value):
    """Native 0x674b20 layout, verified separately from C++ using ARM64 fixtures."""
    assert len(value)==34
    prefix=int(value[:2],16);assert 0x40<=prefix<=0x7f
    base='B8oOvNYtn9RPijWXbcJqGyD5Eklmh21V_efAgLC3wxF4STpQr6usdHM-Kz0IaUZ7'
    n=prefix&63;alphabet=base[n:]+base[:n]
    encoded=value[2:].translate(str.maketrans(alphabet,STANDARD))
    check,user,device,seconds,nonce=struct.unpack('>IIQII',base64.b64decode(encoded,validate=True))
    assert nonce&63==n
    source=('%02x%08x%016x%08x%08x'%(prefix,user,device,seconds,nonce)).encode()
    h=42
    for i in range(0,len(source)//4*4,4):
        k=int.from_bytes(source[i:i+4],'little');k=rot((k*0xcc9e2d51)&mask,15)*0x1b873593&mask
        h=rot(h^k,13);h=(h*5+0xe6546b64)&mask
    k=int.from_bytes(source[len(source)//4*4:],'little')
    h^=rot((k*0xcc9e2d51)&mask,15)*0x1b873593&mask;h^=len(source)
    h^=h>>16;h=h*0x85ebca6b&mask;h^=h>>13;h=h*0xc2b2ae35&mask;h^=h>>16
    assert check==h
    return user,device,seconds,nonce


def live_content_auth(resource,instance,moment_ms):
    if 'token=' not in resource or resource.endswith('token='):return resource
    extra=f'&sign2_method=sign_o3&instance={instance}&start_moment={moment_ms}'
    salt=bytes.fromhex('73616c74333333333d34980d0a1532c9c3821708c0')
    digest=md5variant(resource[resource.index('token='):].encode()+extra.encode()+salt,10)
    return resource+extra+'&sign2='+digest
