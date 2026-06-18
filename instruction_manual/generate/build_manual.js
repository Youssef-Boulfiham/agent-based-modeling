const fs = require("fs");
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  AlignmentType, LevelFormat, HeadingLevel, BorderStyle, WidthType,
  ShadingType, PageBreak,
} = require("docx");

const ACCENT = "2D6CDF";
const cellBorder = { style: BorderStyle.SINGLE, size: 1, color: "CCCCCC" };
const borders = { top: cellBorder, bottom: cellBorder, left: cellBorder, right: cellBorder };
const cellMargins = { top: 60, bottom: 60, left: 120, right: 120 };

function headerCell(text, w) {
  return new TableCell({
    borders, width: { size: w, type: WidthType.DXA }, margins: cellMargins,
    shading: { fill: "D5E1F5", type: ShadingType.CLEAR },
    children: [new Paragraph({ children: [new TextRun({ text, bold: true, size: 18 })] })],
  });
}
function bodyCell(text, w, bold = false) {
  return new TableCell({
    borders, width: { size: w, type: WidthType.DXA }, margins: cellMargins,
    children: [new Paragraph({ children: [new TextRun({ text, bold, size: 18 })] })],
  });
}

const doc = new Document({
  styles: {
    default: { document: { run: { font: "Arial", size: 19 } } },
    paragraphStyles: [
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 30, bold: true, font: "Arial", color: "1A1A1A" },
        paragraph: { spacing: { before: 0, after: 80 }, outlineLevel: 0 } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 21, bold: true, font: "Arial", color: ACCENT },
        paragraph: { spacing: { before: 160, after: 60 }, outlineLevel: 1 } },
    ],
  },
  numbering: {
    config: [
      { reference: "steps",
        levels: [{ level: 0, format: LevelFormat.DECIMAL, text: "%1.", alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 460, hanging: 300 } } } }] },
      { reference: "bul",
        levels: [{ level: 0, format: LevelFormat.BULLET, text: "•", alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 460, hanging: 300 } } } }] },
    ],
  },
  sections: [{
    properties: {
      page: {
        size: { width: 11906, height: 16838 }, // A4
        margin: { top: 1080, right: 1080, bottom: 1080, left: 1080 },
      },
    },
    children: [
      new Paragraph({ heading: HeadingLevel.HEADING_1,
        children: [new TextRun("Agent Walking & Pathfinding — Instruction Manual")] }),
      new Paragraph({ spacing: { after: 120 }, children: [new TextRun({
        text: "Hoe een agent rationeel door de wereld beweegt. Dit een A4 beschrijft het volledige doel; geef het door om vanaf nul opnieuw te beginnen.",
        italics: true, size: 18, color: "555555" })] }),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("1. Doel in een zin")] }),
      new Paragraph({ spacing: { after: 60 }, children: [new TextRun(
        "Een externe functie bepaalt in welk domein de agent moet zijn. Elke step checkt de agent dat: is hij er nog niet, dan loopt hij via de gang naar dat domein (kortste pad als priority-queue, een cel per stap); is hij er wel, dan loopt hij wat rond (idle) of staat hij stil (working).")] }),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("2. Kernconcepten")] }),
      ...bulletRich([
        { text: "Domein stuurt het loopgedrag: ", bold: true },
        { text: "de ruimte waar de agent moet zijn. Wordt EXTERN gezet (andere module). De agent checkt elke step welk domein hij moet zijn — het doel kan elke stap veranderen. Geen minimale stabiliteit; dat is testcase-specifiek voor controleerbaarheid." },
      ]),
      ...bulletRich([
        { text: "Gang verbindt domeinen: ", bold: true },
        { text: "naar een ander domein loopt de agent ALTIJD via de gang. Muren blokkeren; het pad is een kortste route (BFS/A*) over begaanbare cellen (kamers + gangen), nooit dwars door een muur." },
      ]),
      ...bulletRich([
        { text: "Activiteit (3 soorten): ", bold: true },
        { text: "idle = niks te doen, loopt wat rond in zijn kamer; working = blijft staan waar hij is; move to domain = nog onderweg via de gang naar het doel-domein." },
      ]),
      ...bullet("Pad = priority-queue: het hele pad van huidige cel naar doel wordt vooraf berekend als een wachtrij van cellen (front = volgende stap). De agent pakt elke step een cel van de voorkant."),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("3. Pathfinding — harde regels")] }),
      ...numbered("Een stap per step: elke beweging gaat naar een buurcel. Chebyshev-afstand tot de vorige cel is altijd <= 1. De agent mag NOOIT springen van het ene punt op de map naar het andere."),
      ...numbered("Kortste route via de gang: het pad over begaanbare cellen is minimaal. Naar een ander domein loopt de agent door de gang, niet dwars door muren."),
      ...numbered("Vloeiend & volgbaar: opeenvolgende posities vormen een aaneengesloten lijn, zodat het gedrag voor een kijker rationeel en te volgen is."),
      ...numbered("Domein-check elke step: het doel-domein kan extern veranderd zijn. Zo ja, herplan en loop via de gang naar het nieuwe domein; zo nee, idle/working binnen het huidige domein."),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("4. De step-loop")] }),
      new Table({
        width: { size: 9746, type: WidthType.DXA },
        columnWidths: [2450, 7296],
        rows: [
          new TableRow({ tableHeader: true, children: [headerCell("Toestand", 2450), headerCell("Wat de agent doet", 7296)] }),
          new TableRow({ children: [bodyCell("check domein", 2450, true), bodyCell("Elke step: zit ik in het doel-domein? Het doel komt van buiten; kan elke step veranderen.", 7296)] }),
          new TableRow({ children: [bodyCell("move to domain", 2450, true), bodyCell("Niet in doel-domein -> plan/vervolg BFS-pad via de gang, loop een cel. Bij aankomst -> idle of working.", 7296)] }),
          new TableRow({ children: [bodyCell("idle", 2450, true), bodyCell("In doel-domein, niks te doen -> loop wat rond in de kamer (een cel per step).", 7296)] }),
          new TableRow({ children: [bodyCell("working", 2450, true), bodyCell("In doel-domein -> blijf staan waar je bent (geen beweging).", 7296)] }),
        ],
      }),

      new Paragraph({ heading: HeadingLevel.HEADING_2, children: [new TextRun("5. Klaar wanneer")] }),
      ...bullet("Elke beweging heeft stap-afstand 1 (nul jumps over de hele run)."),
      ...bullet("Reizen tussen domeinen lopen via de gang (kortste begaanbare route, niet door muren)."),
      ...bullet("Doel-domein wordt alleen extern gezet en wordt elke step gecheckt."),
      ...bullet("De drie activiteiten idle / working / move to domain gedragen zich correct."),

      new Paragraph({ spacing: { before: 140 }, border: { top: { style: BorderStyle.SINGLE, size: 6, color: ACCENT, space: 6 } },
        children: [new TextRun({ text: "Referentie-implementatie: testcases/html/walking_behaviour/index.html. Gevalideerd: 0 jumps, alle reizen via de gang.",
          size: 17, italics: true, color: "555555" })] }),

      // ── ARTEFACT ────────────────────────────────────────────────────────────
      new Paragraph({ children: [new PageBreak()] }),

      // Artefact header block
      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Artefact v1 — Verbatim Code", bold: true, size: 32, font: "Arial", color: "1F3864" })] }),
      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Commit: 32c49e0c54d71c423590b4696f599ba6f19b317e  —  2026-06-18  —  Branch: main", size: 17, font: "Courier New", color: "555555" })] }),
      new Paragraph({ spacing: { after: 40 }, children: [new TextRun({ text: "Paste sections 1–5 into a clean project to reproduce the approved walking behaviour exactly.", size: 17, italics: true, color: "555555" })] }),
      new Paragraph({ border: { bottom: { style: BorderStyle.SINGLE, size: 4, color: "2E75B6", space: 1 } }, spacing: { before: 0, after: 180 }, children: [] }),

      // Section 1
      ...artefactSection("1", "Env — Relevant Fields"),
      ...artefactNote("Activity struct  (src/include/Env.h)"),
      ...code([
        "struct Activity {",
        "    int domain = WalkGrid::CORRIDOR;",
        "    float probability = 0.0f;",
        "    std::vector<glm::vec2> positions;",
        "};",
      ]),
      ...artefactNote("Env class — walking-relevant members  (src/include/Env.h)"),
      ...code([
        "class Env {",
        "private:",
        "    float width, height, deltaTime;",
        "    int frameCount;",
        "    bool isRunning;",
        "    std::vector<Agent*> agents;",
        "    int maxAgents, activeAgents;",
        "    WalkGrid grid;",
        "    std::unordered_map<std::string, Activity> activities;",
        "    std::vector<std::string> activityNames;",
        "    void controlAgentDomains();",
        "    std::vector<int> domainList;               // domains available for assignment",
        "    std::unordered_map<int, int> nextChangeAt; // agentId -> frame of next change",
        "    ...",
        "};",
      ]),
      ...artefactNote("Agent class — walking-relevant members  (src/include/Agent.h)"),
      ...code([
        "class Agent {",
        "protected:",
        "    int id;",
        "    glm::vec2 position;",
        "    std::string activity, status;",
        "    std::vector<glm::vec2> path;   // waypoints (front = next step)",
        "    glm::vec2 positionCurrent;     // last cell stood on",
        "    int targetDomain;              // where agent MUST be (set externally)",
        "    std::string assignedActivity;  // idle or working (assigned on arrival)",
        "    class Env* world;",
        "    ...",
        "};",
      ]),
      ...artefactNote("WalkGrid struct  (src/include/Pathfinding.h)"),
      ...code([
        "struct WalkGrid {",
        "    static constexpr int BLOCKED  = -1;  // impassable",
        "    static constexpr int CORRIDOR =  0;  // shared walkable",
        "    int cols = 0, rows = 0, cellSize = 10;",
        "    std::vector<int> domains;  // cols*rows, row-major",
        "    bool inBounds(glm::ivec2 c) const {",
        "        return c.x >= 0 && c.y >= 0 && c.x < cols && c.y < rows;",
        "    }",
        "    int  domainAt(glm::ivec2 c) const { return domains[c.y * cols + c.x]; }",
        "    void setDomain(glm::ivec2 c, int d) { domains[c.y * cols + c.x] = d; }",
        "    glm::ivec2 toGrid(glm::vec2 w) const {",
        "        return glm::ivec2(static_cast<int>(w.x)/cellSize, static_cast<int>(w.y)/cellSize);",
        "    }",
        "    glm::vec2 toWorld(glm::ivec2 c) const {",
        "        return glm::vec2(c.x*cellSize + cellSize*0.5f, c.y*cellSize + cellSize*0.5f);",
        "    }",
        "};",
      ]),

      // Section 2
      ...artefactSection("2", "Simulation Loop — System::run()  (src/System.cpp)"),
      ...code([
        "void System::run() {",
        "    bool running = true;",
        "    while (running) {",
        "        handleEvents(running);",
        "        simulation->update(FRAME_TIME);  // stores deltaTime only",
        "        simulation->step();              // advances logic: domains + agents",
        "        render();",
        "        std::this_thread::sleep_for(std::chrono::milliseconds(16));",
        "    }",
        "}",
      ]),

      // Section 3
      ...artefactSection("3", "Env::step() — Frame Stepper  (src/Env.cpp)"),
      ...code([
        "void Env::step() {",
        "    if (!isRunning) return;",
        "    controlAgentDomains();  // external controller: assign/refresh target domains",
        "    updateAgents();         // calls agent->step() for every agent",
        "    activeAgents = agents.size();",
        "    frameCount++;",
        "}",
      ]),

      // Section 4
      ...artefactSection("4", "Agent::step() — Agent Logic  (src/Agent.cpp)"),
      ...code([
        "void Agent::step(float /*deltaTime*/) {",
        "    if (!world) return;",
        "    int currentRoom = world->roomOf(positionCurrent);",
        "    // NOT in target domain -> move_to_domain",
        "    if (currentRoom != targetDomain) {",
        "        activity = \"move to domain\";",
        "        if (path.empty()) routeToDomain(targetDomain);",
        "        if (!path.empty() && uniform01() < 0.6f) {",
        "            positionCurrent = path.front();",
        "            path.erase(path.begin());",
        "            position = positionCurrent;",
        "            if (world->roomOf(positionCurrent) == targetDomain) {",
        "                path.clear();",
        "                assignedActivity = world->findActivityInDomain(targetDomain);",
        "                if (assignedActivity.empty()) assignedActivity = \"idle\";",
        "            }",
        "        }",
        "        return;",
        "    }",
        "    // In target domain -> execute assigned activity",
        "    activity = assignedActivity;",
        "    if (activity == \"working\") { path.clear(); return; }",
        "    // idle: wander within current domain",
        "    if (path.empty()) path = bfs(positionCurrent, pickPosition());",
        "    if (!path.empty() && uniform01() < 0.6f) {",
        "        positionCurrent = path.front();",
        "        path.erase(path.begin());",
        "        position = positionCurrent;",
        "    }",
        "}",
      ]),

      // Section 5
      ...artefactSection("5", "Sub-functions — in call order"),

      ...artefactNote("5a. Env::controlAgentDomains()  (src/Env.cpp)"),
      ...code([
        "void Env::controlAgentDomains() {",
        "    if (domainList.empty()) return;",
        "    static std::uniform_int_distribution<int> jitter(0, 39);",
        "    std::uniform_int_distribution<int> pick(0, (int)domainList.size() - 1);",
        "    for (Agent* agent : agents) {",
        "        int aid = agent->getId();",
        "        auto it = nextChangeAt.find(aid);",
        "        if (it == nextChangeAt.end() || frameCount >= it->second) {",
        "            int current = agent->getTargetDomain(); int next;",
        "            do { next = domainList[pick(rng)]; }",
        "            while (next == current && domainList.size() > 1);",
        "            agent->setTargetDomain(next);",
        "            nextChangeAt[aid] = frameCount + 50 + jitter(rng);",
        "        }",
        "    }",
        "}",
      ]),
      ...artefactNote("5b. Env::updateAgents()  (src/Env.cpp)"),
      ...code([
        "void Env::updateAgents() {",
        "    for (Agent* agent : agents) agent->step(deltaTime);",
        "}",
      ]),
      ...artefactNote("5c. Env::roomOf()  (src/Env.cpp)"),
      ...code([
        "int Env::roomOf(glm::vec2 worldPos) const {",
        "    glm::ivec2 gridPos = grid.toGrid(worldPos);",
        "    if (!grid.inBounds(gridPos)) return WalkGrid::BLOCKED;",
        "    return grid.domainAt(gridPos);",
        "}",
      ]),
      ...artefactNote("5d. Agent::routeToDomain()  (src/Agent.cpp)"),
      ...code([
        "void Agent::routeToDomain(int domain) {",
        "    glm::vec2 goal = world->domainCenter(domain);",
        "    const WalkGrid& grid = world->getWalkGrid();",
        "    std::vector<int> allowed = {domain};",
        "    Path p = Pathfinding::findPath(positionCurrent, goal, grid, allowed);",
        "    if (p.found) path.insert(path.end(), p.waypoints.begin(), p.waypoints.end());",
        "    else path.clear();",
        "}",
      ]),
      ...artefactNote("5e. Env::domainCenter()  (src/Env.cpp)"),
      ...code([
        "glm::vec2 Env::domainCenter(int domain) const {",
        "    float sumX = 0, sumY = 0; int count = 0;",
        "    for (const auto& pair : activities) {",
        "        if (pair.second.domain == domain)",
        "            for (const auto& pos : pair.second.positions)",
        "                { sumX += pos.x; sumY += pos.y; count++; }",
        "    }",
        "    if (count == 0) return glm::vec2(0, 0);",
        "    return glm::vec2(sumX / count, sumY / count);",
        "}",
      ]),
      ...artefactNote("5f. Env::findActivityInDomain()  (src/Env.cpp)"),
      ...code([
        "std::string Env::findActivityInDomain(int domain) const {",
        "    for (const auto& name : activityNames)",
        "        if (activities.at(name).domain == domain) return name;",
        "    return \"\";",
        "}",
      ]),
      ...artefactNote("5g. Agent::bfs()  (src/Agent.cpp)"),
      ...code([
        "std::vector<glm::vec2> Agent::bfs(glm::vec2 start, glm::vec2 goal) const {",
        "    if (!world) return {};",
        "    const WalkGrid& grid = world->getWalkGrid();",
        "    int currentDomain = world->roomOf(start);",
        "    Path p = Pathfinding::findPath(start, goal, grid, {currentDomain});",
        "    return p.found ? p.waypoints : std::vector<glm::vec2>();",
        "}",
      ]),
      ...artefactNote("5h. Agent::pickPosition()  (src/Agent.cpp)"),
      ...code([
        "glm::vec2 Agent::pickPosition() const {",
        "    const Activity* act = world->findActivity(activity);",
        "    if (!act || act->positions.empty()) return positionCurrent;",
        "    if (activity == \"idle\") return randChoice(act->positions);",
        "    std::vector<glm::vec2> sorted = act->positions;",
        "    std::sort(sorted.begin(), sorted.end(), [this](const glm::vec2& a, const glm::vec2& b) {",
        "        float da = std::abs(a.x-positionCurrent.x)+std::abs(a.y-positionCurrent.y);",
        "        float db = std::abs(b.x-positionCurrent.x)+std::abs(b.y-positionCurrent.y);",
        "        return da < db; });",
        "    size_t half = sorted.size() / 2;",
        "    if (half > 0 && uniform01() < 0.75f)",
        "        return sorted[randInt(0, (int)half - 1)];",
        "    return randChoice(sorted);",
        "}",
      ]),
      ...artefactNote("5i. Agent::routeTo()  (src/Agent.cpp)"),
      ...code([
        "void Agent::routeTo(glm::vec2 goal) {",
        "    Path p = Pathfinding::findPath(positionCurrent, goal, world->getWalkGrid(), allowedDomains());",
        "    if (p.found) path.insert(path.end(), p.waypoints.begin(), p.waypoints.end());",
        "    else path.assign(randInt(5, 15), positionCurrent);",
        "}",
      ]),
      ...artefactNote("5j. Agent::allowedDomains()  (src/Agent.cpp)"),
      ...code([
        "std::vector<int> Agent::allowedDomains() const {",
        "    std::vector<int> allowed;",
        "    const Activity* act = world->findActivity(activity);",
        "    if (act) allowed.push_back(act->domain);",
        "    return allowed;",
        "}",
      ]),
      ...artefactNote("5k. Pathfinding::findPath() — WalkGrid overload  (src/Pathfinding.cpp)"),
      ...code([
        "Path Pathfinding::findPath(glm::vec2 start, glm::vec2 goal,",
        "                           const WalkGrid& grid,",
        "                           const std::vector<int>& allowedDomains) {",
        "    Path path;",
        "    if (grid.cols == 0 || grid.rows == 0) return path;",
        "    auto passable = [&](glm::ivec2 c) -> bool {",
        "        if (!grid.inBounds(c)) return false;",
        "        int d = grid.domainAt(c);",
        "        if (d == WalkGrid::BLOCKED) return false;",
        "        if (allowedDomains.empty() || d == WalkGrid::CORRIDOR) return true;",
        "        return std::find(allowedDomains.begin(), allowedDomains.end(), d) != allowedDomains.end();",
        "    };",
        "    glm::ivec2 startGrid = grid.toGrid(start), goalGrid = grid.toGrid(goal);",
        "    if (!grid.inBounds(startGrid) || !grid.inBounds(goalGrid)) return path;",
        "    if (startGrid == goalGrid) { path.waypoints.push_back(goal); path.found=true; return path; }",
        "    std::vector<Node*> openSet, allNodes;",
        "    std::vector<glm::ivec2> closedSet;",
        "    Node* sn = new Node(startGrid); sn->g=0; sn->h=heuristic(startGrid,goalGrid);",
        "    openSet.push_back(sn); allNodes.push_back(sn);",
        "    const glm::ivec2 dirs[] = {{0,1},{1,0},{0,-1},{-1,0},{1,1},{1,-1},{-1,1},{-1,-1}};",
        "    Node* cur = nullptr;",
        "    while (!openSet.empty()) {",
        "        auto minIt = std::min_element(openSet.begin(), openSet.end(),",
        "            [](Node* a, Node* b){ return a->f() < b->f(); });",
        "        cur = *minIt; openSet.erase(minIt);",
        "        if (cur->pos == goalGrid) {",
        "            path.found = true;",
        "            while (cur) { path.waypoints.insert(path.waypoints.begin(), grid.toWorld(cur->pos)); cur=cur->parent; }",
        "            if (!path.waypoints.empty()) path.waypoints.back() = goal;",
        "            for (auto n : allNodes) delete n; return path;",
        "        }",
        "        closedSet.push_back(cur->pos);",
        "        for (const auto& dir : dirs) {",
        "            glm::ivec2 nb = cur->pos + dir;",
        "            if (!passable(nb)) continue;",
        "            if (dir.x!=0 && dir.y!=0 &&",
        "                !passable({cur->pos.x+dir.x, cur->pos.y}) &&",
        "                !passable({cur->pos.x, cur->pos.y+dir.y})) continue;",
        "            if (std::find(closedSet.begin(),closedSet.end(),nb)!=closedSet.end()) continue;",
        "            float mv = (dir.x!=0&&dir.y!=0)?1.414f:1.0f;",
        "            float tG = cur->g + mv;",
        "            auto ex = std::find_if(openSet.begin(),openSet.end(),[nb](Node* n){return n->pos==nb;});",
        "            if (ex==openSet.end()) {",
        "                Node* nn=new Node(nb); nn->g=tG; nn->h=heuristic(nb,goalGrid); nn->parent=cur;",
        "                openSet.push_back(nn); allNodes.push_back(nn);",
        "            } else if (tG < (*ex)->g) { (*ex)->g=tG; (*ex)->parent=cur; }",
        "        }",
        "    }",
        "    path.found = false; for (auto n : allNodes) delete n; return path;",
        "}",
      ]),

      // End note
      new Paragraph({ spacing: { before: 180 },
        border: { top: { style: BorderStyle.SINGLE, size: 4, color: "2E75B6", space: 1 } },
        children: [new TextRun({ text: "End of artefact. Pasting sections 1–5 into a clean project reproduces the approved walking behaviour exactly.",
          size: 17, italics: true, color: "555555" })] }),
    ],
  }],
});

