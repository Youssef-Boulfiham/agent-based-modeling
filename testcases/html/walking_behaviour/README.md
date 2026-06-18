# Walking Behaviour — domein-gestuurd looppad via gangen

Eén self-contained HTML/JS testcase (sim **en** visualisatie in `index.html`) die
het volledige loopgedrag aantoont. Voegt de vroegere `walking_behaviour` en
`domain_extraction` testcases samen — beide gingen over loopgedrag.

**Voor je iets wijzigt, lees dit bestand.**

## Concept

- **Domein** = de ruimte waar de agent MOET zijn. Wordt door een **externe
  functie** gezet (`externalDomainController`), niet door de agent zelf. In het
  echte ABM doet een andere module dit; hier wisselt het **minimaal elke 50
  steps**. De agent **checkt elke step** in welk domein hij moet zijn.
- **Gang (corridor)** = domeinen zijn verbonden door gangen. Naar een ander
  domein loopt de agent **altijd via de gang** — muren blokkeren. Het pad is een
  BFS over begaanbare cellen (kamers + gangen), nooit dwars door een muur.
- **Activiteit** = wat de agent nu doet. Drie soorten:
  | activiteit | gedrag |
  |---|---|
  | **idle** | niks te doen → loopt wat rond binnen zijn kamer |
  | **working** | blijft staan waar hij is |
  | **move to domain** | nog onderweg naar het domein waar hij moet zijn |

## Loop per step

1. **Externe controller** kan het doel-domein zetten (elke step gecheckt; in de
   testcase ≥ 50 steps stabiel).
2. **Check:** zit ik al in het doel-domein?
   - **Nee** → activiteit `move to domain`; plan/vervolg BFS-pad via de gang;
     loop één cel. Bij aankomst → kies `idle` of `working`.
   - **Ja** → activiteit = toegewezen `idle`/`working`.
     - `working`: blijf staan (lege queue).
     - `idle`: loop wat rond — kies een nabije cel in de kamer, loop één cel.

## Harde regels

1. Pad is een priority-queue van cellen; één cel per step.
2. Elke move ≤ 1 cel ver (Chebyshev ≤ 1). Nooit springen. HUD telt `violations`
   en toont `stap Δ` (rood bij > 1).
3. Kortste route over begaanbare cellen, via de gang.
4. Doel-domein verandert alleen extern; verandert het mid-route, dan herplant de
   agent en loopt via de gang naar het nieuwe domein.

## Draaien

Open `index.html` direct, of serveer de map:

```bash
python3 -m http.server 8732 --directory testcases/html/walking_behaviour
# open http://localhost:8732/index.html
```

Of gebruik de `walking-behaviour` launch config (`.claude/launch.json`).

**HUD:** moet in / nu in (kamer of `gang`) / activiteit / queue / stap Δ / since
(steps sinds laatste domein-wissel) / violations.
**Controls:** pause, speed x1–x4, reseed, restart.

## Pass-criteria (headless te checken in de console)

```js
reset(2024);
let jumps=0, changes=[], last=0;
for(let i=0;i<8000;i++){const b={...agent.pos},bt=agent.targetDomain;tick();
  if(Math.max(Math.abs(agent.pos.x-b.x),Math.abs(agent.pos.y-b.y))>1)jumps++;
  if(agent.targetDomain!==bt){changes.push(i-last);last=i;}}
console.log('violations',stats.violations,'jumps',jumps,'minGap',Math.min(...changes));
// verwacht: violations 0, jumps 0, minGap >= 50
```

Gevalideerd: 0 violations, 0 jumps, alle 4 kamers verbonden, alle reizen tussen
domeinen lopen via de gang, domein-wissel min. 50 steps, alle 3 activiteiten
komen voor.

## Naar het echte ABM

- Domein-check elke step blijft; alleen een externe functie zet het doel-domein.
- Routing tussen domeinen **via de gang** (BFS/A* over begaanbare cellen).
- Drie activiteiten idle / working / move to domain als loop-state.

**Specificatie staat in `instruction_manual/Agent_Walking_Pathfinding_Manual.docx`.**
Dit is de bron van waarheid. Wijzigingen aan loopgedrag vereisen expliciete toestemming
— vraag eerst voordat je dit aanpast.
