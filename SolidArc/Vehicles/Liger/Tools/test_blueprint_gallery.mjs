// Reuse the pinned browser toolchain from Refined/Tools (npm ci there first).
import {createRequire} from 'node:module';
const require=createRequire(new URL('../Refined/Tools/package.json',import.meta.url));
const {chromium:playwright}=require('playwright-core');
const module=require('@sparticuz/chromium');const Chromium=module.default??module;
const browser=await playwright.launch({executablePath:await Chromium.executablePath(),args:Chromium.args,headless:true});
const page=await browser.newPage({viewport:{width:1280,height:1000},deviceScaleFactor:1});
const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('response',r=>{if(r.status()>=400)errors.push(`${r.status()} ${r.url()}`)});
await page.goto(process.env.LIGER_BLUEPRINTS_URL||'http://127.0.0.1:8081/');
for(const layer of ['References','PatchLayouts','Schematics']){
 await page.selectOption('#layer',layer);
 for(const name of ['Top','Left','Right','Front','Rear','Bottom']){
  await page.click(`[data-view="${name}"]`);
  await page.waitForFunction(()=>{const i=document.querySelector('#drawing');return i.complete&&i.naturalWidth>0&&!document.querySelector('#dimensions').textContent.includes('loading')});
  const src=await page.locator('#drawing').getAttribute('src');
  if(!src.includes(name))throw Error('Wrong view image');
  const sizes=await page.evaluate(()=>{const i=document.querySelector('#drawing');return [i.getBoundingClientRect().width,document.querySelector('.canvas').getBoundingClientRect().width]});
  if(sizes[0]>sizes[1]+1)throw Error('Image exceeds viewport');
 }
 console.log('PASS all six '+layer+' images load and fit');
}
await page.selectOption('#layer','References');await page.click('[data-view="Right"]');
await page.waitForFunction(()=>document.querySelector('#drawing').complete&&!document.querySelector('#dimensions').textContent.includes('loading'));
await page.screenshot({path:new URL('../Blueprints/Validation/ReferenceGallery.png',import.meta.url).pathname,fullPage:true});
for(const link of await page.locator('a[href$=".zip"]').all()){
 const url=await link.getAttribute('href');const r=await page.request.head(new URL(url,page.url()).href);
 if(r.status()!==200||Number(r.headers()['content-length'])<1000000)throw Error('Package missing: '+url);
}console.log('PASS all three packaged downloads');
const download=page.waitForEvent('download');await page.click('a[download][href="Liger.solidarc"]');
if((await download).suggestedFilename()!=='Liger.solidarc')throw Error('Wrong project download');console.log('PASS Liger.solidarc download');
await page.setViewportSize({width:390,height:844});
if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth+1))throw Error('Mobile overflow');
console.log('PASS mobile layout');
await browser.close();if(errors.length)throw Error(errors.join('\n'));
console.log('JavaScript errors / failed HTTP responses: 0');
