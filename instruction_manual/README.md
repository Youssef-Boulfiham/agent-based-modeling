# Instruction Manual

Eén A4 dat het volledige doel van het loop-/walking-gedrag beschrijft. Geef dit
A4 door om vanaf nul opnieuw te beginnen — het bevat wat we willen bereiken:
pathfinding, domeinen=activiteiten, de stepper die logisch kiest, en het pad als
priority-queue dat je afloopt tot je onderbroken wordt of niets te doen hebt.

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
