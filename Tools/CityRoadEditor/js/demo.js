// One-click full project: countryside -> suburbs -> city -> offroad,
// river + lake, bridges, rail cutting through, elevated monorail loop.
import { Project } from './core.js';

export function buildDemoCity(){
  const p = new Project(); p.name = 'Slate City — countryside to city to offroad';
  const R = (pts, preset) => p.addRoad(pts, preset);

  // ---- Ring road (organic, NOT a grid): closed-ish loop via chained arcs ----
  const ring = [];
  const cx=60, cz=90, N=14;
  for(let i=0;i<=N;i++){ const a=i/N*Math.PI*2; const wob = 1 + 0.10*Math.sin(a*3+1.2);
    ring.push([cx+Math.cos(a)*252*wob, cz+Math.sin(a)*172*(1+0.08*Math.cos(a*2))]); }
  for(let i=0;i<ring.length-1;i++) R([ring[i], ring[i+1]], i%3===0?'boulevard':'arterial');

  // ---- Downtown organic avenues (diagonals & curves, Y / X junctions) ----
  R([[-150,-30],[-40,45],[80,120],[210,205]], 'boulevard');                    // main diagonal
  R([[-95,235],[-10,155],[70,95],[150,20],[225,-55]], 'arterial');              // sweeping cross avenue
  R([[-40,-75],[30,-5],[95,60],[150,140],[185,235]], 'collector');              // curved N-S street
  R([[-170,120],[-60,105],[55,130],[170,115],[285,150]], 'collector');          // curved E-W street
  R([[-120,20],[-30,-35],[70,-60]], 'street');
  R([[120,-40],[170,40],[205,130]], 'street');
  R([[-60,190],[30,215],[130,205]], 'residential');
  R([[10,10],[60,55],[115,95]], 'avenue1way');
  R([[-150,180],[-80,120],[-20,70]], 'bikestreet');

  // ---- Radials out of the ring (each a different road class) ----
  R([[60,-82],[48,-150],[42,-225],[30,-300],[18,-372]], 'highway');             // north, bridges the river
  R([[300,20],[390,-15],[470,-45],[545,-70]], 'highway');                       // NE highway
  R([[312,95],[400,80],[480,95],[548,70]], 'arterial');                         // east arterial
  R([[85,262],[110,320],[150,378]], 'collector');                               // south collector
  R([[-192,75],[-285,55],[-380,70],[-470,45],[-548,60]], 'arterial');           // west boulevard out
  R([[-105,-45],[-190,-110],[-265,-165]], 'collector');                         // NW, bridges river
  R([[240,215],[320,265],[395,300]], 'residential');                            // SE suburbs

  // ---- Suburban organic branches & cul-de-sacs (Y junctions, no grid) ----
  R([[312,95],[345,160],[335,225],[290,265]], 'residential');
  R([[345,160],[410,185],[455,160]], 'residential');
  R([[-192,75],[-235,150],[-225,225],[-170,270]], 'residential');
  R([[-235,150],[-310,175],[-360,150]], 'street');
  R([[110,320],[55,345],[5,330]], 'street');
  R([[240,215],[215,285],[240,345]], 'residential');
  R([[-20,70],[-95,95],[-150,70]], 'street');

  // ---- Countryside ----
  R([[-265,-165],[-360,-205],[-455,-185],[-548,-215]], 'country');              // NW country road
  R([[30,-300],[-60,-335],[-160,-320],[-265,-350]], 'country');                 // north farms road
  R([[150,378],[250,350],[340,365],[430,335],[520,355]], 'country');            // south village road
  R([[340,365],[355,300],[395,300]], 'residential');                            // village Y
  R([[430,335],[465,275]], 'street');
  R([[-470,45],[-500,-40],[-480,-120],[-455,-185]], 'country');                 // west valley road, river bridge
  R([[470,-45],[480,40],[480,95]], 'country');

  // ---- Bridges are automatic where these cross the river / lake inlet ----
  R([[-105,-45],[-150,-15],[-215,5]], 'collector');                             // extra river crossing W

  // ---- Offroad dirt: lakeside + hill climb ----
  R([[-225,225],[-275,205],[-315,235]], 'offroad');                             // to lake shore
  R([[-360,-205],[-405,-260],[-375,-305],[-320,-295]], 'offroad');              // hill trail loop spur
  R([[250,350],[300,320],[360,325]], 'offroad');                                // farm track
  R([[-275,205],[-250,150],[-235,150]], 'offroad');

  // ---- Rail main line cutting straight through everything (SW -> NE) ----
  p.tracks.push({ kind:'rail', points:[[-560,330],[-380,235],[-210,150],[-40,60],[130,-25],[300,-110],[430,-185],[560,-255]] });
  // ---- freight spur ----
  p.tracks.push({ kind:'rail', points:[[130,-25],[215,45],[290,120]] });
  // ---- Elevated monorail loop around downtown ----
  const loop=[]; for(let i=0;i<=12;i++){ const a=i/12*Math.PI*2; loop.push([60+Math.cos(a)*150, 90+Math.sin(a)*98]); }
  p.tracks.push({ kind:'monorail', points:loop });

  p.gcNodes();
  return p;
}
