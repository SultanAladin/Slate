//============================================================================================================================================
//                                                          TERRAINPANEL.BROWSER.JS
//============================================================================================================================================
// 🧩 Real WebGL planet, volumetric sculpture, buried cavities and Texture Workspace slider interactions.

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
Page.on('console', (Message) => { if (Message.type() === 'error') Failures.push(Message.text()); });
Page.on('dialog', (Dialog) => Dialog.accept());
const Destination = process.env.LANDMASS_CAPTURE_PATH;
if (Destination) await mkdir(Destination, { recursive: true });
let Checks = 0;
function Verify(Description, Predicate) { Assert(Predicate, Description); Checks++; console.log(`PASS ${Description}`); }
async function Ready()
{
    await Page.waitForFunction(() => document.querySelector('#Viewport')?.getAttribute('aria-busy') === 'false' && document.querySelector('#BuildDuration')?.textContent.includes('ms'), null, { timeout: 30000 });
    Assert(!(await Page.locator('#Status').textContent()).includes('Extraction refused'), await Page.locator('#Status').textContent());
}
async function SaveRecipe()
{
    const Downloaded = Page.waitForEvent('download'); await Page.locator('#SaveProject').click();
    return JSON.parse(await readFile(await (await Downloaded).path(), 'utf8'));
}
async function Change(Identity, Quantity)
{
    await Page.locator(`#${Identity}`).fill(String(Quantity)); await Page.locator(`#${Identity}`).press('Tab'); await Ready();
}
async function Import(Document)
{
    await Page.locator('#ProjectFile').setInputFiles({ name: 'planet.json', mimeType: 'application/json', buffer: Buffer.from(JSON.stringify(Document)) });
}
try
{
    await Page.goto(process.env.LANDMASS_URL || 'http://127.0.0.1:5173'); await Ready();
    Verify('actual planet and worker start without a JavaScript error', Failures.length === 0 && !(await Page.locator('#RendererFailure').isVisible()));
    Verify('default world is 6,780 km across, not a 512 km square', (await Page.locator('#ExtentLabel').textContent()).includes('6,780 km diameter'));
    if (Destination) await Page.screenshot({ path: `${Destination}/PlanetOverview.png` });
    const Original = await SaveRecipe();
    Verify('saved document is a version-2 3D planet recipe', Original.Format === 'frontier-sdf-planet' && Original.Revision === 2 && Original.Span === undefined);
    await Change('Height', 30);
    Verify('typed slider readout changes the solid in metres', (await SaveRecipe()).Landforms[0].Height === 30000);
    await Page.locator('#Undo').click(); await Ready();
    Verify('one undo restores the original solid recipe', JSON.stringify(await SaveRecipe()) === JSON.stringify(Original));
    await Page.locator('#Redo').click(); await Ready();
    Verify('redo restores the typed solid edit', (await SaveRecipe()).Landforms[0].Height === 30000);
    const Dimensions = await Page.locator('#HeightRange').evaluate((Rail) => ({ Height: getComputedStyle(Rail).height, Thumb: Array.from(document.styleSheets).flatMap((Sheet) => Array.from(Sheet.cssRules)).find((Rule) => Rule.selectorText === 'input[type="range"].SliderPill::-webkit-slider-thumb')?.style.width }));
    Verify('C++ Texture Workspace rail and thumb figures are 28/26 px', Dimensions.Height === '28px' && Dimensions.Thumb === '26px');
    const Rail = await Page.locator('#HeightRange').boundingBox();
    const BeforeDrag = await SaveRecipe();
    await Page.mouse.move(Rail.x + Rail.width * 0.35, Rail.y + Rail.height / 2); await Page.mouse.down();
    await Page.mouse.move(Rail.x + Rail.width * 0.55, Rail.y + Rail.height / 2, { steps: 8 }); await Page.mouse.up(); await Ready();
    const AfterDrag = await SaveRecipe();
    Verify('pill slider dragging changes the authored solid', AfterDrag.Landforms[0].Height !== BeforeDrag.Landforms[0].Height);
    await Page.locator('#Undo').click(); await Ready();
    Verify('slider drag creates one undo, not one per mouse sample', JSON.stringify(await SaveRecipe()) === JSON.stringify(BeforeDrag));
    const Facets = await Page.locator('#FacetCount').textContent();
    await Change('PlanetRadius', 6371);
    Verify('Earth-size radius retains the same overview triangle count', (await Page.locator('#FacetCount').textContent()) === Facets);
    Verify('radius change does not stretch existing landform dimensions', (await SaveRecipe()).Landforms[0].Along === Original.Landforms[0].Along);
    await Page.locator('#MarsPreset').click(); await Ready();
    await Page.locator('#FocusLandform').click(); await Ready();
    Verify('local extraction is a cubic 3D volume', (await Page.locator('#ExtentLabel').textContent()).includes('120 × 120 × 120') && (await Page.locator('#SpacingLabel').textContent()).includes('48³ cells'));
    if (Destination) await Page.screenshot({ path: `${Destination}/LocalVolume.png` });
    await Page.locator('[data-tool="Cave"]').click();
    const Canvas = await Page.locator('#TerrainCanvas').boundingBox();
    await Page.mouse.click(Canvas.x + Canvas.width * 0.5, Canvas.y + Canvas.height * 0.53); await Ready();
    const Caved = await SaveRecipe();
    Verify('Cave tool adds a real subtractive 3D sphere', Caved.Impressions.length === 1 && Caved.Impressions[0].Operation === 'Difference' && Caved.Impressions[0].Radius > 0);
    await Page.locator('#Cutaway').click();
    Verify('Section is available in the local 3D view', (await Page.locator('#Cutaway').getAttribute('class')) === 'Selected');
    if (Destination) await Page.screenshot({ path: `${Destination}/AuthoredCavity.png` });
    await Page.locator('#Undo').click(); await Ready();
    Verify('cavity subtraction undoes atomically', (await SaveRecipe()).Impressions.length === 0);
    await Page.locator('#Redo').click(); await Ready();
    Verify('redo restores the cavity operation exactly', JSON.stringify((await SaveRecipe()).Impressions) === JSON.stringify(Caved.Impressions));
    const BeforeInvalid = await SaveRecipe();
    await Import({ Format: 'frontier-landmass', Revision: 1 });
    await Page.waitForFunction(() => document.querySelector('#Status').textContent.includes('Open refused'));
    Verify('old height-graph format refuses without changing the live planet', JSON.stringify(await SaveRecipe()) === JSON.stringify(BeforeInvalid));
    await Import(Original); await Page.waitForFunction(() => document.querySelector('#Status').textContent.includes('planet opened')); await Ready();
    Verify('saved volumetric planet reloads exactly', JSON.stringify(await SaveRecipe()) === JSON.stringify(Original));
    await Page.locator('#AddLandform').click(); await Ready();
    Verify('new landmass is a real versioned solid', (await SaveRecipe()).Landforms.length === 5);
    await Page.locator('#DeleteLandform').click(); await Ready();
    Verify('selected landmass removes independently', (await SaveRecipe()).Landforms.length === 4);
    await Page.locator('#HelpButton').click();
    Verify('guide distinguishes volumetric authoring from coarse overview', (await Page.locator('#HelpDialog').textContent()).includes('including interior surfaces'));
    await Page.locator('#CloseHelp').click();
    Verify('no page or WebGL exceptions during the complete workflow', Failures.length === 0);
    console.log(`PASS ${Checks} browser checks`);
}
finally { await Browser.close(); }