function bullet(text) {
  return [new Paragraph({ numbering: { reference: "bul", level: 0 }, spacing: { after: 40 },
    children: [new TextRun({ text, size: 19 })] })];
}
function bulletRich(runs) {
  return [new Paragraph({ numbering: { reference: "bul", level: 0 }, spacing: { after: 40 },
    children: runs.map(r => new TextRun({ text: r.text, bold: !!r.bold, size: 19 })) })];
}
function numbered(text) {
  return [new Paragraph({ numbering: { reference: "steps", level: 0 }, spacing: { after: 40 },
    children: [new TextRun({ text, size: 19 })] })];
}

function artefactSection(num, title) {
  const nb = { style: BorderStyle.NONE };
  const noBorder = { top: nb, bottom: nb, left: nb, right: nb };
  return [
    new Paragraph({ spacing: { before: 200, after: 0 }, children: [] }),
    new Table({
      width: { size: 9746, type: WidthType.DXA },
      columnWidths: [560, 9186],
      borders: { top: nb, bottom: nb, left: nb, right: nb, insideH: nb, insideV: nb },
      rows: [new TableRow({ children: [
        new TableCell({
          borders: noBorder, width: { size: 560, type: WidthType.DXA },
          shading: { fill: "2E75B6", type: ShadingType.CLEAR },
          margins: { top: 60, bottom: 60, left: 80, right: 80 },
          children: [new Paragraph({ alignment: AlignmentType.CENTER, spacing: { before: 0, after: 0 },
            children: [new TextRun({ text: num, bold: true, size: 18, font: "Arial", color: "FFFFFF" })] })]
        }),
        new TableCell({
          borders: noBorder, width: { size: 9186, type: WidthType.DXA },
          shading: { fill: "EEF2FA", type: ShadingType.CLEAR },
          margins: { top: 60, bottom: 60, left: 140, right: 80 },
          children: [new Paragraph({ spacing: { before: 0, after: 0 },
            children: [new TextRun({ text: title, bold: true, size: 19, font: "Arial", color: "1F3864" })] })]
        }),
      ]})]
    }),
    new Paragraph({ spacing: { before: 0, after: 80 }, children: [] }),
  ];
}

