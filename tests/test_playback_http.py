#!/usr/bin/env python3
"""Two real libmpv playbacks against a localhost server requiring media headers.
Usage: python tests/test_playback_http.py BINARY TEST_MP4 FONT_TTF [TEST_MPEG_TS]
Compile playback_smoke.cpp with XUPERSWITCH_TEST and SDL2/libmpv first.
"""
from pathlib import Path
import http.server,threading,subprocess,os,sys,tempfile,shutil,time
from urllib.parse import parse_qs
from ranger_reference import live_content_auth,decode_request_id
class Handler(http.server.BaseHTTPRequestHandler):
    requests=0;failures=[];media=b'';ts_media=b'';mode='headers'
    def log_message(self,*args):pass
    def do_GET(self):
        type(self).requests+=1
        expected='/test.mp4' if self.mode=='headers' else ('/live/source-live.mp4' if self.mode=='live_mp4' else '/live/source-live.ts')+'?token=synthetic&channel=test'
        if self.path!=expected:
            if self.mode!='route404':type(self).failures.append('incorrect media path')
            self.send_error(404);return
        try:
            assert self.headers.get('Content-License')=='synthetic-license'
            if self.mode=='headers':assert self.path=='/test.mp4' and self.headers.get('Content-Auth')=='synthetic-auth'
            else:
                assert self.path==('/live/source-live.mp4' if self.mode=='live_mp4' else '/live/source-live.ts')+'?token=synthetic&channel=test'
                assert self.headers.get('App')=='test.app' and self.headers.get('App-Version')=='43404'
                assert self.headers.get('Pragma')=='akamai-x-cache-on'
                user,device,seconds,_=decode_request_id(self.headers.get('Ranger-Id',''))
                assert user==123456 and device==0 and abs(seconds-time.time())<30
                value=self.headers.get('Content-Auth','');q=parse_qs(value.split('?',1)[1]);moment=int(q['start_moment'][0])
                assert q['instance']==['0'] and abs(moment-time.time()*1000)<30000
                assert value==live_content_auth('/auth?token=synthetic&channel=test',0,moment)
        except (AssertionError,ValueError,KeyError,IndexError):
            if self.mode!='denied':type(self).failures.append('incorrect media headers/path')
            self.send_error(403);return
        media=self.ts_media if self.mode=='live' and self.ts_media else self.media
        self.send_response(200);self.send_header('Content-Type','video/MP2T' if self.mode=='live' else 'video/mp4');self.send_header('Content-Length',str(len(media)));self.end_headers();self.wfile.write(media)
def main(binary,mp4,font,ts=None):
    Handler.media=Path(mp4).read_bytes();Handler.ts_media=Path(ts).read_bytes() if ts else b'';server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
    thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    binary=str(Path(binary).resolve());env=dict(os.environ,SDL_VIDEODRIVER='dummy')
    try:
        with tempfile.TemporaryDirectory() as tmp:
            shutil.copyfile(font,Path(tmp)/'font.ttf')
            for mode in ['headers','denied','route404','live','live_mp4']:
                Handler.mode=mode;Handler.requests=0;Handler.failures=[]
                resource='/test.mp4' if mode=='headers' else '/auth?token=synthetic&channel=test'
                subprocess.run([binary,f'http://127.0.0.1:{server.server_port}'+resource,mode],cwd=tmp,env=env,check=True,timeout=30)
                trace=(Path(tmp)/'diagnostico.log').read_text()
                assert all(secret not in trace for secret in ['synthetic-license','synthetic-auth','token=','sign2='])
                if mode in ['denied','route404']:assert ('HTTP 404' if mode=='route404' else 'HTTP 403') in trace and 'play:first_frame' not in trace
                else:assert trace.count('play:first_frame')==2 and trace.count('play:return_to_menu')==2
                assert Handler.requests>=(1 if mode in ['denied','route404'] else 2) and not Handler.failures
                print('HTTP playback test:',mode,'passed')

    finally:server.shutdown();server.server_close();thread.join()
if __name__=='__main__':main(*sys.argv[1:])
