import React, { useEffect, useRef, useState } from 'react';
import { WebGPURenderer } from './webgpu/renderer';

const SliderRow = ({ label, value, unit, min, max, step, onChange, displayValue = null }: any) => {
  const percentage = ((value - min) / (max - min)) * 100;
  const formattedValue = displayValue !== null ? displayValue : value;
  return (
    <div className="flex items-center justify-between">
      <div className="text-[17px] text-[#9a9a9a] font-normal tracking-wide flex-1">{label}</div>
      <div className="flex items-center gap-3">
         <div className="bg-[#212121] flex items-center justify-between rounded-[12px] px-3 h-[38px] w-[86px]">
            <span className="text-[#f5f5f5] text-[16px] font-normal">{formattedValue}</span>
            {unit && <span className="text-[#555] text-[14px]">{unit}</span>}
         </div>
         <div className="relative w-[86px] h-[26px] bg-[#333333] rounded-full flex items-center group">
            <div className="absolute inset-y-0 left-[8px] right-[8px] pointer-events-none">
              <div 
                className="absolute top-1/2 -translate-y-1/2 -ml-[6px] w-[12px] h-[18px] bg-[#ececec] rounded-full shadow-sm transition-transform duration-200 group-hover:scale-110 group-active:scale-95"
                style={{ left: `${percentage}%` }}
              />
            </div>
            <input 
              type="range" min={min} max={max} step={step} value={value}
              onChange={e => onChange(parseFloat(e.target.value))}
              className="absolute inset-0 w-full h-full opacity-0 cursor-pointer"
            />
         </div>
      </div>
    </div>
  );
};

const ToggleRow = ({ label, checked, onChange }: any) => {
  return (
    <div className="flex items-center justify-between">
      <div className="text-[17px] text-[#9a9a9a] font-normal tracking-wide">{label}</div>
      <button 
        onClick={() => onChange(!checked)}
        className={`relative w-[56px] h-[30px] rounded-full transition-colors duration-200 outline-none ${checked ? 'bg-[#5a5a5a]' : 'bg-[#333333]'}`}
      >
        <div 
           className={`absolute top-[4px] left-[4px] w-[22px] h-[22px] bg-[#ececec] rounded-full transition-transform duration-200 shadow-sm ${checked ? 'translate-x-[26px]' : 'translate-x-0'}`}
        />
      </button>
    </div>
  );
};

