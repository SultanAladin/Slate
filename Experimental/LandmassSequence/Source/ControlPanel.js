//============================================================================================================================================
//                                                              CONTROLPANEL.JS
//============================================================================================================================================
// 🧩 Browser counterpart of Texture Workspace ControlKit pill sliders and split numeric/unit readouts.

import { Clamp } from './PlanetProjection.js';

// 📝 Figures follow Frontier ControlKit::Slider / ValuePill: 28 px rail, 26 px thumb,
// half-thumb pointer travel, flush fill, held thumb ×1.12, and adjoining numeric/unit cells.
export function SliderMarkup(Identity, Caption, Minimum, Maximum, Step, Quantity, Unit)
{
    return `<div class="ControlRow"><div class="ControlHeading"><label for="${Identity}">${Caption}</label><div class="ValuePill"><input id="${Identity}" type="number" min="${Minimum}" max="${Maximum}" step="${Step}" value="${Quantity}"/><span>${Unit}</span></div></div><input id="${Identity}Range" class="SliderPill" type="range" min="${Minimum}" max="${Maximum}" step="${Step}" value="${Quantity}" aria-label="${Caption}"/></div>`;
}

export function ReflectSlider(Identity, Quantity)
{
    const Numeric = document.getElementById(Identity);
    const Rail = document.getElementById(`${Identity}Range`);
    const Decimals = Math.min(6, (Numeric.step.split('.')[1] || '').length);
    Numeric.value = Number(Quantity.toFixed(Decimals));
    Rail.value = Numeric.value;
    const Fraction = Clamp((Number(Rail.value) - Number(Rail.min)) / (Number(Rail.max) - Number(Rail.min)), 0, 1);
    const Width = Rail.getBoundingClientRect().width;
    Rail.style.setProperty('--fraction', Width > 26 ? `${13 + Fraction * (Width - 26)}px` : `${Fraction * 100}%`);
}

export function ConnectSlider(Identity, Accept)
{
    const Numeric = document.getElementById(Identity);
    const Rail = document.getElementById(`${Identity}Range`);
    Rail.oninput = () => ReflectSlider(Identity, Number(Rail.value));
    const Release = (Quantity) =>
    {
        if (!Number.isFinite(Quantity)) return;
        const Bounded = Clamp(Quantity, Number(Numeric.min), Number(Numeric.max));
        ReflectSlider(Identity, Bounded);
        Accept(Bounded);
    };
    Rail.onchange = () => Release(Number(Rail.value));
    Numeric.onchange = () => Release(Number(Numeric.value));
    ReflectSlider(Identity, Number(Numeric.value));
    new ResizeObserver(() => ReflectSlider(Identity, Number(Numeric.value))).observe(Rail);
}
