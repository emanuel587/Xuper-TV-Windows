#!/usr/bin/env python3
"""Offline TLS integration tests. Uses only a temporary localhost test server."""
import base64
import datetime
import http.server
import json
import ssl
import subprocess
import sys
import tempfile
import hashlib
import threading
from pathlib import Path
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.ciphers import Cipher,modes
from cryptography.hazmat.primitives.padding import PKCS7
from cryptography.hazmat.decrepit.ciphers.algorithms import TripleDES

KEY=bytes(range(24))
def prepared_password(password):
    pad=PKCS7(128).padder();raw=pad.update(password.encode())+pad.finalize()
    from cryptography.hazmat.primitives.ciphers.algorithms import AES
    cipher=Cipher(AES(b'0123456789abcdef'),modes.ECB()).encryptor()
    encrypted=cipher.update(raw)+cipher.finalize()
    return hashlib.md5((password+'test-salt').encode()).hexdigest()+base64.b64encode(encrypted).decode()
def encrypt(value):
    pad=PKCS7(64).padder();raw=pad.update(json.dumps(value).encode())+pad.finalize()
    cipher=Cipher(TripleDES(KEY),modes.ECB()).encryptor()
    return base64.b64encode(cipher.update(raw)+cipher.finalize()).hex()
def decrypt(value):
    raw=base64.b64decode(bytes.fromhex(value).decode());cipher=Cipher(TripleDES(KEY),modes.ECB()).decryptor()
    raw=cipher.update(raw)+cipher.finalize();pad=PKCS7(64).unpadder()
    return json.loads(pad.update(raw)+pad.finalize())