export default function App() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const rendererRef = useRef<WebGPURenderer | null>(null);
  const [error, setError] = useState<string | null>(null);

  const [temporalWeight, setTemporalWeight] = useState(1.0);
  const [spatialRadius, setSpatialRadius] = useState(30);
  const [spatialSamples, setSpatialSamples] = useState(3);
  const [enableTemporal, setEnableTemporal] = useState(true);
  const [enableReprojection, setEnableReprojection] = useState(true);
  const [enableSpatial, setEnableSpatial] = useState(true);
  const [enableDenoise, setEnableDenoise] = useState(true);
  const [enableReflections, setEnableReflections] = useState(true);
  const [reflectionBounces, setReflectionBounces] = useState(2);
  const [reflectionGI, setReflectionGI] = useState(true);
  const [diffuseBounces, setDiffuseBounces] = useState(2);
  const [denoiserType, setDenoiserType] = useState(1);
  const [movingSpp, setMovingSpp] = useState(2);
  const [playing, setPlaying] = useState(false);

  // New Environment Controls
  const [sunAzimuth, setSunAzimuth] = useState(1.0);
  const [sunElevation, setSunElevation] = useState(0.5);
  const [sunIntensity, setSunIntensity] = useState(20.0);
  const [turbidity, setTurbidity] = useState(3.0);
  const [exposure, setExposure] = useState(1.0);

  useEffect(() => {
    if (!canvasRef.current) return;
    
    let isActive = true;
    let renderer: WebGPURenderer | null = null;
    const abortController = new AbortController();
    
    const init = async () => {
      try {
        renderer = new WebGPURenderer(canvasRef.current!);
        await renderer.init(abortController.signal);
        if (!isActive) {
          renderer.cleanup();
          return;
        }
        rendererRef.current = renderer;
        renderer.start();
      } catch (e: any) {
        if (isActive) setError(e.message || "Failed to init WebGPU");
      }
    };
    init();
    
    return () => {
      isActive = false;
      abortController.abort();
      if (renderer) {
        renderer.cleanup();
      }
      if (rendererRef.current === renderer) {
        rendererRef.current = null;
      }
    };
  }, []);

  useEffect(() => {
    if (rendererRef.current) {
      rendererRef.current.updateSettings({
        temporalWeight,
        spatialRadius,
        spatialSamples,
        enableTemporal: enableTemporal ? 1 : 0,
        enableReprojection: enableReprojection ? 1 : 0,
        enableSpatial: enableSpatial ? 1 : 0,
        enableDenoise: enableDenoise ? 1 : 0,
        enableReflections: enableReflections ? 1 : 0,
        reflectionBounces,
        reflectionGI: reflectionGI ? 1 : 0,
        diffuseBounces,
        denoiserType: denoiserType,
        movingSpp,
        sunAzimuth,
        sunElevation,
        sunIntensity,
        turbidity,
        exposure
      });
      if (playing) rendererRef.current.start();
      else rendererRef.current.stop();
    }
  }, [temporalWeight, spatialRadius, spatialSamples, enableTemporal, enableReprojection, enableSpatial, enableDenoise, enableReflections, reflectionBounces, reflectionGI, diffuseBounces, denoiserType, movingSpp, sunAzimuth, sunElevation, sunIntensity, turbidity, exposure, playing]);

  return (
    <div className="flex h-screen w-full bg-[#0a0a0a] text-white overflow-hidden font-sans">
      <div className="w-[380px] p-6 bg-[#090909] border-r border-[#1f1f1f] flex flex-col gap-6 shrink-0 z-10 overflow-y-auto">
        <div className="pl-1">
          <h1 className="text-[22px] font-medium tracking-wide text-[#fff]">ReSTIR Render</h1>
          <p className="text-[#666] text-[15px] mt-1 tracking-wide">Global Illumination settings</p>
        </div>

        {error ? (
          <div className="bg-red-900/50 border border-red-500 text-red-200 p-4 rounded-xl text-sm">
            {error}
          </div>
        ) : (
          <div className="flex flex-col gap-5">
             <div className="bg-[#151515] rounded-[28px] p-6 text-[15px] text-[#9a9a9a] leading-relaxed">
               <strong>Click the canvas</strong> to capture your mouse. Use <strong>W A S D</strong> to move, <strong>Q E</strong> for height, and <strong>Shift</strong> to sprint. Press ESC to release.
             </div>

             <div className="bg-[#151515] rounded-[28px] p-6 flex flex-col gap-6">
                 <ToggleRow label="Accumulation" checked={enableTemporal} onChange={setEnableTemporal} />
                 <ToggleRow label="Reprojection" checked={enableReprojection} onChange={setEnableReprojection} />
                 <ToggleRow label="Reflections" checked={enableReflections} onChange={setEnableReflections} />
                 
                 {enableReflections && (
                   <div className="flex flex-col gap-4 pl-4 border-l border-[#333]">
                     <SliderRow label="Max Bounces" value={reflectionBounces} unit="x" min={1} max={5} step={1} onChange={setReflectionBounces} />
                     <ToggleRow label="Reflection GI" checked={reflectionGI} onChange={setReflectionGI} />
                   </div>
                 )}

                 <SliderRow label="Diffuse Bounces" value={diffuseBounces} unit="x" min={1} max={5} step={1} onChange={setDiffuseBounces} />
                 
                 <SliderRow label="History" value={temporalWeight} displayValue={temporalWeight.toFixed(2)} unit="u" min={0} max={1} step={0.05} onChange={setTemporalWeight} />
                 <SliderRow label="Movement" value={movingSpp} unit="spp" min={1} max={8} step={1} onChange={setMovingSpp} />
             </div>

             <div className="bg-[#151515] rounded-[28px] p-6 flex flex-col gap-6">
                 <ToggleRow label="Spatial Filter" checked={enableSpatial} onChange={setEnableSpatial} />
                 <SliderRow label="Radius" value={spatialRadius} unit="px" min={1} max={100} step={1} onChange={setSpatialRadius} />
                 <SliderRow label="Samples" value={spatialSamples} unit="x" min={1} max={10} step={1} onChange={setSpatialSamples} />
             </div>

             <div className="bg-[#151515] rounded-[28px] p-6 flex flex-col gap-6">
                 <div className="text-[17px] text-[#fff] font-medium tracking-wide">Environment</div>
                 <SliderRow label="Sun Azimuth" value={sunAzimuth} unit="rad" min={0} max={6.28} step={0.01} onChange={setSunAzimuth} displayValue={sunAzimuth.toFixed(2)} />
                 <SliderRow label="Sun Elevation" value={sunElevation} unit="rad" min={-0.2} max={1.57} step={0.01} onChange={setSunElevation} displayValue={sunElevation.toFixed(2)} />
                 <SliderRow label="Sun Intensity" value={sunIntensity} unit="x" min={0} max={100} step={1} onChange={setSunIntensity} displayValue={sunIntensity.toFixed(0)} />
                 <SliderRow label="Turbidity" value={turbidity} unit="" min={2} max={10} step={0.1} onChange={setTurbidity} displayValue={turbidity.toFixed(1)} />
                 <SliderRow label="Exposure" value={exposure} unit="ev" min={0.1} max={5.0} step={0.1} onChange={setExposure} displayValue={exposure.toFixed(1)} />
             </div>

             <div className="bg-[#151515] rounded-[28px] p-6 flex flex-col gap-6">
                 <ToggleRow label="Post Denoise" checked={enableDenoise} onChange={setEnableDenoise} />
                 
                 {enableDenoise && (
                   <div className="flex flex-col gap-2">
                     <span className="text-[13px] text-gray-400">Denoiser Algorithm</span>
                     <select
                       value={denoiserType}
                       onChange={(e) => setDenoiserType(Number(e.target.value))}
                       className="bg-[#2a2a2a] text-white rounded-[12px] px-3 py-2 outline-none text-[13px] w-full"
                     >
                       <option value={1}>Bilateral (Basic)</option>
                       <option value={2}>🏆 A-Trous Wavelet (Best)</option>
                       <option value={3}>🏆 SVGF Approx (High Quality)</option>
                       <option value={4}>⭐ IGN + TAA (Fast Blue Noise)</option>
                       <option value={5}>⭐ AMD FFX-Style (Compute Beast)</option>
                     </select>
                   </div>
                 )}
             </div>
             
             <div className="flex gap-3 mt-2">
                <button onClick={() => setPlaying(!playing)} className="flex-1 flex items-center justify-center bg-[#151515] hover:bg-[#222222] p-4 rounded-[16px] transition-colors text-[16px] text-[#eee] tracking-wide outline-none">
                   {playing ? 'Pause Engine' : 'Resume Engine'}
                </button>
             </div>
          </div>
        )}
      </div>
      
      <div className="flex-1 relative bg-black flex items-center justify-center p-4">
          <canvas ref={canvasRef} className="w-full h-full object-contain rounded-[20px] shadow-2xl border border-[#1a1a1a]" />
      </div>
    </div>
  );
}