function artefactNote(text) {
  return [new Paragraph({ spacing: { before: 120, after: 40 },
    children: [new TextRun({ text, bold: true, size: 17, font: "Arial", color: "2E75B6" })] })];
}

function code(lines) {
  const nb = { style: BorderStyle.NONE };
  const noBorder = { top: nb, bottom: nb, left: nb, right: nb };
  const rows = lines.map(line =>
    new TableRow({ children: [new TableCell({
      borders: noBorder, width: { size: 9746, type: WidthType.DXA },
      shading: { fill: "1A1A2E", type: ShadingType.CLEAR },
      margins: { top: 18, bottom: 18, left: 160, right: 80 },
      children: [new Paragraph({ spacing: { before: 0, after: 0 },
        children: [new TextRun({ text: line, font: "Courier New", size: 15, color: "E8E8E8" })] })]
    })] })
  );
  return [new Table({
    width: { size: 9746, type: WidthType.DXA }, columnWidths: [9746],
    borders: {
      top:    { style: BorderStyle.SINGLE, size: 2, color: "333355" },
      bottom: { style: BorderStyle.SINGLE, size: 2, color: "333355" },
      left:   { style: BorderStyle.SINGLE, size: 8, color: "2E75B6" },
      right:  { style: BorderStyle.SINGLE, size: 2, color: "333355" },
      insideH: nb, insideV: nb,
    },
    rows,
  }), new Paragraph({ spacing: { before: 0, after: 60 }, children: [] })];
}

Packer.toBuffer(doc).then((buf) => {
  fs.writeFileSync("Agent_Walking_Pathfinding_Manual.docx", buf);
  console.log("written Agent_Walking_Pathfinding_Manual.docx");
});
