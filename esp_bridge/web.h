#pragma once
// Single-page UI served from flash. Polls /api/status, sends commands to /api/cmd.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wash &amp; Cure</title>
<style>
:root{--bg:#f6f7f9;--card:#fff;--fg:#1d2330;--mut:#6b7280;--acc:#4f46e5;--ok:#15803d;--warn:#b45309;--bad:#b91c1c;--bd:#e5e7eb}
@media (prefers-color-scheme:dark){:root{--bg:#0f1218;--card:#181c25;--fg:#e6e8ee;--mut:#9aa3b2;--acc:#818cf8;--ok:#4ade80;--warn:#fbbf24;--bad:#f87171;--bd:#2a3040}}
*{box-sizing:border-box}body{margin:0;font:15px/1.4 system-ui,sans-serif;background:var(--bg);color:var(--fg)}
main{max-width:720px;margin:0 auto;padding:16px}
h1{font-size:20px;margin:4px 0 12px}.card{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:14px;margin-bottom:12px}
.time{font:600 56px/1 ui-monospace,monospace;letter-spacing:2px;text-align:center;margin:6px 0}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center}.grow{flex:1}
button{font:inherit;padding:9px 14px;border-radius:9px;border:1px solid var(--bd);background:var(--card);color:var(--fg);cursor:pointer}
button.p{background:var(--acc);border-color:var(--acc);color:#fff}button.sel{outline:2px solid var(--acc)}
input{font:inherit;padding:8px;border-radius:8px;border:1px solid var(--bd);background:var(--bg);color:var(--fg);width:90px}
.kv{display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:6px 12px;font-size:13px}.kv b{display:block;color:var(--mut);font-weight:500}
.pill{display:inline-block;padding:2px 10px;border-radius:99px;font-size:13px;border:1px solid var(--bd)}
.ok{color:var(--ok)}.warn{color:var(--warn)}.bad{color:var(--bad)}
pre{background:var(--bg);border:1px solid var(--bd);border-radius:8px;padding:8px;height:180px;overflow:auto;font-size:12px;margin:8px 0 0;white-space:pre-wrap}
small{color:var(--mut)}
</style></head><body><main>
<h1>Wash &amp; Cure <span id="conn" class="pill">…</span></h1>
<div class="card">
 <div class="row"><span id="mode" class="pill">–</span><span id="state" class="pill">–</span><span id="lid" class="pill">–</span><span id="uv" class="pill">UV –</span></div>
 <div class="time" id="time">--:--</div>
 <div class="row" style="justify-content:center">
  <button id="bw" onclick="c('MODE WASH')">Wash</button><button id="bc" onclick="c('MODE CURE')">Cure</button>
  <button onclick="c('PRESS MINUS')">−30 s</button><button onclick="c('PRESS PLUS')">+30 s</button>
  <button class="p" onclick="c('START')">Start</button><button onclick="c('PAUSE')">Pause</button><button onclick="c('STOP')">Stop</button>
 </div>
 <div class="row" style="justify-content:center;margin-top:8px"><span>Set time</span><input id="mm" type="number" min="0" max="99" placeholder="min"><input id="ss" type="number" min="0" max="59" placeholder="sec"><button onclick="setTime()">Set</button></div>
</div>
<div class="card"><div class="kv" id="kv"></div></div>
<div class="card">
 <div class="row"><button onclick="c('BEEP 150')">Beep</button><button onclick="c('TONE 1000 300')">Tone</button>
 <span class="grow"></span><small>Cure speed (PWM period, bigger = slower)</small><input id="cp" type="number" min="1000"><button onclick="c('CFG CURE_PERIOD '+v('cp'))">Set</button></div>
 <div class="row" style="margin-top:8px"><small>Defaults (s): wash</small><input id="dw" type="number"><small>cure</small><input id="dc" type="number"><button onclick="c('CFG WASH_DEFAULT '+v('dw'));c('CFG CURE_DEFAULT '+v('dc'))">Set</button><button onclick="c('SAVE')">Save to flash</button></div>
</div>
<div class="card">
 <div class="row"><input id="cmd" class="grow" style="width:auto" placeholder="console command, e.g. HELP" onkeydown="if(event.key=='Enter'){c(this.value);this.value=''}"><button onclick="c(document.getElementById('cmd').value)">Send</button></div>
 <pre id="log"></pre>
 <small>Raw TCP console: port 23 · Firmware: <a href="/update">ESP update</a> · MCU update: POST .bin to /api/mcu/update</small>
</div>
</main><script>
const $=id=>document.getElementById(id),v=id=>$(id).value;
function log(t){const l=$('log');l.textContent+=t+'\n';l.scrollTop=l.scrollHeight}
async function c(cmd){if(!cmd)return;log('> '+cmd);try{const r=await fetch('/api/cmd?c='+encodeURIComponent(cmd));log((await r.text()).trim())}catch(e){log('! '+e)}poll()}
function setTime(){c('TIME '+((+v('mm')||0)*60+(+v('ss')||0)))}
let cfgLoaded=false,seq=-1;
async function poll(){try{const r=await fetch('/api/status'+(seq>=0?'?since='+seq:''));const s=await r.json();seq=s.seq;$('conn').textContent=s.unit_ok?'online':'unit not responding';$('conn').className='pill '+(s.unit_ok?'ok':'bad');
 const t=+s.time||0;$('time').textContent=String(Math.floor(t/60)).padStart(2,'0')+':'+String(t%60).padStart(2,'0');
 $('mode').textContent=s.mode||'–';$('state').textContent=s.state||'–';$('state').className='pill '+(s.state=='RUNNING'?'ok':s.state=='LID_PAUSED'?'warn':'');
 $('lid').textContent='lid '+(s.lid||'–').toLowerCase();$('lid').className='pill '+(s.lid=='OPEN'?'warn':'');
 $('uv').textContent='UV '+(s.uv=='1'?'ON':'off');$('uv').className='pill '+(s.uv=='1'?'bad':'');
 $('bw').className=s.mode=='WASH'?'sel':'';$('bc').className=s.mode=='CURE'?'sel':'';
 const keys=['total','motor_en','dir','period','step_hz','rpm','leds','buttons','buzzer','manual','interlock','locked','uptime','rssi','esp_uptime'];
 $('kv').innerHTML=keys.filter(k=>k in s).map(k=>'<div><b>'+k+'</b>'+s[k]+'</div>').join('');
 if(s.events)for(const e of s.events)log(e);
 if(!cfgLoaded&&s.unit_ok){cfgLoaded=true;const r2=await fetch('/api/cmd?c=CFG');const t2=await r2.text();const m=k=>(t2.match(new RegExp(k+'=(\\d+)'))||[])[1];$('cp').value=m('cure_period')||'';$('dw').value=m('wash_default')||'';$('dc').value=m('cure_default')||''}
}catch(e){$('conn').textContent='offline';$('conn').className='pill bad'}}
setInterval(poll,1000);poll();
</script></body></html>)HTML";
