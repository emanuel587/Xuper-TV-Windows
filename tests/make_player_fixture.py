#!/usr/bin/env python3
"""Create the synthetic 30s/two-audio/SRT fixture used by playback_controls.cpp."""
from pathlib import Path
import subprocess,sys,tempfile
output=Path(sys.argv[1]).resolve()
output.parent.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory() as tmp:
    subtitle=Path(tmp)/'test.srt'
    subtitle.write_text('1\n00:00:00,000 --> 00:00:25,000\nSubtitulos de prueba\n')
    subprocess.run(['ffmpeg','-v','error','-y','-f','lavfi','-i','testsrc2=size=320x180:rate=15',
        '-f','lavfi','-i','sine=frequency=440:sample_rate=48000','-f','lavfi','-i','sine=frequency=880:sample_rate=48000',
        '-i',str(subtitle),'-t','30','-map','0:v','-map','1:a','-map','2:a','-map','3:s',
        '-c:v','libx264','-preset','ultrafast','-crf','30','-c:a','aac','-b:a','48k','-c:s','srt',
        '-metadata:s:a:0','language=spa','-metadata:s:a:1','language=eng','-metadata:s:s:0','language=spa',str(output)],check=True)
print(output)
