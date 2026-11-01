"""Guard native reference minification against dotted/missing high-resolution ink.
Reprojects the registered Top image into the actual native capture's camera/rect.
This checks visible ink, not geometric tolerance or Windows desktop execution.
"""
import json
import math
from pathlib import Path
import tomllib
import numpy as np
from PIL import Image
from scipy.ndimage import maximum_filter


def main():
    root=Path(__file__).resolve().parents[1]/'Blueprints'
    capture=json.loads((root/'Validation/GPU/Liger_Top.json').read_text())
    view=json.loads((root/'registration.json').read_text())['views'][0]
    fov=tomllib.loads((root/'Views/Top.toml').read_text())['Viewport']['FieldOfView']
    camera=capture['Camera'];x,y,w,h=capture['Viewport']
    assert camera['Orthographic'] and abs(camera['Elevation']-math.pi/2)<1e-8
    scale=h/(2*camera['Distance']*math.tan(fov/2))
    W=round(np.linalg.norm(view['right'])*scale);H=round(np.linalg.norm(view['down'])*scale)
    left=round(x+w/2+(view['corner'][0]-camera['Target'][0])*scale)
    top=round(y+h/2-(view['corner'][1]-camera['Target'][1])*scale)
    with Image.open(root/'References/Top.png') as image:
        alpha=np.array(image.getchannel('A').resize((W,H),Image.Resampling.LANCZOS))
        # Opaque visible-curve cores and intentionally faint guides have different
        # contrast budgets. Keep a strict core check, and a one-pixel rasterisation
        # allowance for all ink (including the new translucent/dashed guides).
        opaque=np.where(np.array(image.getchannel('A'))==255,255,0).astype(np.uint8)
        visible_alpha=np.array(Image.fromarray(opaque).resize((W,H),Image.Resampling.LANCZOS))
    with Image.open(root/'Validation/GPU/Liger_Top.png') as image:
        pixels=np.array(image.convert('RGB'))[top:top+H,left:left+W]
    assert pixels.shape[:2]==(H,W)
    expected=alpha>40
    ink=np.max(abs(pixels.astype(float)-[23,27,35]),axis=2)>12
    direct=float(np.mean(ink[expected]));near=float(np.mean(maximum_filter(ink,size=3)[expected]))
    core=visible_alpha>96  # minified opaque-curve core, excluding low-alpha fringes
    core_direct=float(np.mean(ink[core]));core_near=float(np.mean(maximum_filter(ink,size=3)[core]))
    assert np.count_nonzero(expected)>10000 and direct>.90 and near>.99
    assert np.count_nonzero(core)>10000 and core_direct>.98 and core_near>.995
    result=dict(visible_core_expected_pixels=int(np.count_nonzero(core)),visible_core_direct_coverage=core_direct,
        visible_core_coverage_within_one_pixel=core_near,expected_ink_pixels=int(np.count_nonzero(expected)),direct_native_ink_coverage=direct,
        coverage_within_one_pixel=near,source='Actual Vulkan Liger_Top.png capture / registered Top reference',
        note='Opaque cores: alpha>96 after minification, >98% direct coverage. All ink including faint guides: >99% within one pixel. Screen-space visibility guard, not a geometric tolerance certificate. Tested on Linux / SwiftShader.')
    (root/'Validation/MinificationCheck.json').write_text(json.dumps(result,indent=2))
    print(json.dumps(result,indent=2))


if __name__=='__main__':main()
