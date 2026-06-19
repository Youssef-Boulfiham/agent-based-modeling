# LLM Control Sandbox (pygame)

Proving ground for the multi-LLM control layer **before** it goes into the C++ ABM.
Sandbox-first per the Programming Agent Workflow (Stage 07/08).

## What it does

- **Gathers all LLMs** behind one uniform API (`llm_manager.LLMManager`):
  - **Claude (Anthropic)** — headless `claude -p`, Max subscription (OAuth token).
  - **GPT (OpenAI)** — `codex exec`, ChatGPT Plus/Pro login (no API token).
  - **Local (Ollama)** — HTTP `:11434`, **fully offline, no internet / tokens / accounts**.
- Pick **provider** + **sub-model** (haiku/sonnet/opus · gpt models · ollama tags).
- One agent **speaks through** the chosen LLM — **async**, the UI never blocks.
- Per-provider **availability** status.
- **SPEED TEST** across every available LLM, ranked fastest-first (with tok/s).

## Files

| File | Role | C++ port target |
|------|------|-----------------|
| `llm_manager.py` | the LLM control module (providers, async, speed test). **No pygame.** | `src/LLMClient.{h,cpp}` |
| `app.py` | pygame UI on top of the manager | SettingsWindow LLM panel + worker thread |
| `selftest.py` | headless proof of plumbing (real calls) | — |
| `smoke_ui.py` | headless UI construct/draw check | — |

## Run

```bash
cd testcases/pygame/llm_control
python3 selftest.py     # headless: availability + one call per provider + speed test
python3 app.py          # the visual sandbox
```

## Claude setup (one-time, required for the Claude path)

Headless `claude -p` will NOT use your Max plan via the keychain login — it 403s
("organization does not have access"). Mint a long-lived OAuth token instead:

```bash
claude setup-token          # opens browser, logs in with Max, prints a token (~1yr)
```

Then make it available to the sandbox (token is a **secret** — never commit it):

```bash
# option A: shell env
export CLAUDE_CODE_OAUTH_TOKEN="<token>"

# option B: local secrets file (gitignored)
echo 'CLAUDE_CODE_OAUTH_TOKEN=sk-ant-oat01-XXXX' > secrets.local
```

> Paste the **raw** token (starts `sk-ant-oat01-`). Do **not** wrap it in `< >` —
> literal angle brackets corrupt the bearer token (401 "Invalid bearer token").

GPT and Local need **no** setup if `codex login` is done and `ollama serve` is running.

## Async model (why this matters for the ABM)

Every call runs on a `ThreadPoolExecutor` worker; the UI polls the `Future` each
frame and applies the result on the main thread. This is the exact pattern the
C++ ABM must use — a worker thread + a result drained inside `Env::step()` — so
the 12 Hz fixed-timestep loop never stalls on a 0.5–7 s LLM call.

## Notes / gotchas (verified on this machine)

- Local `llama3.2:3b` ≈ **340 ms warm**, ~40 tok/s. First call is **cold** (model load).
- `codex exec` is a heavy agent harness → ~5–7 s even for one word. Subscription-billed.
- Claude env scrub: the manager strips inherited `CLAUDE_CODE_*` / `ANTHROPIC_BASE_URL`
  so a spawned `claude` uses its own OAuth, not a parent Claude Code's gateway.
- Per the ABM veto: this layer does **not** touch walking/pathfinding. It is a
  conversational/assistant layer attached to one agent (later: all agents).
