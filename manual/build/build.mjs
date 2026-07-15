// Generates, from the two Markdown sources in ../ :
//   manual.html            – standalone bilingual file (images embedded, CSS-only
//                            language toggle, tiny JS image hydrator). For download/offline.
//   manual-wordpress.html  – WordPress embed fragment, PAGINATED: left-sidebar chapter
//                            menu + one chapter per "page", NL/EN toggle, and a Print/PDF
//                            button that prints the full manual. Navigation + language are
//                            pure CSS (work even if scripts are stripped); a tiny script only
//                            adds the active-menu highlight. Images via IMAGE_BASE URL.
//
// Usage:  npm install   (first time)   then   node build.mjs
import { marked } from 'marked';
import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { resolve, extname, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const here = dirname(fileURLToPath(import.meta.url));
const manualDir = resolve(here, '..');

const SOURCES = [
  { md: 'handleiding.md', lang: 'nl' },
  { md: 'manual.md', lang: 'en' },
];

// Base URL where the manual images live once uploaded to the WordPress site
// (no trailing slash). Change this if you upload them to a different location,
// e.g. a subfolder: 'https://www.nrgwatch.nl/wp-content/uploads/itho-manual'.
const IMAGE_BASE = 'https://www.nrgwatch.nl/wp-content/uploads';

marked.setOptions({ gfm: true, breaks: false });
const slug = (s) => s.toLowerCase().replace(/[^\w\s-]/g, '').trim().replace(/\s+/g, '-');

// Render one source to body HTML with per-language heading ids + anchors so the two
// languages can coexist in one document without colliding (e.g. both "rf-module").
function renderBody(mdFile, lang) {
  const md = readFileSync(resolve(manualDir, mdFile), 'utf8');
  let html = marked.parse(md);
  html = html.replace(/<h([1-6])>([\s\S]*?)<\/h\1>/g, (m, lvl, inner) => {
    const text = inner.replace(/<[^>]+>/g, '')
      .replace(/&amp;/g, '&').replace(/&lt;/g, '<').replace(/&gt;/g, '>')
      .replace(/&#39;/g, "'").replace(/&quot;/g, '"');
    return `<h${lvl} id="${lang}-${slug(text)}">${inner}</h${lvl}>`;
  });
  html = html.replace(/href="#([^"]+)"/g, (m, a) => `href="#${lang}-${a}"`);
  return html;
}

// Split a rendered (per-language) body into front-matter + chapters (split at each <h2>).
// The table-of-contents chapter is dropped (the sidebar menu replaces it); the leading
// logo paragraph is dropped (the logo is shown in the top bar). The h2 id is moved to the
// wrapping <section> so each chapter is addressable as a :target without duplicate ids.
function splitIntoChapters(html, lang) {
  html = html.replace(/<hr\s*\/?>/g, '');
  const parts = html.split(/(?=<h2 )/g);
  let front = (parts.shift() || '');
  front = front.replace(/^\s*<p>\s*<img[^>]*>\s*<\/p>\s*/i, '').trim();
  const chapters = [];
  for (const part of parts) {
    const m = part.match(/^<h2 id="([^"]*)"[^>]*>([\s\S]*?)<\/h2>/);
    if (!m) continue;
    const id = m[1];
    const title = m[2].replace(/<[^>]+>/g, '').trim();
    if (id === `${lang}-inhoudsopgave` || id === `${lang}-table-of-contents`) continue;
    const body = part.replace(/^<h2 id="[^"]*"([^>]*)>/, '<h2$1>');
    chapters.push({ id, title, html: body.trim() });
  }
  return { front, chapters };
}

const BASE_CSS = `
* { box-sizing: border-box; }
img { max-width: 100%; height: auto; }
h1, h2, h3 { line-height: 1.25; margin-top: 1.8em; }
h1 { font-size: 1.9rem; }
h2 { font-size: 1.5rem; border-bottom: 1px solid #e1e4e8; padding-bottom: .3em; }
h3 { font-size: 1.2rem; }
a { color: #0969da; text-decoration: none; }
a:hover { text-decoration: underline; }
code { background: #f3f4f6; padding: .15em .35em; border-radius: 4px; font-size: .9em; }
pre { background: #f6f8fa; padding: 1rem; border-radius: 6px; overflow-x: auto; }
pre code { background: none; padding: 0; white-space: pre-wrap; word-break: break-word; }
blockquote { margin: 1em 0; padding: .5em 1em; border-left: 4px solid #d0d7de;
  background: #f6f8fa; color: #57606a; border-radius: 0 4px 4px 0; }
table { border-collapse: collapse; margin: 1em 0; }
th, td { border: 1px solid #d0d7de; padding: .5em .7em; vertical-align: top; }
hr { border: none; border-top: 1px solid #e1e4e8; margin: 2.5em 0; }
ul, ol { padding-left: 1.5em; }
`;

