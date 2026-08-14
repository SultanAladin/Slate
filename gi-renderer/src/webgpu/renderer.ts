/// <reference types="@webgpu/types" />
import { computeWGSL, renderWGSL } from './wgsl';
import { buildScene } from './mesh';

export class WebGPURenderer {
  canvas: HTMLCanvasElement;
  device!: GPUDevice;
  context!: GPUCanvasContext;
  format!: GPUTextureFormat;
  
  uniformBuffer!: GPUBuffer;
  accumBufferA!: GPUBuffer;
  accumBufferB!: GPUBuffer;
  gbufferA!: GPUBuffer;
  gbufferB!: GPUBuffer;
  resBufferA!: GPUBuffer;
  resBufferB!: GPUBuffer;
  resSpatial!: GPUBuffer;
  trianglesBuffer!: GPUBuffer;
  bvhBuffer!: GPUBuffer;
  colorTexture!: GPUTexture;

  bindGroupA!: GPUBindGroup;
  bindGroupB!: GPUBindGroup;
  renderBindGroup!: GPUBindGroup;
  
  computePipelineTemporal!: GPUComputePipeline;
  computePipelineSpatial!: GPUComputePipeline;
  computePipelineResolve!: GPUComputePipeline;
  computePipelinePostprocess!: GPUComputePipeline;
  renderPipeline!: GPURenderPipeline;
  
  frameCount = 0;
  reqFrame = 0;
  destroyed = false;
  
  settings = {
    temporalWeight: 1.0,
    spatialRadius: 30,
    spatialSamples: 3,
    enableTemporal: 1,
    enableReprojection: 1,
    enableSpatial: 1,
    enableDenoise: 1,
    movingSpp: 2,
    sunAzimuth: 1.0,
    sunElevation: 0.5,
    sunIntensity: 20.0,
    turbidity: 3.0,
    skyIntensity: 1.0,
    sunAngularRadius: 0.05,
    maxHistory: 30.0,
    initialCandidates: 16,
    normalThreshold: 0.9,
    depthThreshold: 0.1,
    exposure: 1.0,
    denoiserType: 1,
    enableReflections: 1,
    reflectionBounces: 2,
    reflectionGI: 1,
    diffuseBounces: 2
  };
  
  width = 0;
  height = 0;

  // First-Person Camera State
  camPos = [0, 2, -15];
  yaw = 0;
  pitch = 0;
  keys: Record<string, boolean> = {};
  isLocked = false;
  isMoving = false;

  prevCamPos = [0, 2, -15];
  prevDir = [0, 0, -1];
  prevUp = [0, 1, 0];
  prevRight = [1, 0, 0];
  
