import { readFile, writeFile } from 'node:fs/promises';
import { resolve, dirname } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const dataDir = dirname(fileURLToPath(import.meta.url));
const exampleDir = resolve(dataDir, '..');
// nice_gfx/examples/08_Applications/MosaicoCameraDemo/data -> GFX/nice_gfx_editor.
const editorRoot = process.argv[2]
  ? resolve(process.argv[2])
  : resolve(dataDir, '../../../../../nice_gfx_editor');
const load = file => import(pathToFileURL(resolve(editorRoot, 'packages/html-ui', file)).href);
const { compileProject } = await load('src/index.js');
const { defaultHardware, validateHardware } = await load('hardware.mjs');
const { createArduinoPreview } = await load('arduino.mjs');
const project = {
  version: 1,
  name: 'MosaicoCameraDemo',
  width: 480,
  height: 480,
  canvasShape: 'square',
  initialRoute: '/home',
  hardware: defaultHardware('esp-mosaico'),
  pages: [{
    id: 'home', name: 'Camera', path: '/home',
    html: await readFile(resolve(dataDir, 'home.html'), 'utf8'),
    css: await readFile(resolve(dataDir, 'home.css'), 'utf8'),
  }],
};
const hardwareErrors = validateHardware(project);
if (hardwareErrors.length) throw new Error(hardwareErrors.join('\n'));
const compiled = compileProject(project);
if (!compiled.ok) throw new Error(compiled.diagnostics.map(d => `${d.severity} ${d.code}: ${d.message}`).join('\n'));
const expected = ['preview', 'status', 'fps', 'detect', ...Array.from({ length: 4 }, (_, i) => `box${i}`), ...Array.from({ length: 4 }, (_, i) => `label${i}`)];
const nodes = new Map(compiled.pages[0].nodes.map(node => [node.id, node]));
for (const id of expected) if (!nodes.has(id)) throw new Error(`Missing public ID ${id}`);
for (const node of nodes.values()) {
  if (node.x < 0 || node.y < 0 || node.x + node.width > 480 || node.y + node.height > 480) throw new Error(`Out of screen: ${node.id}`);
}
const preview = nodes.get('preview');
if (preview.x !== 80 || preview.y !== 96 || preview.width !== 320 || preview.height !== 240 || !preview.mediaDynamic) throw new Error('Invalid camera viewport');
for (let i = 0; i < 4; ++i) for (const prefix of ['box', 'label']) {
  if (!nodes.get(`${prefix}${i}`).runtimeHidden) throw new Error(`${prefix}${i} must start hidden`);
}
const arduino = createArduinoPreview(project);
if (arduino.files['ui.h'] !== compiled.uiHeader) throw new Error('Compiler and Arduino UI output differ');
// Repository examples use the installed NICE_GFX library. The complete Arduino
// exporter normally emits includes for its self-contained src/nice_gfx tree.
function installedHeaders(text) {
  return text.replace(/"src\/nice_gfx\/([^"\r\n]+)"/g, '<$1>');
}
const generatedHeaders = {
  'ui.h': installedHeaders(compiled.uiHeader),
  'Hardware.h': installedHeaders(arduino.files['Hardware.h']),
  'TouchAdapter.h': installedHeaders(arduino.files['TouchAdapter.h']),
};
for (const [file, text] of Object.entries(generatedHeaders)) {
  if (text.includes('src/nice_gfx/')) throw new Error(`Bundled-library include remains in ${file}`);
}
await writeFile(resolve(dataDir, 'project.gfx.json'), JSON.stringify(project, null, 2) + '\n');
for (const [file, text] of Object.entries(generatedHeaders)) await writeFile(resolve(exampleDir, file), text);
await writeFile(resolve(dataDir, 'ui-build-report.json'), JSON.stringify({
  compiler: '@nice-gfx/html-ui', hardwareErrors, diagnostics: compiled.diagnostics,
  stats: compiled.stats,
  viewport: { x: preview.x, y: preview.y, width: preview.width, height: preview.height },
  publicElements: Object.fromEntries(expected.map(id => {
    const n = nodes.get(id); return [id, { kind: n.kind, x: n.x, y: n.y, width: n.width, height: n.height, hidden: !!n.runtimeHidden }];
  })),
}, null, 2) + '\n');
console.log(JSON.stringify({ ok: compiled.ok, pages: compiled.stats.pages, nodes: compiled.stats.nodes, diagnostics: compiled.diagnostics, viewport: { x: preview.x, y: preview.y, width: preview.width, height: preview.height }, generatedHeaders: Object.keys(generatedHeaders), editorRoot, exampleDir, dataDir }, null, 2));
