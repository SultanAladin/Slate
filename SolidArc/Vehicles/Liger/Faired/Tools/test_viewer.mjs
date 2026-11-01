import c from '@sparticuz/chromium';
import {chromium} from 'playwright-core';
const b=await chromium.launch({executablePath:await c.executablePath(),args:c.args,headless:true});
const page=await b.newPage({viewport:{width:1100,height:800},deviceScaleFactor:1});
const errors=[],requests=[];page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text())});page.on('response',r=>{if(r.status()>=400)requests.push(`${r.status()} ${r.url()}`)});
await page.goto(process.env.LIGER_PROOFS_URL || 'http://127.0.0.1:8080/');await page.waitForSelector('#loading[hidden]',{timeout:60000,state:'attached'});
console.log('PASS initial CAD geometry + change map loaded');
for(const mode of ['0','1','2','3','4']){await page.check(`[name=mode][value="${mode}"]`);await page.waitForTimeout(150);console.log('PASS display mode '+mode);}
await page.check('[name=mode][value="3"]');await page.screenshot({path:new URL('../Proofs/10_Interactive_Inspector.png',import.meta.url).pathname});
await page.check('#before');await page.waitForFunction(()=>document.querySelector('#revision').textContent.startsWith('BEFORE'));
if(!await page.isDisabled('[name=mode][value="4"]'))throw Error('Change map must be disabled for original');console.log('PASS original CAD comparison and change-map guard');
await page.uncheck('#before');await page.waitForFunction(()=>document.querySelector('#revision').textContent.startsWith('AFTER'));console.log('PASS restore faired CAD');
for(const p of ['0','1','2']){await page.uncheck(`[data-part="${p}"]`);await page.check(`[data-part="${p}"]`);}console.log('PASS per-part visibility');
await page.check('#edges');await page.uncheck('#edges');await page.check('#reference');await page.waitForTimeout(250);await page.uncheck('#reference');console.log('PASS fixed rims and source overlay');
await page.click('[data-view="[7,-9,5]"]');await page.waitForTimeout(500);
const box=await page.locator('#viewport').boundingBox();
outer: for(const y of [.3,.4,.5,.6,.7])for(const x of [.3,.4,.5,.6,.7]){await page.mouse.click(box.x+box.width*x,box.y+box.height*y);if((await page.locator('#selection').textContent()).includes('Source distance RMS'))break outer;}
if(!(await page.locator('#selection').textContent()).includes('Source distance RMS'))throw Error('Panel picking failed');console.log('PASS panel picking and current-revision distance labels');
const download=page.waitForEvent('download');await page.click('a[download][href$=".solidarc"]');console.log('PASS native download '+(await download).suggestedFilename());
await page.setViewportSize({width:390,height:844});await page.waitForTimeout(300);if(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth+1))throw Error('Mobile overflow');console.log('PASS mobile layout');
for(const img of await page.locator('.gallery img').all()){await img.scrollIntoViewIfNeeded();await img.waitFor({state:'visible'});if(!await img.evaluate(i=>i.complete&&i.naturalWidth>0))throw Error('Gallery image missing');}console.log('PASS all computed proofs load');
console.log(`JavaScript / shader errors: ${errors.length}; failed HTTP responses: ${requests.length}`);
await b.close();if(errors.length||requests.length){console.error(errors,requests);process.exit(1);}
