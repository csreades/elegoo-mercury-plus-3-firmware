import re,struct,sys
# usage: subst.py IMAGE.bin decomp_in.c decomp_out.c  (run from this directory)
d=open(sys.argv[1],'rb').read()
labels={}
for l in open('swm190_regs.txt'):
    a,n,t,s=l.split(); a=int(a,16)
    if t!='reg' and a not in labels: labels[a]=n
fn={0x1514:'GPIO_Init',0x1786:'GPIO_SetBit',0x1792:'GPIO_ClrBit',0x179e:'GPIO_InvBit',0x17aa:'GPIO_GetBit',0x1884:'PORT_Init',0x138:'delay_ms',0x2c5c:'__udiv',0x2c88:'__sdivmod'}
src=open(sys.argv[2]).read()
def rep(m):
    a=int(m.group(1),16)
    if a>=len(d): return m.group(0)
    v=struct.unpack_from('<I',d,a)[0]
    if v in labels: return labels[v]
    if 0x20000000<=v<0x20010000: return 'RAM_%08x'%v
    return '0x%x'%v
src=re.sub(r'DAT_([0-9a-f]{8})',rep,src)
for a,n in fn.items(): src=src.replace('FUN_%08x'%a,n)
open(sys.argv[3],'w').write(src)