class Handler(http.server.BaseHTTPRequestHandler):
    mode='success';posts=0;gets=0;failures=[]
    serials=[];open_counts=[]
    def log_message(self,*args):pass
    def do_POST(self):
        type(self).posts+=1
        try:
            body=self.rfile.read(int(self.headers['Content-Length'])).decode()
            request=decrypt(body)
            assert self.headers['apk']=='test.app'
            if self.mode.startswith('bootstrap'):
                assert 'password' not in request and 'userName' not in request
                assert request['apkVersion']=='43404' and self.headers['apkVer']=='43404'
                if self.path=='/api/portalCore/v3/snToken':
                    type(self).serials.append(request['serialNumber'])
                    if self.mode=='bootstrap_version':response={'returnCode':'portal200001'}
                    elif self.mode=='bootstrap_existing':response={'returnCode':'0','data':encrypt({'snToken':'not-reused','isNew':'1','sn':'server-existing-sn'})}
                    else:response={'returnCode':'0','data':encrypt({'snToken':'device-test-token','isNew':'0'})}
                elif self.path=='/api/portalCore/v8/active':
                    if self.mode=='bootstrap_existing':
                        assert request['snToken']=='' and request['sn']=='server-existing-sn'
                    else:
                        assert request['snToken']=='device-test-token'
                        assert request['sn']==hashlib.md5(b'device-test-tokentest-salt').hexdigest()
                    assert request['authCode']=='' and request['authVersion']==''
                    type(self).open_counts.append(request['openNum'])
                    if self.mode=='bootstrap_denied':response={'returnCode':'permission-denied'}
                    elif self.mode=='bootstrap_incomplete':response={'returnCode':'0','data':encrypt({})}
                    elif (self.mode.startswith('bootstrap_catalog') or self.mode=='bootstrap_vod'):
                        response={'returnCode':'0','data':encrypt({'userId':'test-user','userToken':'test-session','portalCodeList':[{'portalCode':'test-portal'}]})}
                    else:response={'returnCode':'0','data':encrypt({'userId':'test-user','userToken':'test-session',
                        'portalCodeList':[{'portalCode':'test-portal'}],
                        'playlistUrl':f'https://localhost:{self.server.server_port}/playlist.m3u'})}
                else:
                    assert (self.mode.startswith('bootstrap_catalog') or self.mode=='bootstrap_vod')
                    assert request['userId']=='test-user' and request['userToken']=='test-session'
                    assert request['portalCode']=='test-portal'
                    if self.path=='/api/portalCore/v15/getSlbInfo':
                        response={'returnCode':'0','data':encrypt({'error_code':0,'cdn_list':[]})}
                    elif self.mode=='bootstrap_vod' and self.path not in ['/api/portalCore/v3/getColumnContents','/api/portalCore/v6/getLiveData']:
                        if self.path=='/api/portalCore/v3/getShelveData':
                            assert request['pageSize']==24 and request['numDisplay']==24
                            assert request['columnType']=='2' and request['encryptVersion']==0
                            assert all(x not in request for x in ['originalCountry','tags','year','audio'])
                            if request['columnId']==300:
                                assert request['pageNum'] in [1,2]
                                data={'assetList':[{'contentId':'movie-test','name':'Pelicula de prueba','posterList':[{'fileUrl':'https://images.example.invalid/movie.jpg'}],'description':'Ficha sintetica','type':'1','programType':'Largometraje'}],
                                    'assetListTotalSize':25}
                            else:
                                assert request['columnId']==120 and request['pageNum']==1
                                data={'assetList':[{'contentId':'series-test','name':'Serie de prueba','type':'0','programType':'Drama'}],'assetListTotalSize':1}
                            response={'returnCode':'0','data':encrypt(data)}
                        elif self.path=='/api/portalCore/v4/getItemData':
                            assert request['sortType']=='0' and request['macAddr']=='02:00:00:00:00:01'
                            if request['type']=='1':
                                assert request['contentId']=='movie-test'
                                asset={'contentId':'movie-test','name':'Pelicula','simpleProgramList':[{'contentId':'movie-video','name':'Completa'}]}
                            else:
                                assert request['type']=='0' and request['contentId']=='series-test'
                                asset={'contentId':'series-test','name':'Serie','simpleProgramList':[
                                    {'contentId':'episode-one','seriesNumber':1,'name':'Inicio'},
                                    {'contentId':'episode-two','seriesNumber':2,'name':'Continuacion'}]}
                            response={'returnCode':'0','data':encrypt({'assetData':asset})}
                        elif self.path=='/api/portalCore/v10/startPlayVOD':
                            assert request['columnId']==-1 and request['type']=='1' and request['startTime']==0
                            assert request['authType'] is None and request['episodeNumberList'] is None
                            if request['contentId']=='movie-video':
                                assert request['seriesContentId'] is None
                                response={'returnCode':'0','data':encrypt({'episodeList':[{'programContentId':'movie-video',
                                    'totalMovieList':[{'movieList':[{'contentId':'opaque-movie','licenseList':[{'license':'synthetic'}]}]}]}]})}
                            else:
                                assert request['contentId']=='episode-one' and request['seriesContentId']=='series-test'
                                response={'returnCode':'permission-denied'}
                        elif self.path=='/api/portalCore/v3/searchByName':
                            assert request['value']=='Prueba ñ' and request['columnId']=='-1' and request['type']=='1'
                            assert request['pageSize']==100 and request['filter']==''
                            assert request['pageNum'] in [1,2]
                            groups=[{'programType':'Cine','itemList':[{'contentId':'movie-test','name':'Prueba','type':'1','programType':'Drama'}]}]
                            if request['pageNum']==1:groups.append({'programType':'TV','itemList':[{'contentId':'series-test','name':'Serie Prueba','type':'0','programType':'Comedia'}]})
                            response={'returnCode':'0','data':encrypt({'searchItemList':groups,'totalSize':101})}
                        else:raise AssertionError('unexpected VOD route')
                    elif self.path=='/api/portalCore/v3/getColumnContents':
                        assert request['pageNum']==1 and request['pageSize']==1000
                        assert request['numDisplay'] is None and request['isAv1']=='0'
                        if request['columnId'] is None:
                            assert request['specialFlag'] is None
                            data={'childColumnList':[{'id':99,'name':'VOD','code':'test-portal_vod'},
                                {'id':100,'name':'TV','code':'test-portal_live'}]}
                            if self.mode=='bootstrap_vod':data['childColumnList'] += [{'id':110,'code':'test-portal_movies','name':'Movies'},{'id':120,'code':'test-portal_series','name':'Series'}]
                            if self.mode=='bootstrap_catalog_missing':data={'childColumnList':[]}
                        elif self.mode=='bootstrap_vod' and request['columnId'] in [110,120]:
                            assert request['specialFlag']=='0'
                            data={'childColumnList':[{'id':300,'name':'Estrenos'}] if request['columnId']==110 else []}
                        else:
                            assert request['columnId']==100 and request['specialFlag']=='0'
                            data={'childColumnList':[{'id':200,'name':'Noticias'},
                                {'id':201,'name':'Protegida','restricted':'1'},
                                {'id':'bad-id','name':'Invalid category'}]}
                        response={'returnCode':'0','data':encrypt(data)}
                    elif self.path=='/api/portalCore/v6/getLiveData':
                        assert request['columnId']==200 and request['pageNum']==1 and request['pageSize']==3000
                        assert request['dataVersion']=='' and request['expireTimeStr']==''
                        data={'channelList':[{'channelCode':'channel-test','name':'Canal de Prueba ñ',
                            'posterList':[{'fileUrl':'https://images.example.invalid/channel.png'}],'liveAddressList':[{'playCode':'https://must-not-use.example.invalid'}]},
                            {'channelCode':'locked','name':'Locked','isLock':True}]}
                        response={'returnCode':'0','data':encrypt(data)}
                    elif self.path=='/api/portalCore/v4/startPlayLive':
                        assert request['type']=='1' and request['columnId'] is None and request['channelCode']=='channel-test'
                        url='ranger:opaque-id' if self.mode=='bootstrap_catalog_opaque' else 'https://media.example.invalid/live.m3u8?sig=a%2Fb&expires=123'
                        data={'liveAddressList':[{'playCode':url,'license':'test-license' if self.mode=='bootstrap_catalog_license' else ''}]}
                        response={'returnCode':'permission-denied'} if self.mode=='bootstrap_catalog_denied' else {'returnCode':'0','data':encrypt(data)}
                    else:raise AssertionError('unexpected catalog route')
                raw=json.dumps(response).encode();self.send_response(200);self.end_headers();self.wfile.write(raw);return
            assert self.path=='/api/portalCore/v8/login'
            assert request['userName']=='demo@example.invalid'
            assert request['password']==prepared_password('test-password')
            assert request['password']!='test-password'
            assert request['accountType']=='2'
            assert request['model']=='Nintendo Switch'
            if self.mode=='denied':response={'returnCode':'denied','errorMessage':'test rejection'}
            elif self.mode=='malformed':response={'returnCode':'0','data':'not-a-ciphertext'}
            else:response={'returnCode':'0','data':encrypt({'playlistUrl':f'https://localhost:{self.server.server_port}/playlist.m3u'})}
            raw=json.dumps(response).encode();self.send_response(200);self.end_headers();self.wfile.write(raw)
        except Exception as exc:
            type(self).failures.append(type(exc).__name__);self.send_error(500)
    def do_GET(self):
        type(self).gets+=1
        if self.path!='/playlist.m3u':self.send_error(404);return
        self.send_response(200);self.end_headers()
        self.wfile.write(b'#EXTM3U\n#EXTINF:-1,Offline test channel\nhttps://example.invalid/video.m3u8\n')

