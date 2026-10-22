import fs from 'node:fs/promises';
import path from 'node:path';
import assert from 'node:assert/strict';
const option=name=>process.argv.find(v=>v.startsWith(`--${name}=`))?.slice(name.length+3);
const baseline=option('baseline'),optimized=option('optimized'),out=option('out');
if(!baseline||!optimized||!out)throw Error('Pass --baseline=transcript --optimized=transcript --out=directory');
const columns='scene,status,mode,wall_us,frames,fps,avg_ms,p95_recent_ms,max_ms,draw_ms,io_other_ms,pixels,ui_tiles,ui_bytes,actions,events,observed_errors,heap_min,psram_free,warmup_errors'.split(',');
async function read(file){
  const transcript=await fs.readFile(file,'utf8');
  assert(!/^BENCH_(?:SKIP|FATAL|ABORT)/m.test(transcript),'Incomplete/aborted benchmark');
  const lines=transcript.split(/\r?\n/).filter(s=>/^BENCH_CSV,[^,]+,(?:OK|ERROR|NO_FRAMES),/.test(s));
  assert.equal(lines.length,27,'Require three complete nine-scene rounds');
  const rows=lines.map((line,i)=>{
    const fields=line.split(',').slice(1);assert.equal(fields.length,columns.length,'Malformed CSV');
    const row=Object.fromEntries(columns.map((c,j)=>[c,j<3?fields[j]:Number(fields[j])]));
    assert.equal(row.status,'OK');assert.equal(row.observed_errors,0);assert.equal(row.warmup_errors,0);
    assert(columns.slice(3).every(c=>Number.isFinite(row[c])));assert(row.frames>0 && row.wall_us>=5900000);
    assert(Math.abs(row.fps-row.frames*1e6/row.wall_us)<0.001);
    return {round:Math.floor(i/9)+1,...row};
  });
  assert.equal(new Set(rows.map(r=>r.scene)).size,9);
  for(let round=1;round<=3;++round)assert.equal(new Set(rows.filter(r=>r.round===round).map(r=>r.scene)).size,9);
  return {rows,csv:'round,'+columns.join(',')+'\n'+rows.map(r=>[r.round,...columns.map(c=>r[c])].join(',')).join('\n')+'\n'};
}
const [a,b]=await Promise.all([read(baseline),read(optimized)]);
const median=values=>[...values].sort((a,b)=>a-b)[Math.floor(values.length/2)];
const summary=[...new Set(a.rows.map(r=>r.scene))].map(scene=>{
  const left=a.rows.filter(r=>r.scene===scene),right=b.rows.filter(r=>r.scene===scene);
  assert.equal(right.length,3);assert.equal(left[0].mode,right[0].mode);
  const oldFps=median(left.map(r=>r.fps)),newFps=median(right.map(r=>r.fps));
  return {scene,baseline_fps:oldFps,optimized_fps:newFps,improvement_percent:(newFps/oldFps-1)*100,
    baseline_p95_ms:median(left.map(r=>r.p95_recent_ms)),optimized_p95_ms:median(right.map(r=>r.p95_recent_ms)),
    baseline_range:[Math.min(...left.map(r=>r.fps)),Math.max(...left.map(r=>r.fps))],
    optimized_range:[Math.min(...right.map(r=>r.fps)),Math.max(...right.map(r=>r.fps))]};
});
await fs.mkdir(out,{recursive:true});
await Promise.all([fs.writeFile(path.join(out,'baseline.csv'),a.csv),fs.writeFile(path.join(out,'optimized.csv'),b.csv),
  fs.writeFile(path.join(out,'comparison.json'),JSON.stringify(summary,null,2)+'\n')]);
for(const r of summary)console.log(`${r.scene}: ${r.baseline_fps.toFixed(3)} -> ${r.optimized_fps.toFixed(3)} FPS (${r.improvement_percent.toFixed(1)}%); p95 ${r.baseline_p95_ms} -> ${r.optimized_p95_ms} ms`);
