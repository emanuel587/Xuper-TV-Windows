#!/usr/bin/env python3
"""Static, offline evidence extractor. Does not execute APK code or contact servers."""
import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path
from loguru import logger
logger.remove()
from androguard.core.apk import APK
from androguard.core.dex import DEX

TARGETS = {
    'Lbc/f;': {'k0','P'}, 'Lbc/z1;': {'<init>','E2','D2'},
    'Lcc/k0;': {'d'}, 'Lcc/c0;': {'s'}, 'Lcc/m0;': {'n','p','u','i0'},
    'Lza/f;': {'g1','u'}, 'Lnf/c;': {'b'}, 'Lnf/b;': {'a','b','d'},
    'Lnf/PM;': {'checkServerTrusted'}, 'Lnf/G;': {'<clinit>','mk','m2'},
    'Lcom/titan/ranger/NativeJni;': {'<clinit>','Call'},
    'Ltv/danmaku/ijk/media/player/widget/IjkVideoView;': {'setVideoPath','setVideoURI'},
}
MODELS = {
    'Lcom/request/bean/LoginBean;', 'Lcom/request/result/LoginResultData;',
    'Lcom/request/result/GetAuthInfoResultData;', 'Lcom/request/result/GetPlayUrlResult;',
}
# Only names describing protocol operations are retained as literals. No embedded
# credentials, domains, full URLs, tokens or opaque constants are exported.
LITERALS = {'ranger-jni','c++_shared','nfbrand','SwlNativeService','deviceUniqueId',
    'metadata','signdata','https','PrepareProgram','Pause','Play','GetStatus',
    'getRetrofit().create(PortalApi::class.java)'}

def inspect(path):
    apk=APK(str(path))
    result={'sha256':hashlib.file_digest(path.open('rb'),'sha256').hexdigest(),
        'package':apk.get_package(),'version':apk.get_androidversion_name(),
        'version_code':apk.get_androidversion_code(),'min_sdk':apk.get_min_sdk_version(),
        'target_sdk':apk.get_target_sdk_version(),'dex':[], 'libraries':[],
        'models':{},'code_references':[], 'native_command_names':[], 'api':{}}
    commands=set()
    with zipfile.ZipFile(path) as archive:
        result['libraries']=[n for n in archive.namelist() if n.startswith('lib/') and n.endswith('.so')]
        for name in archive.namelist():
            if not name.endswith('.dex'): continue
            dex=DEX(archive.read(name))
            result['dex'].append({'name':name,'classes':len(dex.get_classes())})
            for cls in dex.get_classes():
                cn=cls.get_name()
                if cn in MODELS:result['models'][cn]=[f.get_name() for f in cls.get_fields()]
                if cn=='Lsb/b;':
                    result['api']['interface']=cn
                    result['api']['method_count']=len(cls.get_methods())
                    result['api']['annotations']=[]
                    directory=dex.CM.get_annotations_directory_item(cls.get_annotations_off())
                    for annotated in directory.get_method_annotations():
                        aset=dex.CM.get_annotation_set_item(annotated.get_annotations_off())
                        for item in aset.get_annotation_off_item():
                            ann=item.get_annotation_item().get_annotation()
                            # Keep annotation types, not opaque route literals.
                            result['api']['annotations'].append(dex.CM.get_type(ann.get_type_idx()))
                    result['api']['annotations']=sorted(set(result['api']['annotations']))
                for method in cls.get_methods():
                    if cn not in TARGETS and not cn.startswith('Lcom/titan/ranger/NativeJni$'):continue
                    selected=cn in TARGETS and method.get_name() in TARGETS[cn]
                    ref={'class':cn,'method':method.get_name(),
                         'flags':method.get_access_flags_string(),'calls':[], 'literals':[]}
                    if method.get_code():
                        for instruction in method.get_instructions():
                            op=instruction.get_name();text=instruction.get_output()
                            if op.startswith('invoke') and selected:
                                ref['calls'].append(text)
                            if op.startswith('const-string'):
                                value=text.split(', ',1)[-1].strip('"')
                                if selected and value in LITERALS: ref['literals'].append(value)
                                if cn.startswith('Lcom/titan/ranger/NativeJni$') and re.fullmatch(r'[A-Z][A-Za-z]{2,40}',value):
                                    commands.add(value)
                    if selected:result['code_references'].append(ref)
        result['native_command_names']=sorted(commands)
    return result

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    evidence=inspect(args.apk)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(evidence,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f"Evidence saved: {args.output}; {len(evidence['code_references'])} method references")