  cleanupEvents?: () => void;
  
  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
  }
  
  async init(abortSignal?: AbortSignal) {
    if (!navigator.gpu) throw new Error("WebGPU not supported on this browser.");
    const adapter = await navigator.gpu.requestAdapter({ powerPreference: "high-performance" });
    if (!adapter) throw new Error("No appropriate GPUAdapter found.");
    if (abortSignal?.aborted) return;
    
    this.device = await adapter.requestDevice({
      requiredLimits: {
        maxStorageBufferBindingSize: adapter.limits.maxStorageBufferBindingSize,
        maxComputeWorkgroupsPerDimension: adapter.limits.maxComputeWorkgroupsPerDimension,
        maxStorageBuffersPerShaderStage: adapter.limits.maxStorageBuffersPerShaderStage,
      }
    });
    if (abortSignal?.aborted) {
      this.device.destroy();
      return;
    }
    
    this.context = this.canvas.getContext('webgpu') as GPUCanvasContext;
    this.format = navigator.gpu.getPreferredCanvasFormat();
    
    let cw = this.canvas.clientWidth || 800;
    let ch = this.canvas.clientHeight || 600;
    if(cw > 800) cw = 800;
    if(ch > 600) ch = 600;
    this.width = cw;
    this.height = ch;
    this.canvas.width = cw;
    this.canvas.height = ch;
    
    this.context.configure({
      device: this.device,
      format: this.format,
      alphaMode: 'opaque'
    });
    
    const numPixels = this.width * this.height;
    
    this.uniformBuffer = this.device.createBuffer({
        size: 256, // 64 floats * 4 bytes
        usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST
    });
    
    this.accumBufferA = this.device.createBuffer({ size: numPixels * 16, usage: GPUBufferUsage.STORAGE });
    this.accumBufferB = this.device.createBuffer({ size: numPixels * 16, usage: GPUBufferUsage.STORAGE });
    this.gbufferA = this.device.createBuffer({ size: numPixels * 64, usage: GPUBufferUsage.STORAGE });
    this.gbufferB = this.device.createBuffer({ size: numPixels * 64, usage: GPUBufferUsage.STORAGE });
    this.resBufferA = this.device.createBuffer({ size: numPixels * 48, usage: GPUBufferUsage.STORAGE });
    this.resBufferB = this.device.createBuffer({ size: numPixels * 48, usage: GPUBufferUsage.STORAGE });
    this.resSpatial = this.device.createBuffer({ size: numPixels * 48, usage: GPUBufferUsage.STORAGE });
    
    const scene = buildScene();
    this.trianglesBuffer = this.device.createBuffer({
        size: scene.triBuffer.byteLength,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST
    });
    this.device.queue.writeBuffer(this.trianglesBuffer, 0, scene.triBuffer);

    this.bvhBuffer = this.device.createBuffer({
        size: scene.bvhBuffer.byteLength,
        usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST
    });
    this.device.queue.writeBuffer(this.bvhBuffer, 0, scene.bvhBuffer);
    
    this.colorTexture = this.device.createTexture({
        size: [this.width, this.height, 1],
        format: 'rgba16float',
        usage: GPUTextureUsage.STORAGE_BINDING | GPUTextureUsage.TEXTURE_BINDING
    });
    
    const computeModule = this.device.createShaderModule({ code: computeWGSL });
    const renderModule = this.device.createShaderModule({ code: renderWGSL });
    
    const computeLayout = this.device.createBindGroupLayout({
        entries: [
            { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
            { binding: 1, visibility: GPUShaderStage.COMPUTE, storageTexture: { access: 'write-only', format: 'rgba16float' } },
            { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 5, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 6, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 7, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 8, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
            { binding: 9, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
            { binding: 10, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
        ]
    });
    
    const pipelineLayout = this.device.createPipelineLayout({ bindGroupLayouts: [computeLayout] });
    
    this.computePipelineTemporal = this.device.createComputePipeline({
        layout: pipelineLayout,
        compute: { module: computeModule, entryPoint: 'pass_temporal' }
    });
    this.computePipelineSpatial = this.device.createComputePipeline({
        layout: pipelineLayout,
        compute: { module: computeModule, entryPoint: 'pass_spatial' }
    });
    this.computePipelineResolve = this.device.createComputePipeline({
        layout: pipelineLayout,
        compute: { module: computeModule, entryPoint: 'pass_resolve' }
    });
    this.computePipelinePostprocess = this.device.createComputePipeline({
        layout: pipelineLayout,
        compute: { module: computeModule, entryPoint: 'pass_postprocess' }
    });
    
    this.bindGroupA = this.device.createBindGroup({
        layout: computeLayout,
        entries: [
            { binding: 0, resource: { buffer: this.uniformBuffer } },
            { binding: 1, resource: this.colorTexture.createView() },
            { binding: 2, resource: { buffer: this.accumBufferA } },
            { binding: 3, resource: { buffer: this.accumBufferB } },
            { binding: 4, resource: { buffer: this.gbufferA } },
            { binding: 5, resource: { buffer: this.gbufferB } },
            { binding: 6, resource: { buffer: this.resBufferA } },
            { binding: 7, resource: { buffer: this.resBufferB } },
            { binding: 8, resource: { buffer: this.resSpatial } },
            { binding: 9, resource: { buffer: this.trianglesBuffer } },
            { binding: 10, resource: { buffer: this.bvhBuffer } },
        ]
    });
    this.bindGroupB = this.device.createBindGroup({
        layout: computeLayout,
        entries: [
            { binding: 0, resource: { buffer: this.uniformBuffer } },
            { binding: 1, resource: this.colorTexture.createView() },
            { binding: 2, resource: { buffer: this.accumBufferB } },
            { binding: 3, resource: { buffer: this.accumBufferA } },
            { binding: 4, resource: { buffer: this.gbufferB } },
            { binding: 5, resource: { buffer: this.gbufferA } },
            { binding: 6, resource: { buffer: this.resBufferB } },
            { binding: 7, resource: { buffer: this.resBufferA } },
            { binding: 8, resource: { buffer: this.resSpatial } },
            { binding: 9, resource: { buffer: this.trianglesBuffer } },
            { binding: 10, resource: { buffer: this.bvhBuffer } },
        ]
    });
    
    const renderLayout = this.device.createBindGroupLayout({
        entries: [
            { binding: 0, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'unfilterable-float' } }
        ]
    });
    this.renderBindGroup = this.device.createBindGroup({
        layout: renderLayout,
        entries: [{ binding: 0, resource: this.colorTexture.createView() }]
    });
    
    this.renderPipeline = this.device.createRenderPipeline({
        layout: this.device.createPipelineLayout({ bindGroupLayouts: [renderLayout] }),
        vertex: { module: renderModule, entryPoint: 'vs_main' },
        fragment: { module: renderModule, entryPoint: 'fs_main', targets: [{ format: this.format }] },
        primitive: { topology: 'triangle-list' }
    });

    this.setupInputs();
  }
  
  setupInputs() {
    const onMouseDown = () => this.canvas.requestPointerLock();
    const onPointerLock = () => {
      this.isLocked = document.pointerLockElement === this.canvas;
    };
    const onMouseMove = (e: MouseEvent) => {
        if (!this.isLocked) return;
        this.yaw += e.movementX * 0.002;
        this.pitch -= e.movementY * 0.002;
        this.pitch = Math.max(-Math.PI/2.1, Math.min(Math.PI/2.1, this.pitch));
        this.isMoving = true;
    };
    const onKeyDown = (e: KeyboardEvent) => this.keys[e.key.toLowerCase()] = true;
    const onKeyUp = (e: KeyboardEvent) => this.keys[e.key.toLowerCase()] = false;

    this.canvas.addEventListener('mousedown', onMouseDown);
    document.addEventListener('pointerlockchange', onPointerLock);
    document.addEventListener('mousemove', onMouseMove);
    document.addEventListener('keydown', onKeyDown);
    document.addEventListener('keyup', onKeyUp);

    this.cleanupEvents = () => {
        this.canvas.removeEventListener('mousedown', onMouseDown);
        document.removeEventListener('pointerlockchange', onPointerLock);
        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('keydown', onKeyDown);
        document.removeEventListener('keyup', onKeyUp);
    };
  }

  getCameraBasis() {
    const dirX = Math.cos(this.pitch) * Math.sin(this.yaw);
    const dirY = Math.sin(this.pitch);
    const dirZ = Math.cos(this.pitch) * Math.cos(this.yaw); // Z-forward

    // right = cross(WorldUp(0,1,0), dir)
    let rightX = dirZ;
    let rightZ = -dirX;
    const rLen = Math.hypot(rightX, rightZ);
    if (rLen > 0.0001) {
        rightX /= rLen;
        rightZ /= rLen;
    } else {
        rightX = 1; rightZ = 0;
    }

    // up = cross(dir, right)
    const upX = dirY * rightZ;
    const upY = dirZ * rightX - dirX * rightZ;
    const upZ = -dirY * rightX;

    return { dirX, dirY, dirZ, rightX, rightZ, upX, upY, upZ };
  }

  updateUniforms() {
    const arr = new Float32Array(64);
    const u32arr = new Uint32Array(arr.buffer);
    
    arr[0] = this.width;
    arr[1] = this.height;
    u32arr[2] = this.frameCount;
    arr[3] = this.settings.temporalWeight;
    arr[4] = this.settings.spatialRadius;
    u32arr[5] = this.settings.spatialSamples;
    u32arr[6] = this.settings.enableTemporal;
    u32arr[7] = this.settings.enableSpatial;
    u32arr[8] = this.settings.enableDenoise;
    u32arr[9] = this.settings.movingSpp;
    u32arr[10] = this.isMoving ? 1 : 0;
    u32arr[11] = this.settings.enableReprojection;

    const { dirX, dirY, dirZ, rightX, rightZ, upX, upY, upZ } = this.getCameraBasis();
    
    // Current Cam
    arr[12] = this.camPos[0]; arr[13] = this.camPos[1]; arr[14] = this.camPos[2];
    arr[16] = dirX; arr[17] = dirY; arr[18] = dirZ;
    arr[20] = upX; arr[21] = upY; arr[22] = upZ;
    arr[24] = rightX; arr[25] = 0.0; arr[26] = rightZ;
    
    // Previous Cam
    arr[28] = this.prevCamPos[0]; arr[29] = this.prevCamPos[1]; arr[30] = this.prevCamPos[2];
    arr[32] = this.prevDir[0]; arr[33] = this.prevDir[1]; arr[34] = this.prevDir[2];
    arr[36] = this.prevUp[0]; arr[37] = this.prevUp[1]; arr[38] = this.prevUp[2];
    arr[40] = this.prevRight[0]; arr[41] = 0.0; arr[42] = this.prevRight[2];
    
    // Sun and Sky
    arr[44] = this.settings.sunAzimuth;
    arr[45] = this.settings.sunElevation;
    arr[46] = this.settings.sunIntensity;
    arr[47] = this.settings.turbidity;
    arr[48] = this.settings.skyIntensity;
    arr[49] = this.settings.sunAngularRadius;
    arr[50] = this.settings.maxHistory;
    u32arr[51] = this.settings.initialCandidates;
    arr[52] = this.settings.normalThreshold;
    arr[53] = this.settings.depthThreshold;
    arr[54] = this.settings.exposure;
    u32arr[55] = this.settings.denoiserType;
    u32arr[56] = this.settings.enableReflections;
    u32arr[57] = this.settings.reflectionBounces;
    u32arr[58] = this.settings.reflectionGI;
    u32arr[59] = this.settings.diffuseBounces;

    this.device.queue.writeBuffer(this.uniformBuffer, 0, arr.buffer);
    
    this.prevCamPos = [...this.camPos];
    this.prevDir = [dirX, dirY, dirZ];
    this.prevUp = [upX, upY, upZ];
    this.prevRight = [rightX, 0.0, rightZ];
  }
  
  render() {
    if (this.destroyed) return;

    let moved = false;
    let speed = 0.3;
    if (this.keys['shift']) speed *= 3.0;
    
    const { dirX, dirY, dirZ, rightX, rightZ } = this.getCameraBasis();

    if (this.keys['w']) { this.camPos[0] += dirX*speed; this.camPos[1] += dirY*speed; this.camPos[2] += dirZ*speed; moved = true; }
    if (this.keys['s']) { this.camPos[0] -= dirX*speed; this.camPos[1] -= dirY*speed; this.camPos[2] -= dirZ*speed; moved = true; }
    if (this.keys['a']) { this.camPos[0] -= rightX*speed; this.camPos[2] -= rightZ*speed; moved = true; }
    if (this.keys['d']) { this.camPos[0] += rightX*speed; this.camPos[2] += rightZ*speed; moved = true; }
    if (this.keys['e'] || this.keys[' ']) { this.camPos[1] += speed; moved = true; }
    if (this.keys['q']) { this.camPos[1] -= speed; moved = true; }
    
    this.isMoving = moved;
    if (moved) {
        // Only trigger accumulation reset if camera moved significantly through pointer lock, but we rely on reprojection now
        // so we don't need to call resetAccumulation() at all.
    }

    this.updateUniforms();
    
    const passGroups = this.frameCount % 2 === 0 ? this.bindGroupA : this.bindGroupB;
    const commandEncoder = this.device.createCommandEncoder();
    
    const computePass = commandEncoder.beginComputePass();
    computePass.setPipeline(this.computePipelineTemporal);
    computePass.setBindGroup(0, passGroups);
    const workgroupsX = Math.ceil(this.width / 8);
    const workgroupsY = Math.ceil(this.height / 8);
    computePass.dispatchWorkgroups(workgroupsX, workgroupsY);

    computePass.setPipeline(this.computePipelineSpatial);
    computePass.setBindGroup(0, passGroups);
    computePass.dispatchWorkgroups(workgroupsX, workgroupsY);

    computePass.setPipeline(this.computePipelineResolve);
    computePass.setBindGroup(0, passGroups);
    computePass.dispatchWorkgroups(workgroupsX, workgroupsY);

    computePass.setPipeline(this.computePipelinePostprocess);
    computePass.setBindGroup(0, passGroups);
    computePass.dispatchWorkgroups(workgroupsX, workgroupsY);
    
    computePass.end();

    const renderPass = commandEncoder.beginRenderPass({
        colorAttachments: [{
            view: this.context.getCurrentTexture().createView(),
            clearValue: { r: 0, g: 0, b: 0, a: 1 },
            loadOp: 'clear',
            storeOp: 'store'
        }]
    });
    renderPass.setPipeline(this.renderPipeline);
    renderPass.setBindGroup(0, this.renderBindGroup);
    renderPass.draw(6);
    renderPass.end();

    this.device.queue.submit([commandEncoder.finish()]);
    
    this.frameCount++;
    this.reqFrame = requestAnimationFrame(() => this.render());
  }
  
  start() {
    if(!this.reqFrame) this.render();
  }
  
  stop() {
    cancelAnimationFrame(this.reqFrame);
    this.reqFrame = 0;
  }
  
  updateSettings(newSettings: any) {
    let changed = false;
    for(let k in newSettings) {
       // @ts-ignore
       if(this.settings[k] !== newSettings[k]) {
          // @ts-ignore
          this.settings[k] = newSettings[k];
          changed = true;
       }
    }
    if (changed) this.resetAccumulation();
  }
  
  resetAccumulation() {
    this.frameCount = 0;
  }
  
  cleanup() {
    this.destroyed = true;
    this.stop();
    if (this.cleanupEvents) this.cleanupEvents();
    this.device?.destroy();
  }
}
