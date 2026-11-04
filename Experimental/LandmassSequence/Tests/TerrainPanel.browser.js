//============================================================================================================================================
//                                                          TERRAINPANEL.BROWSER.JS
//============================================================================================================================================
// 🧩 Real WebGL viewport, authoring, recipe download/import and worker preview integration checks.

import Assert from 'node:assert/strict';
import { readFile, mkdir } from 'node:fs/promises';
import { chromium as Chromium } from 'playwright';

const Browser = await Chromium.launch({
    executablePath: process.env.LANDMASS_BROWSER_EXECUTABLE || undefined,
    headless: true,
    args: ['--no-sandbox', '--disable-dev-shm-usage', '--no-zygote', '--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'],
});
const Page = await Browser.newPage({ viewport: { width: 1440, height: 960 }, acceptDownloads: true });
const Failures = [];
Page.on('pageerror', (Failure) => Failures.push(Failure.message));
Page.on('dialog', (Dialog) => Dialog.accept());
const Destination = process.env.LANDMASS_CAPTURE_PATH;
if (Destination) await mkdir(Destination, { recursive: true });
let Checks = 0;
function Verify(Description, Predicate)
{
    Assert(Predicate, Description);
    Checks++;
    console.log(`PASS ${Description}`);
}
async function Ready()
{
    await Page.waitForFunction(() => document.querySelector('#Viewport')?.getAttribute('aria-busy') === 'false' && document.querySelector('#BuildDuration')?.textContent.includes('ms'), null, { timeout: 30000 });
}
async function SaveRecipe()
{
    const Downloaded = Page.waitForEvent('download');
    await Page.locator('#SaveProject').click();
    return JSON.parse(await readFile(await (await Downloaded).path(), 'utf8'));
}
async function Change(Identity, Quantity)
{
    await Page.locator(`#${Identity}`).fill(String(Quantity));
    await Page.locator(`#${Identity}`).press('Tab');
    await Ready();
}
try
{
    await Page.goto(process.env.LANDMASS_URL || 'http://127.0.0.1:5173');
    await Ready();
    Verify('WebGL surface and worker start without a page error', Failures.length === 0 && !(await Page.locator('#RendererFailure').isVisible()));
    Verify('four procedural landforms are present', await Page.locator('.LandformRow').count() === 4);
    if (Destination) await Page.screenshot({ path: `${Destination}/World.png` });
    const Original = await SaveRecipe();
    await Change('Height', 6000);
    const Edited = await SaveRecipe();
    Verify('Inspector changes the saved authoring recipe', Edited.Landforms[0].Height === 6000);
    await Page.locator('#Undo').click(); await Ready();
    Verify('undo restores exact original parameters', JSON.stringify(await SaveRecipe()) === JSON.stringify(Original));
    await Page.locator('#Redo').click(); await Ready();
    Verify('redo restores edited parameters', JSON.stringify(await SaveRecipe()) === JSON.stringify(Edited));
    const Facets = await Page.locator('#FacetCount').textContent();
    await Change('WorldSpan', 2000);
    Verify('2,000 km world keeps the same preview triangle count', await Page.locator('#FacetCount').textContent() === Facets);
    await Change('WorldSpan', 512);
    await Page.locator('#AddLandform').click(); await Ready();
    Verify('adding an island updates the Outliner and recipe', await Page.locator('.LandformRow').count() === 5 && (await SaveRecipe()).Landforms.length === 5);
    await Page.locator('#DeleteLandform').click(); await Ready();
    Verify('selected landform can be removed without deleting other landforms', await Page.locator('.LandformRow').count() === 4);
    await Page.locator('#DetailPreview').click(); await Ready();
    Verify('local detail samples a smaller extent', (await Page.locator('#ExtentLabel').textContent()).includes('32 × 32'));
    if (Destination) await Page.screenshot({ path: `${Destination}/LocalDetail.png` });
    await Page.locator('#WorldPreview').click(); await Ready();
    await Page.locator('[data-tool="Raise"]').click();
    const Canvas = await Page.locator('#TerrainCanvas').boundingBox();
    const CentreEast = Canvas.x + Canvas.width * 0.43;
    const CentreNorth = Canvas.y + Canvas.height * 0.49;
    await Page.mouse.move(CentreEast, CentreNorth);
    await Page.mouse.down();
    await Page.mouse.move(CentreEast + 25, CentreNorth + 6, { steps: 8 });
    await Page.mouse.up();
    await Ready();
    const Sculpted = await SaveRecipe();
    Verify('viewport drag produces real bounded sculpture impressions', Sculpted.Impressions.length > 0 && Sculpted.Impressions.every((Impression) => Impression.Operation === 'Raise'));
    await Page.locator('#Undo').click(); await Ready();
    Verify('one undo removes the whole pointer stroke', (await SaveRecipe()).Impressions.length === 0);
    await Page.locator('#Redo').click(); await Ready();
    Verify('redo restores the entire pointer stroke', (await SaveRecipe()).Impressions.length === Sculpted.Impressions.length);
    await Page.keyboard.press('Escape');
    Verify('Escape returns to selection', await Page.locator('[data-tool="Navigate"]').getAttribute('class') === 'Selected');
    const Invalid = { ...Sculpted, Revision: 99 };
    await Page.locator('#ProjectFile').setInputFiles({ name: 'invalid.json', mimeType: 'application/json', buffer: Buffer.from(JSON.stringify(Invalid)) });
    await Page.waitForFunction(() => document.querySelector('#Status').textContent.includes('Open refused'));
    Verify('invalid imported project preserves the entire live recipe', JSON.stringify(await SaveRecipe()) === JSON.stringify(Sculpted));
    await Page.locator('#ProjectFile').setInputFiles({ name: 'restored.json', mimeType: 'application/json', buffer: Buffer.from(JSON.stringify(Original)) });
    await Page.waitForFunction(() => document.querySelector('#Status').textContent.includes('Project opened'));
    await Ready();
    Verify('downloaded recipe reopens exactly', JSON.stringify(await SaveRecipe()) === JSON.stringify(Original));
    await Page.locator('#NewProject').click(); await Ready();
    Verify('blank project starts without landforms and selects Island', await Page.locator('.LandformRow').count() === 0 && await Page.locator('[data-tool="Island"]').getAttribute('class') === 'Selected');
    await Page.mouse.click(Canvas.x + Canvas.width * 0.5, Canvas.y + Canvas.height * 0.55); await Ready();
    Verify('Island tool places a new procedural landform on blank seabed', await Page.locator('.LandformRow').count() === 1);
    await Page.locator('#HelpButton').click();
    Verify('in-app guide explicitly states SDF and ocean limitations', (await Page.locator('#HelpDialog').textContent()).includes('not a reinitialised volumetric SDF'));
    await Page.locator('#CloseHelp').click();
    Verify('all browser interactions completed without JavaScript exceptions', Failures.length === 0);
    console.log(`PASS ${Checks} browser checks`);
}
finally
{
    await Browser.close();
}