// ---------- 1) standalone bilingual manual.html ----------
function buildStandalone() {
  const bodies = SOURCES.map((s) => ({
    lang: s.lang,
    html: renderBody(s.md, s.lang).replace(/src="(images\/[^"]+)"/g, (m, p) => `data-img="${p}"`),
  }));

  const set = new Set();
  for (const b of bodies) {
    const re = /data-img="(images\/[^"]+)"/g; let m;
    while ((m = re.exec(b.html))) set.add(m[1]);
  }
  const IMG = {};
  for (const rel of set) {
    const p = resolve(manualDir, rel);
    if (!existsSync(p)) { console.warn('  ! missing image:', rel); continue; }
    const ext = extname(p).slice(1).toLowerCase();
    const mime = ext === 'jpg' || ext === 'jpeg' ? 'image/jpeg' : `image/${ext}`;
    IMG[rel] = `data:${mime};base64,${readFileSync(p).toString('base64')}`;
  }

  const css = `:root { color-scheme: light dark; }
body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  line-height: 1.6; color: #24292f; background: #fff; margin: 0; }
.lang-radio { position: absolute; left: -9999px; }
.lang-switch { position: sticky; top: 0; z-index: 10; display: flex; gap: .5rem; justify-content: center;
  padding: .6rem; background: rgba(255,255,255,.92); backdrop-filter: blur(6px); border-bottom: 1px solid #e1e4e8; }
.lang-switch label { cursor: pointer; padding: .35em 1em; border-radius: 999px; border: 1px solid #d0d7de;
  font-size: .9rem; user-select: none; }
#lang-nl:checked ~ .lang-switch label[for="lang-nl"],
#lang-en:checked ~ .lang-switch label[for="lang-en"] { background: #0969da; color: #fff; border-color: #0969da; }
.lang { display: none; }
#lang-nl:checked ~ .lang-nl { display: block; }
#lang-en:checked ~ .lang-en { display: block; }
main.lang { max-width: 820px; margin: 0 auto; padding: 1.5rem 1.2rem 4rem; }
${BASE_CSS}
@media (prefers-color-scheme: dark) {
  body { background: #0d1117; color: #c9d1d9; }
  .lang-switch { background: rgba(13,17,23,.92); border-bottom-color: #21262d; }
  .lang-switch label { border-color: #30363d; }
  h2 { border-bottom-color: #21262d; }
  a { color: #58a6ff; }
  code, pre, blockquote { background: #161b22; }
  blockquote { color: #8b949e; border-left-color: #30363d; }
  th, td, hr { border-color: #30363d; }
}
@media print {
  .lang-switch { display: none; }
  body { color: #000; background: #fff; }
  main.lang { max-width: none; padding: 0; }
  a { color: #000; text-decoration: underline; }
  h1, h2, h3 { page-break-after: avoid; }
  pre, blockquote, table, img { page-break-inside: avoid; }
  @page { margin: 18mm 16mm; }
}`;

  const nl = bodies.find((b) => b.lang === 'nl').html;
  const en = bodies.find((b) => b.lang === 'en').html;

  const doc = `<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Itho WiFi add-on — Handleiding / User Manual</title>
<style>${css}</style>
</head>
<body>
<input class="lang-radio" type="radio" name="lang" id="lang-nl" checked>
<input class="lang-radio" type="radio" name="lang" id="lang-en">
<nav class="lang-switch" aria-label="Language / Taal">
  <label for="lang-nl">Nederlands</label>
  <label for="lang-en">English</label>
</nav>
<main class="lang lang-nl" lang="nl">${nl}</main>
<main class="lang lang-en" lang="en">${en}</main>
<script>
var IMG=${JSON.stringify(IMG)};
document.querySelectorAll('img[data-img]').forEach(function(i){var s=IMG[i.getAttribute('data-img')];if(s){i.src=s;}});
function setDocLang(l){try{document.documentElement.lang=l;}catch(e){}}
var rnl=document.getElementById('lang-nl'),ren=document.getElementById('lang-en');
rnl.addEventListener('change',function(){if(this.checked)setDocLang('nl');});
ren.addEventListener('change',function(){if(this.checked)setDocLang('en');});
try{if((navigator.language||'').toLowerCase().indexOf('en')===0){ren.checked=true;setDocLang('en');}}catch(e){}
</script>
</body>
</html>`;

  const out = resolve(manualDir, 'manual.html');
  writeFileSync(out, doc);
  console.log(`  manual.html            ${(Buffer.byteLength(doc) / 1048576).toFixed(2)} MB, ${Object.keys(IMG).length} images embedded`);
}

