# Parse SWM190.h typedef structs + base defines -> "addr name" register label list
import re,sys
src=open(sys.argv[1]).read()
src=re.sub(r'//.*','',src); src=re.sub(r'/\*.*?\*/','',src,flags=re.S)
sizes={'uint32_t':4,'uint16_t':2,'uint8_t':1}
structs={}
def parse(body):
    off=0; regs=[]
    for m in re.finditer(r'(struct\s*\{.*?\}|union\s*\{.*?\}|[^;{}]+);',body,re.S):
        s=m.group(1).strip()
        if s.startswith('union') or s.startswith('struct'):
            inner=parse(s[s.index('{')+1:s.rindex('}')])
            mx=0
            for (o,n,sz) in inner[0]:
                regs.append((off+o,n,sz))
            off+= inner[1] if s.startswith('struct') else max([o+sz for o,n,sz in inner[0]] or [0])
            continue
        mm=re.match(r'(?:__IO|__I|__O)?\s*(uint\d+_t)\s+(\w+)(?:\[(\d+)\])?',s)
        if not mm: continue
        t,n,cnt=mm.groups(); sz=sizes[t]; cnt=int(cnt) if cnt else None
        if cnt:
            if not n.startswith('RESERVED'):
                for i in range(cnt): regs.append((off+i*sz,f'{n}[{i}]',sz))
            off+=sz*cnt
        else:
            if not n.startswith('RESERVED'): regs.append((off,n,sz))
            off+=sz
    return regs,off
for m in re.finditer(r'typedef\s+struct\s*\{(.*?)\}\s*(\w+)\s*;',src,re.S):
    structs[m.group(2)]=parse(m.group(1))[0]
defs={}
for m in re.finditer(r'#define\s+(\w+_BASE)\s+(.+)',src):
    defs[m.group(1)]=m.group(2).strip()
def ev(e): 
    for k in sorted(defs,key=len,reverse=True): e=e.replace(k,'('+defs[k]+')')
    return eval(e)
for m in re.finditer(r'#define\s+(\w+)\s+\(\(\s*(\w+)\s*\*\s*\)\s*(\w+_BASE)\s*\)',src):
    name,ty,base=m.groups(); b=ev(defs[base])
    print(f'{b:08x} {name} {ty} 0')
    for o,n,sz in structs.get(ty,[]): print(f'{b+o:08x} {name}_{n.replace("[","").replace("]","")} reg {sz}')
