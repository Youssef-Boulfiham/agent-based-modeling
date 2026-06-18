# Documentation Index

Complete guide to ABM project documentation.

**READ THIS if you're new to the project. Then read the README in the folder you're working on.**

## Golden Rule

Before working on any folder, **read its README first.** READMEs explain purpose, structure, conventions, and constraints specific to that area. This prevents mistakes and ensures consistency.

## Core Documentation

### Project Level

| File | Purpose |
|------|---------|
| **readme.md** | Project overview, quick start, high-level architecture |
| **CLAUDE.md** | Development guidelines, code layout rules, mandatory structure |
| **DOCUMENTATION.md** | This file — navigation guide for all docs |

**Start here if:** You're new to the project, setting up, or want architectural overview.

### Build System

| File | Purpose |
|------|---------|
| **build/README.md** | CMake builds, compilation, binary output |

**Start here if:** Building the app, configuring cmake, debugging build issues.

### Source Code

| File | Purpose |
|------|---------|
| **src/README.md** | Source organization, components overview, include conventions |
| **src/include/README.md** | Header files, forward declarations, no-implementation rule |

**Start here if:** Adding code, creating new components, writing headers.

### Data Management

| File | Purpose |
|------|---------|
| **data/README.md** | Data directory structure, input/output/temp/logs overview |
| **data/input/README.md** | User config files, simulation parameters |
| **data/output/README.md** | Final results, metrics, processed data |
| **data/temp/README.md** | Working files, caches, temporary data (safe to delete) |
| **data/logs/README.md** | Experiment history, iteration notes, analysis |

**Start here if:** Managing data, understanding file organization, setting up experiments.

### Testing

| File | Purpose |
|------|---------|
| **testcases/README.md** | Testing infrastructure, sandbox philosophy, adding test suites |
| **testcases/sandbox/README.md** | Sandbox mode guide, building tests, extending with scenarios |

**Start here if:** Writing tests, debugging with sandbox, validating algorithms.

## How to Use This Guide

### You need to...

**...understand the project structure?**
→ Read `readme.md` → `CLAUDE.md`

**...add new code to src/?**
→ Read `CLAUDE.md` → `src/README.md` → `src/include/README.md`

**...work with data files?**
→ Read `data/README.md` → specific data subfolder (`input/`, `output/`, etc.)

**...write tests?**
→ Read `testcases/README.md` → `testcases/sandbox/README.md`

**...build the application?**
→ Read `build/README.md` → `CLAUDE.md` (for dependencies)

**...debug a specific folder?**
→ Find relevant README in navigation above → look for "Debugging" section

## Documentation Philosophy

Each README explains:

1. **Purpose** — Why does this folder exist? What goes in it?
2. **Structure** — How are files organized?
3. **Conventions** — What rules must be followed?
4. **How to use** — Workflows, examples, common operations
5. **Integration** — How does this folder connect to others?
6. **Debugging** — Common issues and solutions

READMEs are living documents. Update them when:
- Adding new folders or files
- Changing conventions
- Discovering new best practices
- Documenting gotchas you find

## Quick Reference

```
Project Root/
├── readme.md                  ← START HERE (project overview)
├── CLAUDE.md                  ← Mandatory rules & guidelines
├── DOCUMENTATION.md           ← This file (navigation)
│
├── build/README.md            ← CMake & compilation
├── src/README.md              ← Source code organization
├── src/include/README.md      ← Header file conventions
│
├── data/README.md             ← Data directory structure
│   ├── input/README.md        ← User configuration
│   ├── output/README.md       ← Final results
│   ├── temp/README.md         ← Working files (deletable)
│   └── logs/README.md         ← Experiment history
│
└── testcases/README.md        ← Testing infrastructure
    └── sandbox/README.md      ← Sandbox mode guide
```

## Before & After Checklist

### Before Starting Work

- [ ] Read readme.md (project overview)
- [ ] Read CLAUDE.md (mandatory rules)
- [ ] Read README.md in the folder you're working on
- [ ] Understand the structure & conventions in that folder

### After Completing Work

- [ ] Did you create new folders? Add a README.md to each.
- [ ] Did you change conventions? Update relevant READMEs.
- [ ] Did you discover a gotcha? Document it in the README.
- [ ] Are all files in correct locations per CLAUDE.md? Verify.
- [ ] Did sandbox tests pass? (if relevant)

## Special Cases

### Adding a New Component to src/

1. Read `CLAUDE.md` (code layout rules)
2. Read `src/README.md` (component organization)
3. Read `src/include/README.md` (header conventions)
4. Create `src/include/MyComponent.h`
5. Create `src/MyComponent.cpp`
6. Update `src/README.md` with new component entry
7. Update `CLAUDE.md` components table if major component

### Adding a New Test Suite

1. Read `testcases/README.md` (testing philosophy)
2. Create `testcases/my_test/README.md` explaining what you test
3. Create test executable & build script
4. Update `testcases/README.md` with new suite in structure

### Modifying Data Workflow

1. Read `data/README.md` (data lifecycle)
2. Read specific folder you're modifying (`input/`, `output/`, etc.)
3. Ensure changes follow conventions
4. Update relevant README if adding new file types or patterns

## Questions?

If you can't find an answer:

1. **Search the READMEs** — Most conventions are documented
2. **Check CLAUDE.md** — Mandatory rules are there
3. **Review existing code** — Patterns follow documented conventions
4. **Read sandbox/ examples** — Sandbox is a working example of isolated testing
5. **Check git log** — Commit messages explain why decisions were made

## Maintenance

Update docs when:
- **Adding folders** — Create README
- **Changing structure** — Update parent folder's README
- **Finding bugs in docs** — Fix immediately
- **Discovering gotchas** — Document in relevant README
- **Establishing new conventions** — Update CLAUDE.md and relevant READMEs

Outdated docs are worse than no docs. Keep them current.
