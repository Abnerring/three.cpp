"""Verify full pinned dependency trees and apply two content-checked patches.
The frontend fork changes three glslang files. SPIRV-Tools' build-only patch keeps
Python codegen bytecode outside source trees, without ignoring any extra file.
"""
import hashlib,json,pathlib,re,sys
lock_path,frontend_patch,*directories=map(pathlib.Path,sys.argv[1:])
lock=json.loads(lock_path.read_text(encoding='utf-8'))
digest=lambda data:hashlib.sha256(data).hexdigest()
def tree(path):
    h=hashlib.sha256()
    files=sorted((p for p in path.rglob('*') if p.is_file()),key=lambda p:p.relative_to(path).as_posix())
    for file in files:
        h.update(file.relative_to(path).as_posix().encode('utf-8')+b'\0'+hashlib.sha256(file.read_bytes()).digest())
    return h.hexdigest()
for item in lock['licenses']:
    if digest((lock_path.parent.parent.parent/item['file']).read_bytes())!=item['sha256']:
        raise RuntimeError('Bundled dependency license hash mismatch: '+item['file'])
def apply(source,patch_path,patch_sha,records,tree_before):
    patch=patch_path.read_bytes()
    if digest(patch)!=patch_sha:raise RuntimeError('Dependency patch hash mismatch: '+patch_path.name)
    states=[]
    for item in records:
        value=digest((source/item['file']).read_bytes())
        if value==item['beforeSHA256']:states.append('before')
        elif value==item['afterSHA256']:states.append('after')
        else:raise RuntimeError('Dependency source hash mismatch: '+item['file'])
    if len(set(states))!=1:raise RuntimeError('Mixed dependency patch state')
    if states[0]=='after':return
    if tree(source)!=tree_before:raise RuntimeError('Original dependency tree mismatch')
    lines=patch.decode('utf-8').splitlines(keepends=True);i=0;prepared={}
    while i<len(lines):
        if not lines[i].startswith('--- a/'):raise RuntimeError('Unexpected patch framing')
        rel=lines[i][6:].strip();i+=1
        if lines[i]!='+++ b/'+rel+'\n':raise RuntimeError('Patch path mismatch')
        i+=1;original=(source/rel).read_bytes().decode('utf-8').splitlines(keepends=True);output=[];cursor=0
        while i<len(lines) and lines[i].startswith('@@ '):
            match=re.fullmatch(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@\n',lines[i])
            if not match:raise RuntimeError('Unexpected patch hunk')
            start,old_count,new_start,new_count=(int(n) if n is not None else 1 for n in match.groups());i+=1
            output.extend(original[cursor:start-1]);cursor=start-1;removed=added=0
            if len(output)!=new_start-1:raise RuntimeError('Patch new-line position mismatch')
            while i<len(lines) and not lines[i].startswith(('@@ ','--- a/')):
                line=lines[i];i+=1
                if line[0] in ' -':
                    if original[cursor]!=line[1:]:raise RuntimeError('Patch context mismatch: '+rel)
                    cursor+=1;removed+=1
                if line[0] in ' +':output.append(line[1:]);added+=1
                if line[0] not in ' +-':raise RuntimeError('Unexpected hunk line')
            if removed!=old_count or added!=new_count:raise RuntimeError('Patch line count mismatch')
        output.extend(original[cursor:]);prepared[rel]=''.join(output).encode('utf-8')
    if prepared.keys()!={item['file'] for item in records}:raise RuntimeError('Unexpected patch target files')
    for item in records:
        if digest(prepared[item['file']])!=item['afterSHA256']:raise RuntimeError('Patched dependency hash mismatch: '+item['file'])
    for rel,data in prepared.items():(source/rel).write_bytes(data)
frontend=[dict(item,file=item['file'].split('/',2)[2]) for item in lock['patchedDependencyFiles']]
apply(directories[0],frontend_patch,lock['frontendPatchSHA256'],frontend,lock['sourceTrees'][0]['beforeSHA256'])
tools=lock['toolsBuildScriptPatch']
apply(directories[1],lock_path.parent/tools['file'],tools['sha256'],tools['files'],lock['sourceTrees'][1]['beforeSHA256'])
for directory,item in zip(directories,lock['sourceTrees']):
    if tree(directory)!=item['afterSHA256']:raise RuntimeError('Pinned dependency tree mismatch: '+item['name'])
print('Pinned glslang frontend fork, SPIRV-Tools codegen-build patch and SPIRV-Headers complete trees verified')