// ---------- 2) WordPress embed (paginated) manual-wordpress.html ----------
function buildWordpress() {
  const blocks = SOURCES.map((s) => {
    const html = renderBody(s.md, s.lang).replace(/src="images\/([^"]+)"/g, (m, f) => `src="${IMAGE_BASE}/${f}"`);
    const { front, chapters } = splitIntoChapters(html, s.lang);
    const startId = `${s.lang}-start`;
    const startLabel = s.lang === 'nl' ? 'Start' : 'Home';
    const menuLabel = s.lang === 'nl' ? 'Inhoud' : 'Contents';
    let menu = `<li><a href="#${startId}">${startLabel}</a></li>`;
    let content = `<section class="im-chapter" id="${startId}">${front}</section>`;
    for (const c of chapters) {
      menu += `<li><a href="#${c.id}">${c.title}</a></li>`;
      content += `<section class="im-chapter" id="${c.id}">${c.html}</section>`;
    }
    return `<div class="im-lang-block im-${s.lang}-block" lang="${s.lang}">
<nav class="im-menu" aria-label="${menuLabel}"><ul>${menu}</ul></nav>
<main class="im-content">${content}</main>
</div>`;
  });
  const nlBlock = blocks[0];
  const enBlock = blocks[1];

  const css = `
.itho-manual{--im-fg:#24292f;--im-line:#e1e4e8;--im-soft:#f6f8fa;--im-link:#0969da;
  color:var(--im-fg);line-height:1.6;max-width:1080px;margin:0 auto;
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;}
.itho-manual *{box-sizing:border-box;}
.itho-manual .im-radio{position:absolute;left:-9999px;}
.itho-manual .im-bar{position:sticky;top:0;z-index:20;display:flex;align-items:center;gap:1rem;flex-wrap:wrap;
  padding:.6rem 0;margin-bottom:1.2rem;background:#fff;border-bottom:1px solid var(--im-line);}
.itho-manual .im-bar .im-logo{height:30px;width:auto;}
.itho-manual .im-langs{margin-left:auto;display:flex;gap:.4rem;}
.itho-manual .im-langs label{cursor:pointer;padding:.3em .85em;border:1px solid #d0d7de;border-radius:999px;font-size:.85rem;user-select:none;}
.itho-manual #im-lang-nl:checked ~ .im-bar .im-langs label[for="im-lang-nl"],
.itho-manual #im-lang-en:checked ~ .im-bar .im-langs label[for="im-lang-en"]{background:var(--im-link);color:#fff;border-color:var(--im-link);}
.itho-manual .im-print{cursor:pointer;padding:.35em .9em;border:1px solid var(--im-link);background:var(--im-link);color:#fff;border-radius:999px;font-size:.85rem;}
.itho-manual #im-lang-nl:checked ~ .im-body .im-en-block{display:none;}
.itho-manual #im-lang-en:checked ~ .im-body .im-nl-block{display:none;}
.itho-manual .im-lang-block{display:flex;gap:2rem;align-items:flex-start;}
.itho-manual .im-menu{flex:0 0 250px;position:sticky;top:60px;max-height:calc(100vh - 80px);overflow:auto;}
.itho-manual .im-menu ul{list-style:none;margin:0;padding:0;}
.itho-manual .im-menu a{display:block;padding:.4em .7em;border-radius:6px;color:var(--im-fg);text-decoration:none;font-size:.92rem;}
.itho-manual .im-menu a:hover{background:#f0f3f6;}
.itho-manual .im-menu a.im-current{background:var(--im-link);color:#fff;}
.itho-manual .im-content{flex:1 1 auto;min-width:0;}
.itho-manual .im-chapter{display:none;scroll-margin-top:70px;}
.itho-manual .im-chapter:target{display:block;}
.itho-manual .im-chapter.im-default{display:block;}
.itho-manual .im-content:not(:has(.im-chapter:target)) > .im-chapter:first-child{display:block;}
.itho-manual .im-chapter > h1:first-child,.itho-manual .im-chapter > h2:first-child{margin-top:0;}
.itho-manual img{max-width:100%;height:auto;}
.itho-manual h1{font-size:1.9rem;line-height:1.25;}
.itho-manual h2{font-size:1.5rem;line-height:1.25;margin-top:1.8em;border-bottom:1px solid var(--im-line);padding-bottom:.3em;}
.itho-manual h3{font-size:1.2rem;line-height:1.25;margin-top:1.6em;}
.itho-manual a{color:var(--im-link);}
.itho-manual code{background:#f3f4f6;padding:.15em .35em;border-radius:4px;font-size:.9em;}
.itho-manual pre{background:var(--im-soft);padding:1rem;border-radius:6px;overflow-x:auto;}
.itho-manual pre code{background:none;padding:0;white-space:pre-wrap;word-break:break-word;}
.itho-manual blockquote{margin:1em 0;padding:.5em 1em;border-left:4px solid #d0d7de;background:var(--im-soft);color:#57606a;border-radius:0 4px 4px 0;}
.itho-manual table{border-collapse:collapse;margin:1em 0;}
.itho-manual th,.itho-manual td{border:1px solid #d0d7de;padding:.5em .7em;vertical-align:top;}
.itho-manual ul,.itho-manual ol{padding-left:1.5em;}
@media(max-width:760px){
  .itho-manual .im-lang-block{flex-direction:column;}
  .itho-manual .im-menu{position:static;flex-basis:auto;max-height:none;width:100%;border-bottom:1px solid var(--im-line);margin-bottom:1rem;}
}
@media print{
  .itho-manual .im-bar,.itho-manual .im-menu{display:none!important;}
  .itho-manual .im-lang-block{display:block;}
  .itho-manual .im-chapter{display:block!important;}
  .itho-manual .im-content{max-width:none;}
  .itho-manual a{color:#000;}
  .itho-manual h2,.itho-manual h3{page-break-after:avoid;}
  .itho-manual pre,.itho-manual blockquote,.itho-manual table,.itho-manual img{page-break-inside:avoid;}
  @page{margin:18mm 16mm;}
}
`;

  const frag = `<!--
  Itho WiFi add-on manual — WordPress embed (bilingual, PAGINATED).
  - Left sidebar = chapter menu; clicking a chapter shows just that chapter.
  - NL/EN toggle and the chapter navigation are pure CSS (work even if scripts are stripped);
    a tiny script only adds the active-menu highlight.
  - "Print / PDF" prints the full manual in the selected language (or use Ctrl/Cmd+P).
  Images are referenced at ${IMAGE_BASE}/<filename>.
  1. Upload the contents of manual/images/ to that location (so e.g.
     ${IMAGE_BASE}/05-vochtsensor-cve-pcb.jpg resolves).
  2. Paste everything below into a "Custom HTML" block (or the Code Editor) on a WordPress page.
  (To change the image location, edit IMAGE_BASE in manual/build/build.mjs and rebuild.)
-->
<style>${css}</style>
<div class="itho-manual">
<input class="im-radio" type="radio" name="im-lang" id="im-lang-nl" checked>
<input class="im-radio" type="radio" name="im-lang" id="im-lang-en">
<div class="im-bar">
  <img class="im-logo" src="${IMAGE_BASE}/00-nrgwatch-logo.png" alt="NRG.Watch">
  <div class="im-langs">
    <label for="im-lang-nl">Nederlands</label>
    <label for="im-lang-en">English</label>
  </div>
  <button type="button" class="im-print" onclick="window.print()">&#128424; Print / PDF</button>
</div>
<div class="im-body">
${nlBlock}
${enBlock}
</div>
</div>
<script>
(function(){var r=document.querySelector('.itho-manual');if(!r)return;
function upd(){var h=location.hash.replace('#',''),noHash=!h;
r.querySelectorAll('.im-content').forEach(function(c){var f=c.querySelector('.im-chapter');if(f)f.classList.toggle('im-default',noHash);});
var any=false;r.querySelectorAll('.im-menu a').forEach(function(a){var on=a.getAttribute('href')==='#'+h;a.classList.toggle('im-current',on);if(on)any=true;});
if(!any)r.querySelectorAll('.im-menu').forEach(function(m){var f=m.querySelector('a');if(f)f.classList.add('im-current');});}
upd();window.addEventListener('hashchange',upd);})();
</script>
`;

  const dst = resolve(manualDir, 'manual-wordpress.html');
  writeFileSync(dst, frag);
  console.log(`  manual-wordpress.html  ${(Buffer.byteLength(frag) / 1024).toFixed(0)} KB, paginated, no base64`);
}

console.log('Building manual outputs from handleiding.md + manual.md ...');
buildStandalone();
buildWordpress();
console.log('Done.');
