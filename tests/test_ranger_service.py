#!/usr/bin/env python3
"""Local HTTPS integration, no vendor requests or user credentials."""
import base64,datetime,time,gzip,hashlib,http.server,json,ssl,subprocess,sys,tempfile,threading
from pathlib import Path
from urllib.parse import urlsplit,parse_qs
from cryptography import x509
from cryptography.x509.oid import NameOID
from cryptography.hazmat.primitives import hashes,serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from test_service import encrypt,decrypt,KEY
from ranger_reference import wire_encode,wire_decode,decode_request_id,live_content_auth
AUTH='app_id=test.app&tag=alpha&expired=4102444800&token=synthetic-cdn-auth'
LICENSE='app_id=test.app&tag=alpha&media_code=source-live&expired=4102444800&token=synthetic-license'
SOURCE='https://media.example.invalid/watch.m3u8?sig=a%2Fb&expires=123'
SIGNED_SOURCE='https://media.example.invalid/live/%2Fcam?tag=alpha&token=a%2Fb%3D%3D&expires=1793550000'
SUBTITLE='1\n00:00:00,000 --> 00:00:10,000\nTexto de prueba\n'
SEED=base64.b64encode(KEY).decode()
class Handler(http.server.BaseHTTPRequestHandler):
    mode='';posts=0;gets=0;failures=[]
    def log_message(self,*args):pass
    def respond(self,value,status=200,location=None,encoding=None):
        self.send_response(status)
        if location:self.send_header('Location',location)
        if encoding:self.send_header('Content-Encoding',encoding)
        self.end_headers();self.wfile.write(value if isinstance(value,bytes) else value.encode() if isinstance(value,str) else json.dumps(value).encode())
    def do_POST(self):
        type(self).posts+=1
        try:
            assert isinstance(self.connection,ssl.SSLSocket), 'portal POST must use TLS'
            p=decrypt(self.rfile.read(int(self.headers['Content-Length'])).decode())
            assert self.headers['apk']=='test.app' and p['apkVersion']=='43404'
            assert 'password' not in p and 'userName' not in p
            if self.path.endswith('/snToken'):data={'snToken':'synthetic-sn-token','isNew':'0'}
            elif self.path.endswith('/active'):
                if self.mode=='bootstrap_ranger_region':self.respond({'returnCode':'portal100024'});return
                data={'userId':'123456','userToken':'synthetic-session','portalCodeList':[{'portalCode':'test-portal'}],'hasPay':'1','userIdentity':'test-identity'}
            else:
                assert p['userId']=='123456' and p['userToken']=='synthetic-session' and p['portalCode']=='test-portal'
                if self.path.endswith('/getColumnContents'):
                    data={'childColumnList':[{'id':100,'name':'TV','code':'test-portal_live'}] if p['columnId'] is None else [{'id':200,'name':'Test category'}]}
                elif self.path.endswith('/startPlayLive'):
                    assert p['channelCode']=='channel-test' and p['columnId'] is None
                    if self.mode=='bootstrap_ranger_play_denied':self.respond({'returnCode':'permission-denied'});return
                    data={'liveAddressList':[{'playCode':'source-live','license':LICENSE,'tag':'alpha','cdnType':'1','AVFormat':'mp4' if self.mode=='bootstrap_ranger_live_mp4' else 'ts'}]}
                elif self.path.endswith('/startPlayVOD'):
                    assert p['contentId']=='movie-video' and p['seriesContentId'] is None
                    movies=[{'contentId':'movie-source','quality':'720p','licenseList':[{'license':LICENSE,'tag':'alpha'}]}]
                    if self.mode=='bootstrap_ranger_vod_quality':movies.append({'contentId':'movie-source-hd','quality':'1080p','audioInfo':'English','licenseList':[{'license':LICENSE,'tag':'alpha'}]})
                    data={'episodeList':[{'programContentId':'movie-video','totalMovieList':[{'movieList':movies}],
                        'subtitleList':[{'language':'es','file':[{'fileType':'srt','url':f'https://localhost:{self.server.server_port}/subtitle.srt','md5':hashlib.md5(SUBTITLE.encode()).hexdigest()}]}]}]}
                elif self.path.endswith('/getSlbInfo'):
                    assert self.path=='/api/portalCore/v15/getSlbInfo'
                    assert p['hasPay']=='1' and p['userIdentity']=='test-identity' and p['type']=='merge'
                    assert p['liveCodeList']==['test-portal_live'] and p['encMediaSupported']==1
                    assert p['pipFlag']=='0' and p['appParams']=='' and p['appVer']=='43404'
                    if self.mode=='bootstrap_ranger_slb_denied':self.respond({'returnCode':'permission-denied'});return
                    data={'error_code':0,'cdn_list':[{'tag':'vod' if self.mode in ['bootstrap_ranger_vod','bootstrap_ranger_vod_quality'] else 'live','cdn_type':'1',
                        'main_addr':('ftp://localhost/unsupported' if self.mode=='bootstrap_ranger_scheme' else f'http://localhost:{Handler.http_port}/discarded-path?old=discarded' if self.mode in ['bootstrap_ranger_http','bootstrap_ranger_http_redirect'] else f'https://localhost:{self.server.server_port}/discarded-path?old=discarded'),
                        'url_list':[{'tag':'alpha','url':AUTH}]}]}
                else:raise AssertionError('unexpected portal route')
            self.respond({'returnCode':'0','data':encrypt(data)})
        except Exception as exc:
            type(self).failures.append((self.path,type(exc).__name__,str(exc)));self.respond('failure',500)
    def do_GET(self):
        type(self).gets+=1
        try:
            if self.path=='/subtitle.srt':
                assert all(self.headers.get(k) is None for k in ['apk','App','App-Version','Ranger-Id','Content-Auth','Content-License','Authorization'])
                self.respond(SUBTITLE);return
            # Original ARM64 RequestSLB explicitly sets this header for GET.
            # Reject the former request shape with the same HTTP status reported on console.
            if self.headers.get('Content-Type')!='application/octet-stream':
                type(self).failures.append(('SLB','missing native Content-Type'))
                self.respond('missing content type',400);return
            assert self.request_version=='HTTP/1.1'
            assert self.headers.get('App')=='test.app' and self.headers.get('App-Version')=='43404'
            uid,device,seconds,_=decode_request_id(self.headers.get('Ranger-Id',''))
            assert uid==123456 and device==0 and abs(seconds-time.time())<30
            assert not self.headers.get('Content-Length') and not self.headers.get('Transfer-Encoding')
            assert self.headers.get('apk') is None and self.headers.get('Content-License') is None
            assert self.headers.get('Authorization') is None
            u=urlsplit(self.path);mode='vod' if self.mode in ['bootstrap_ranger_vod','bootstrap_ranger_vod_quality'] else 'live'
            assert u.path==('/slb/v11/vod' if mode=='vod' else '/slb/v9/live')
            query=parse_qs(u.query);assert set(query)=={'auth'}
            decoded=wire_decode(SEED,mode,query['auth'][0],False)
            if mode=='live':assert decoded==AUTH
            else:assert json.loads(decoded)==[{'auth':AUTH,'license':LICENSE,'media_code':'movie-source-hd' if self.mode=='bootstrap_ranger_vod_quality' else 'movie-source'}]
            if self.mode in ['bootstrap_ranger_redirect','bootstrap_ranger_http_redirect']:self.respond('',302,f'http://localhost:{Handler.http_port}/must-not-follow');return
            if self.mode=='bootstrap_ranger_cdn_bad_request':self.respond({'error':'bad_request','message':AUTH},400);return
            if self.mode=='bootstrap_ranger_cdn_denied':self.respond('denied',403);return
            if self.mode=='bootstrap_ranger_malformed':self.respond('not-valid-ciphertext!');return
            source='http://127.0.0.1/local-proxy' if self.mode=='bootstrap_ranger_loopback' else SIGNED_SOURCE if self.mode=='bootstrap_ranger_live_signed' else SOURCE
            data={'servers':[{'tag':'alpha','server_code':'test','media_url':source}]}
            if mode=='vod':data={'play_infos':[{'media_code':'movie-source-hd' if self.mode=='bootstrap_ranger_vod_quality' else 'movie-source','status_code':200,'servers':data['servers']}]}
            wire=wire_encode(SEED,mode,json.dumps(data,separators=(',',':')),True)
            if self.mode=='bootstrap_ranger_padded':wire+='='*((-len(wire))%4)+'\r\n'
            if self.mode=='bootstrap_ranger_gzip':self.respond(gzip.compress(wire.encode()),encoding='gzip');return
            if self.mode=='bootstrap_ranger_html':self.respond('<html>nginx error</html>');return
            self.respond(wire)
        except Exception as exc:
            type(self).failures.append((self.path.split('?')[0],type(exc).__name__,str(exc)));self.respond('failure',500)

