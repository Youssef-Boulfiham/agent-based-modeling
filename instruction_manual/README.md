# Instruction Manual — Walking Behaviour (VETO)

Eén A4 dat het volledige doel van het loop-/walking-gedrag beschrijft. Dit is de
**bron van waarheid** — alle implementaties (testcases, ABM code) moeten dit exact
volgen.

**Wijzigingen aan loopgedrag vereisen expliciete toestemming. Vraag eerst voordat
je iets aanpast.**

Inhoud: pathfinding, domeinen, corridors, de drie activiteiten (idle/working/move
to domain), en het pad als priority-queue dat je afloopt tot je onderbroken wordt
of niets te doen hebt.

## Bestanden

- **Agent_Walking_Pathfinding_Manual.docx** — het 1-A4 manual (de bron van waarheid).
- **build_manual.js** — generator-script dat de .docx bouwt.

## Regenereren

```bash
cd instruction_manual
npm install docx          # eenmalig, lokaal
node build_manual.js      # schrijft Agent_Walking_Pathfinding_Manual.docx
```

## Referentie-implementatie

Het bewezen prototype staat in `testcases/html/walking_behaviour/index.html`.
Gevalideerd: 0 jumps, 0 omwegen, geen activiteit-koppeling. Loopgedrag is puur
domein-gestuurd; activiteit is een losse, nog ongedefinieerde variabele.
Volgende stap: implementeren in het echte ABM-model (`src/Agent.cpp`).
