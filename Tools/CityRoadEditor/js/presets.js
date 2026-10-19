// Road type preset library. All dimensions in metres.
// lanes = total asphalt lanes (both directions), laneW = width of one lane.
export const PRESETS = {
  motorway:   { label:'Motorway — 6 lane divided', lanes:6, laneW:3.6, median:4.0, medianKind:'barrier', bike:0, parking:0, sidewalk:0, shoulder:2.4, surface:'asphaltDark', center:'median', edgeLine:true,  dash:true, crosswalk:false, speed:110, bump:false, trees:false },
  highway:    { label:'Highway — 4 lane divided',  lanes:4, laneW:3.5, median:3.0, medianKind:'grass',   bike:0, parking:0, sidewalk:0, shoulder:2.0, surface:'asphaltDark', center:'median', edgeLine:true,  dash:true, crosswalk:false, speed:90,  bump:false, trees:false },
  arterial:   { label:'Arterial — 4 lane + bike',  lanes:4, laneW:3.4, median:0,   medianKind:'none',    bike:1.8, parking:0, sidewalk:2.6, shoulder:0, surface:'asphalt', center:'doubleYellow', edgeLine:true, dash:true, crosswalk:true, speed:60, bump:false, trees:true },
  boulevard:  { label:'Boulevard — 4 lane treed median', lanes:4, laneW:3.4, median:5.0, medianKind:'grass', bike:1.8, parking:0, sidewalk:3.4, shoulder:0, surface:'asphalt', center:'median', edgeLine:true, dash:true, crosswalk:true, speed:50, bump:false, trees:true },
  collector:  { label:'Collector — 2 lane + bike + parking', lanes:2, laneW:3.3, median:0, medianKind:'none', bike:1.7, parking:2.2, sidewalk:2.4, shoulder:0, surface:'asphalt', center:'dashedYellow', edgeLine:true, dash:false, crosswalk:true, speed:40, bump:false, trees:true },
  residential:{ label:'Residential — 2 lane + parking', lanes:2, laneW:3.1, median:0, medianKind:'none', bike:0, parking:2.1, sidewalk:2.2, shoulder:0, surface:'asphaltLight', center:'none', edgeLine:false, dash:false, crosswalk:true, speed:30, bump:true, trees:true },
  street:     { label:'Small street — 2 lane narrow', lanes:2, laneW:2.8, median:0, medianKind:'none', bike:0, parking:0, sidewalk:1.8, shoulder:0, surface:'asphaltLight', center:'none', edgeLine:false, dash:false, crosswalk:true, speed:30, bump:false, trees:true },
  avenue1way: { label:'One-way avenue — 3 lane', lanes:3, laneW:3.3, median:0, medianKind:'none', bike:1.8, parking:2.2, sidewalk:3.0, shoulder:0, surface:'asphalt', center:'oneway', edgeLine:true, dash:true, crosswalk:true, speed:40, bump:false, trees:true, oneway:true },
  country:    { label:'Country road — 2 lane, gravel shoulder', lanes:2, laneW:3.2, median:0, medianKind:'none', bike:0, parking:0, sidewalk:0, shoulder:1.6, surface:'asphaltOld', center:'dashedWhite', edgeLine:true, dash:false, crosswalk:false, speed:80, bump:false, trees:false },
  offroad:    { label:'Offroad dirt track', lanes:2, laneW:2.6, median:0, medianKind:'none', bike:0, parking:0, sidewalk:0, shoulder:0.8, surface:'dirt', center:'none', edgeLine:false, dash:false, crosswalk:false, speed:20, bump:false, trees:false, dirt:true },
  alley:      { label:'Alley / service lane', lanes:1, laneW:3.4, median:0, medianKind:'none', bike:0, parking:0, sidewalk:0, shoulder:0, surface:'concrete', center:'none', edgeLine:false, dash:false, crosswalk:false, speed:15, bump:false, trees:false },
  bikestreet: { label:'Bike street — shared, green', lanes:2, laneW:2.9, median:0, medianKind:'none', bike:2.0, parking:0, sidewalk:2.4, shoulder:0, surface:'asphaltLight', center:'none', edgeLine:true, dash:false, crosswalk:true, speed:25, bump:true, trees:true },
};
export function resolvePreset(edge){ const base = PRESETS[edge.preset] || PRESETS.residential; return { ...base, ...(edge.overrides||{}) }; }
export function asphaltHalf(p){ return (p.lanes*p.laneW + (p.median||0))/2 + (p.bike||0) + (p.parking||0); }
export function totalHalf(p){ return asphaltHalf(p) + (p.sidewalk||0) + (p.shoulder||0); }
export const SURFACE_COLORS = { asphaltDark:0x2b2e33, asphalt:0x33373d, asphaltLight:0x3d4148, asphaltOld:0x4a453f, dirt:0x8a6a48, concrete:0x8d8d89 };