def main(binary):
    for v in json.loads((Path(__file__).parent/'fixtures/live_auth_arm64_vectors.json').read_text()):
        assert live_content_auth(v['resource'],v['instance'],v['moment_ms'])==v['content_auth']
    for v in json.loads((Path(__file__).parent/'fixtures/ranger_id_arm64_vectors.json').read_text()):
        assert decode_request_id(v['value'])==(v['user'],v['device'],v['seconds'],v['nonce'])
    with tempfile.TemporaryDirectory() as tmp:
        folder=Path(tmp);key=rsa.generate_private_key(public_exponent=65537,key_size=2048)
        name=x509.Name([x509.NameAttribute(NameOID.COMMON_NAME,'localhost')]);now=datetime.datetime.now(datetime.timezone.utc)
        cert=(x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key()).serial_number(x509.random_serial_number())
              .not_valid_before(now-datetime.timedelta(minutes=1)).not_valid_after(now+datetime.timedelta(hours=1))
              .add_extension(x509.SubjectAlternativeName([x509.DNSName('localhost')]),False)
              .add_extension(x509.BasicConstraints(ca=True,path_length=None),True).sign(key,hashes.SHA256()))
        ca=folder/'ca.pem';secret=folder/'key.pem';ca.write_bytes(cert.public_bytes(serialization.Encoding.PEM))
        secret.write_bytes(key.private_bytes(serialization.Encoding.PEM,serialization.PrivateFormat.PKCS8,serialization.NoEncryption()))
        server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);ctx=ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER);ctx.load_cert_chain(str(ca),str(secret))
        server.socket=ctx.wrap_socket(server.socket,server_side=True);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        plain=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);Handler.http_port=plain.server_port
        plain_thread=threading.Thread(target=plain.serve_forever,daemon=True);plain_thread.start()
        config={'portal_hosts':[f'https://localhost:{server.server_port}'],'key_base64':SEED,'app_id':'test.app','apk_version':'43404',
            'device_id':'02:00:00:00:00:01','ca_file':str(ca),'password_salt':'test-salt','installation_serial':'stable-ranger-test'}
        path=folder/'service.json';path.write_text(json.dumps(config))
        try:
            for name in ['http','http_redirect','scheme','live','live_mp4','live_signed','padded','gzip','html','vod','vod_quality','region','play_denied','slb_denied','cdn_bad_request','cdn_denied','redirect','malformed','loopback']:
                mode='bootstrap_ranger_'+name;Handler.mode=mode;Handler.posts=0;Handler.gets=0;Handler.failures=[]
                process=subprocess.run([str(Path(binary).resolve()),str(path),mode],check=True,timeout=20,capture_output=True,text=True)
                if name=='live_signed':
                    value=process.stdout.strip();extra=parse_qs(value.split('?',1)[1]);moment=int(extra['start_moment'][0])
                    assert abs(moment-time.time()*1000)<30000 and extra['instance']==['0']
                    assert value==live_content_auth(SIGNED_SOURCE.split('media.example.invalid',1)[1],0,moment)
                assert not Handler.failures,Handler.failures
                log=Path('diagnostico.log').read_text()
                assert all(secret not in log for secret in [AUTH,LICENSE,'synthetic-session'])
                if name=='cdn_bad_request':assert 'status=400' in log and 'shape=json' in log
                assert Handler.posts==(2 if name=='region' else 5 if name=='play_denied' else 6),(name,Handler.posts)
                assert Handler.gets==(0 if name in ['region','play_denied','slb_denied','scheme'] else 3 if name=='vod_quality' else 1),(name,Handler.gets)
                print('local Ranger HTTP/TLS test:',name,'passed')
            config['portal_hosts']=[f'http://localhost:{Handler.http_port}'];path.write_text(json.dumps(config))
            Handler.posts=0;Handler.gets=0;Handler.failures=[]
            subprocess.run([str(Path(binary).resolve()),str(path),'bootstrap_http_forbidden'],check=True,timeout=20,capture_output=True)
            assert Handler.posts==0 and Handler.gets==0 and not Handler.failures
            print('portal HTTP rejected before sending account data: passed')
        finally:server.shutdown();server.server_close();thread.join();plain.shutdown();plain.server_close();plain_thread.join()
if __name__=='__main__':main(sys.argv[1])