def main(binary,app=None):
    with tempfile.TemporaryDirectory() as tmp:
        folder=Path(tmp);key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
        name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'localhost')]);now=datetime.datetime.now(datetime.timezone.utc)
        cert=(x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key())
              .serial_number(x509.random_serial_number()).not_valid_before(now-datetime.timedelta(minutes=1))
              .not_valid_after(now+datetime.timedelta(hours=1))
              .add_extension(x509.SubjectAlternativeName([x509.DNSName('localhost')]),False)
              .add_extension(x509.BasicConstraints(ca=True,path_length=None),True).sign(key,hashes.SHA256()))
        ca=folder/'ca.pem';secret=folder/'key.pem';ca.write_bytes(cert.public_bytes(serialization.Encoding.PEM))
        secret.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
        server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
        ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(str(ca),str(secret));server.socket=ctx.wrap_socket(server.socket,server_side=True)
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        host=f'https://localhost:{server.server_port}'
        config={'portal_hosts':[host,host],'key_base64':base64.b64encode(KEY).decode(),
            'app_id':'test.app','apk_version':'43500','device_id':'02:00:00:00:00:01','ca_file':str(ca),
            'password_salt':'test-salt','password_aes_key':'0123456789abcdef','installation_serial':'stable-test-install'}
        path=folder/'config.json';path.write_text(json.dumps(config))
        try:
            for mode in ['success','denied','malformed']:
                Handler.mode=mode;Handler.posts=0;Handler.gets=0;Handler.failures=[]
                subprocess.run([str(Path(binary).resolve()),str(path),mode],check=True,timeout=15)
                assert not Handler.failures,Handler.failures
                assert Handler.posts==1,(mode,Handler.posts)
                assert Handler.gets==(1 if mode=='success' else 0),(mode,Handler.gets)
                print('local TLS test:',mode,'passed')
            config['apk_version']='43404';path.write_text(json.dumps(config))
            for mode in ['bootstrap_success','bootstrap_success','bootstrap_version','bootstrap_denied','bootstrap_incomplete','bootstrap_existing']:
                Handler.mode=mode;Handler.posts=0;Handler.gets=0;Handler.failures=[]
                subprocess.run([str(Path(binary).resolve()),str(path),mode],check=True,timeout=15)
                assert not Handler.failures,Handler.failures
                assert Handler.posts==(1 if mode=='bootstrap_version' else 2),(mode,Handler.posts)
                assert Handler.gets==(1 if mode in ['bootstrap_success','bootstrap_existing'] else 0),(mode,Handler.gets)
                print('local TLS test:',mode,'passed')
            assert Handler.open_counts==[0,1,2,3,4],Handler.open_counts
            for mode in ['bootstrap_catalog','bootstrap_catalog_missing','bootstrap_catalog_denied','bootstrap_catalog_opaque','bootstrap_catalog_license']:
                Handler.mode=mode;Handler.posts=0;Handler.gets=0;Handler.failures=[]
                subprocess.run([str(Path(binary).resolve()),str(path),mode],check=True,timeout=15)
                assert not Handler.failures,Handler.failures
                assert Handler.posts==(3 if mode=='bootstrap_catalog_missing' else 8 if mode=='bootstrap_catalog' else 7 if mode=='bootstrap_catalog_license' else 6),(mode,Handler.posts)
                assert Handler.gets==0,(mode,Handler.gets)
                state=json.loads((folder/'device_state.json').read_text())
                assert set(state)=={'sn','open_count'},state.keys()
                print('local TLS test:',mode,'passed')
            Handler.mode='bootstrap_vod';Handler.posts=0;Handler.gets=0;Handler.failures=[]
            subprocess.run([str(Path(binary).resolve()),str(path),'bootstrap_vod'],check=True,timeout=15)
            assert not Handler.failures,Handler.failures
            assert Handler.posts==18,Handler.posts
            assert Handler.gets==0
            print('local TLS test: bootstrap_vod passed (movies, series, episodes, search, pagination, denied source)')
            assert len(set(Handler.serials))==1
            if app:
                import os
                for filename,source in [('font.ttf','work/runtime/switch/xuperswitch/font.ttf')]:
                    (folder/filename).write_bytes(Path(source).read_bytes())
                (folder/'cacert.pem').write_bytes(ca.read_bytes())
                (folder/'service.json').write_bytes(path.read_bytes())
                Handler.posts=0;Handler.gets=0;Handler.failures=[]
                subprocess.run([str(Path(app).resolve())],cwd=folder,env=dict(os.environ,SDL_VIDEODRIVER='dummy'),check=True,timeout=60)
                assert Handler.posts==6 and Handler.gets==0 and not Handler.failures,(Handler.posts,Handler.gets,Handler.failures)

        finally:server.shutdown();server.server_close();thread.join()

if __name__=='__main__':main(*sys.argv[1:])
