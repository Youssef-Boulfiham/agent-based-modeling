"""
llm_manager.py  --  Unified LLM control layer for the ABM sandbox.

ONE place that "gathers all the LLMs" and exposes a single, uniform API:

    mgr = LLMManager()
    res = mgr.ask("claude", "haiku", "Say hello")     # blocking
    fut = mgr.ask_async("local", "llama3.2:3b", "...") # non-blocking (Future)
    ranking = mgr.speed_test("Reply PONG")             # fastest-first

Design goals (kept deliberately simple so this ports 1:1 to a C++ module
`src/LLMClient.{h,cpp}` later, per the Programming Agent Workflow):

  * Provider abstraction      -> Provider subclasses, one per backend.
  * Subprocess / HTTP only    -> no paid SDKs, no API-token accounts required.
  * Subscription / offline    -> Claude(Max OAuth), GPT(ChatGPT login via codex),
                                 Local(Ollama, fully offline, no internet/tokens).
  * Async boundary            -> ThreadPoolExecutor == the C++ worker-thread +
                                 result-queue pattern. The sim/UI never blocks.
  * Normalized result         -> every backend returns an LLMResult.

Nothing here imports pygame; the UI (app.py) sits on top of this.
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import time
import urllib.error
import urllib.request
from concurrent.futures import Future, ThreadPoolExecutor
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

HERE = Path(__file__).resolve().parent

# --------------------------------------------------------------------------- #
# Secrets / config loading (never commit real secrets; secrets.local is .gitignored)
# --------------------------------------------------------------------------- #


def _load_secrets() -> dict:
    """Read KEY=VALUE lines from secrets.local (gitignored). Falls back to env."""
    secrets = {}
    f = HERE / "secrets.local"
    if f.exists():
        for line in f.read_text().splitlines():
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, _, v = line.partition("=")
            secrets[k.strip()] = v.strip().strip('"').strip("'")
    return secrets


_SECRETS = _load_secrets()


def _secret(name: str) -> Optional[str]:
    return _SECRETS.get(name) or os.environ.get(name)


# --------------------------------------------------------------------------- #
# Normalized result
# --------------------------------------------------------------------------- #


@dataclass
class LLMResult:
    provider: str
    model: str
    ok: bool
    text: str = ""
    error: str = ""
    latency_ms: float = 0.0           # wall-clock total (spawn -> answer)
    ttft_ms: Optional[float] = None   # time to first token, when known
    tokens_out: Optional[int] = None
    tok_per_s: Optional[float] = None
    raw: dict = field(default_factory=dict)

    def short(self) -> str:
        if not self.ok:
            return f"[{self.provider}/{self.model}] ERROR: {self.error}"
        tps = f", {self.tok_per_s:.1f} tok/s" if self.tok_per_s else ""
        return f"[{self.provider}/{self.model}] {self.latency_ms:.0f} ms{tps}: {self.text[:80]}"


# --------------------------------------------------------------------------- #
# Provider base
# --------------------------------------------------------------------------- #


class Provider:
    name = "base"
    display = "Base"
    kind = "cloud"   # "cloud" | "local"

    def models(self) -> list[str]:
        raise NotImplementedError

    def default_model(self) -> str:
        m = self.models()
        return m[0] if m else ""

    def available(self) -> tuple[bool, str]:
        """(ready, human-readable status). Cheap checks only."""
        raise NotImplementedError

    def ask(self, model: str, prompt: str, *, max_tokens: int = 64,
            timeout: float = 120.0) -> LLMResult:
        raise NotImplementedError

    # shared helper
    def _run(self, cmd: list[str], *, env=None, stdin_devnull=True,
             timeout: float = 120.0) -> tuple[int, str, str, float]:
        t0 = time.perf_counter()
        proc = subprocess.run(
            cmd,
            env=env,
            stdin=subprocess.DEVNULL if stdin_devnull else None,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
        )
        dt = (time.perf_counter() - t0) * 1000.0
        return proc.returncode, proc.stdout, proc.stderr, dt


# --------------------------------------------------------------------------- #
# Claude  (Anthropic) -- headless `claude -p`, Max subscription via OAuth token
# --------------------------------------------------------------------------- #


class ClaudeProvider(Provider):
    name = "claude"
    display = "Claude (Anthropic)"
    kind = "cloud"

    # aliases accepted by `claude --model`
    MODELS = ["haiku", "sonnet", "opus"]

    # env vars that, when inherited from a parent Claude Code process, hijack
    # auth/routing and cause a 403 ("organization does not have access").
    # We scrub them so the spawned `claude` uses its own OAuth/keychain instead.
    SCRUB_PREFIXES = ("CLAUDE_CODE_", "CLAUDE_AGENT")
    SCRUB_EXACT = {"ANTHROPIC_BASE_URL", "ANTHROPIC_API_KEY", "ANTHROPIC_AUTH_TOKEN"}

    def models(self) -> list[str]:
        return list(self.MODELS)

    def default_model(self) -> str:
        return "haiku"

    def _clean_env(self) -> dict:
        token = _secret("CLAUDE_CODE_OAUTH_TOKEN")  # captured before scrub
        env = {
            k: v for k, v in os.environ.items()
            if k not in self.SCRUB_EXACT
            and not any(k.startswith(p) for p in self.SCRUB_PREFIXES)
        }
        if token:
            env["CLAUDE_CODE_OAUTH_TOKEN"] = token
        return env

    def available(self) -> tuple[bool, str]:
        if not shutil.which("claude"):
            return False, "claude CLI not installed"
        if not _secret("CLAUDE_CODE_OAUTH_TOKEN"):
            return False, "needs: run `claude setup-token`, put token in secrets.local"
        return True, "ready (Max via OAuth token)"

    def ask(self, model: str, prompt: str, *, max_tokens: int = 64,
            timeout: float = 120.0) -> LLMResult:
        cmd = ["claude", "-p", prompt, "--model", model, "--output-format", "json"]
        try:
            rc, out, err, dt = self._run(cmd, env=self._clean_env(), timeout=timeout)
        except subprocess.TimeoutExpired:
            return LLMResult(self.name, model, False, error=f"timeout after {timeout}s",
                             latency_ms=timeout * 1000)
        except Exception as e:  # noqa: BLE001
            return LLMResult(self.name, model, False, error=str(e))

        try:
            data = json.loads(out)
        except json.JSONDecodeError:
            return LLMResult(self.name, model, False,
                             error=(err or out or "no output").strip()[:300],
                             latency_ms=dt)
        if data.get("is_error"):
            return LLMResult(self.name, model, False,
                             error=str(data.get("result", "unknown error"))[:300],
                             latency_ms=dt, raw=data)
        usage = data.get("usage", {}) or {}
        out_tok = usage.get("output_tokens")
        api_ms = data.get("duration_ms", dt)
        tps = (out_tok / (api_ms / 1000.0)) if (out_tok and api_ms) else None
        return LLMResult(self.name, model, True,
                         text=str(data.get("result", "")).strip(),
                         latency_ms=float(api_ms or dt),
                         tokens_out=out_tok, tok_per_s=tps, raw=data)


# --------------------------------------------------------------------------- #
# GPT (OpenAI) -- `codex exec`, ChatGPT Plus/Pro subscription (Sign in w/ ChatGPT)
# --------------------------------------------------------------------------- #


class CodexProvider(Provider):
    name = "gpt"
    display = "GPT (OpenAI / Codex)"
    kind = "cloud"

    # "default" -> let codex pick (safest). Others are overridable; an unknown
    # model surfaces as a clear error rather than a crash.
    MODELS = ["default", "gpt-5.5", "gpt-5.3-codex"]

    def models(self) -> list[str]:
        return list(self.MODELS)

    def default_model(self) -> str:
        return "default"

    def available(self) -> tuple[bool, str]:
        if not shutil.which("codex"):
            return False, "codex CLI not installed"
        try:
            p = subprocess.run(["codex", "login", "status"], stdin=subprocess.DEVNULL,
                               capture_output=True, text=True, timeout=15)
            txt = (p.stdout + p.stderr).lower()
            if "logged in" in txt:
                return True, "ready (ChatGPT login)"
            return False, "not logged in: run `codex login`"
        except Exception as e:  # noqa: BLE001
            return False, f"status check failed: {e}"

    def ask(self, model: str, prompt: str, *, max_tokens: int = 64,
            timeout: float = 180.0) -> LLMResult:
        cmd = ["codex", "exec", "--skip-git-repo-check", "--sandbox", "read-only", "--json"]
        if model and model != "default":
            cmd += ["-m", model]
        cmd.append(prompt)
        try:
            rc, out, err, dt = self._run(cmd, timeout=timeout)
        except subprocess.TimeoutExpired:
            return LLMResult(self.name, model, False, error=f"timeout after {timeout}s",
                             latency_ms=timeout * 1000)
        except Exception as e:  # noqa: BLE001
            return LLMResult(self.name, model, False, error=str(e))

        # JSONL stream: take the last agent_message text; usage from turn.completed.
        text, usage, err_evt = "", {}, ""
        for line in out.splitlines():
            line = line.strip()
            if not line.startswith("{"):
                continue
            try:
                evt = json.loads(line)
            except json.JSONDecodeError:
                continue
            t = evt.get("type")
            if t == "item.completed":
                item = evt.get("item", {})
                if item.get("type") == "agent_message":
                    text = item.get("text", text)
            elif t == "turn.completed":
                usage = evt.get("usage", {}) or {}
            elif t in ("error", "turn.failed"):
                err_evt = json.dumps(evt)[:300]

        if not text:
            return LLMResult(self.name, model, False,
                             error=(err_evt or err or "no agent_message in output").strip()[:300],
                             latency_ms=dt)
        out_tok = usage.get("output_tokens")
        tps = (out_tok / (dt / 1000.0)) if (out_tok and dt) else None
        return LLMResult(self.name, model, True, text=text.strip(),
                         latency_ms=dt, tokens_out=out_tok, tok_per_s=tps, raw={"usage": usage})


# --------------------------------------------------------------------------- #
# Local -- Ollama HTTP API, fully offline, no internet / tokens / accounts
# --------------------------------------------------------------------------- #


class OllamaProvider(Provider):
    name = "local"
    display = "Local (Ollama, offline)"
    kind = "local"

    HOST = "http://localhost:11434"

    def __init__(self) -> None:
        self._cache: Optional[list[str]] = None

    def models(self) -> list[str]:
        if self._cache is not None:
            return self._cache
        try:
            req = urllib.request.Request(f"{self.HOST}/api/tags")
            with urllib.request.urlopen(req, timeout=4) as r:
                data = json.loads(r.read().decode())
            self._cache = [m["name"] for m in data.get("models", [])]
        except Exception:  # noqa: BLE001
            self._cache = []
        return self._cache

    def default_model(self) -> str:
        m = self.models()
        for pref in ("llama3.2:3b", "qwen2.5:3b", "llama3.2:1b"):
            if pref in m:
                return pref
        return m[0] if m else "llama3.2:3b"

    def available(self) -> tuple[bool, str]:
        try:
            with urllib.request.urlopen(f"{self.HOST}/api/tags", timeout=3) as r:
                data = json.loads(r.read().decode())
            n = len(data.get("models", []))
            return (n > 0), (f"ready ({n} models, offline)" if n else "running but no models pulled")
        except Exception:
            return False, "ollama not running: `ollama serve`"

    def ask(self, model: str, prompt: str, *, max_tokens: int = 64,
            timeout: float = 120.0, system: Optional[str] = None,
            temperature: Optional[float] = None) -> LLMResult:
        opts = {"num_predict": max_tokens}
        if temperature is not None:
            opts["temperature"] = temperature
        payload = {
            "model": model,
            "prompt": prompt,
            "stream": False,
            "options": opts,
        }
        if system:
            payload["system"] = system
        body = json.dumps(payload).encode()
        req = urllib.request.Request(f"{self.HOST}/api/generate", data=body,
                                     headers={"Content-Type": "application/json"})
        t0 = time.perf_counter()
        try:
            with urllib.request.urlopen(req, timeout=timeout) as r:
                data = json.loads(r.read().decode())
        except urllib.error.URLError as e:
            return LLMResult(self.name, model, False, error=f"http: {e}")
        except Exception as e:  # noqa: BLE001
            return LLMResult(self.name, model, False, error=str(e))
        dt = (time.perf_counter() - t0) * 1000.0

        # Ollama durations are nanoseconds.
        load_ns = data.get("load_duration", 0) or 0
        peval_ns = data.get("prompt_eval_duration", 0) or 0
        eval_ns = data.get("eval_duration", 0) or 0
        eval_cnt = data.get("eval_count", 0) or 0
        ttft = (load_ns + peval_ns) / 1e6 if (load_ns or peval_ns) else None
        tps = (eval_cnt / (eval_ns / 1e9)) if (eval_cnt and eval_ns) else None
        return LLMResult(self.name, model, True,
                         text=str(data.get("response", "")).strip(),
                         latency_ms=dt, ttft_ms=ttft, tokens_out=eval_cnt or None,
                         tok_per_s=tps, raw=data)


# --------------------------------------------------------------------------- #
# Manager -- the single entry point ("gather all the LLMs")
# --------------------------------------------------------------------------- #


class LLMManager:
    SPEED_PROMPT = "Reply with exactly one word: PONG"

    # Local refiner: SPELLING + GRAMMAR fix ONLY. It must not reinterpret the
    # message (that is what made it mangle jargon like "abm"). Strict + low temp.
    REFINER_SYSTEM = (
        "You are a spelling and grammar corrector. Return the user's message with "
        "spelling mistakes and grammar fixed. Keep the exact wording, meaning, "
        "intent, terms, and language. Do NOT rephrase, reword, expand, shorten, "
        "translate, answer, explain, or add anything. Leave unknown words/acronyms "
        "(e.g. abm) exactly as written. Output ONLY the corrected message."
    )

    def __init__(self, max_workers: int = 6) -> None:
        self._providers: dict[str, Provider] = {
            p.name: p for p in (ClaudeProvider(), CodexProvider(), OllamaProvider())
        }
        self._pool = ThreadPoolExecutor(max_workers=max_workers,
                                        thread_name_prefix="llm")

    # -- introspection -- #
    def providers(self) -> list[str]:
        return list(self._providers.keys())

    def provider(self, name: str) -> Provider:
        return self._providers[name]

    def models(self, provider: str) -> list[str]:
        return self._providers[provider].models()

    def availability(self) -> dict[str, tuple[bool, str]]:
        return {n: p.available() for n, p in self._providers.items()}

    # -- calls -- #
    def ask(self, provider: str, model: str, prompt: str, **kw) -> LLMResult:
        p = self._providers.get(provider)
        if p is None:
            return LLMResult(provider, model, False, error=f"unknown provider '{provider}'")
        if not model:
            model = p.default_model()
        return p.ask(model, prompt, **kw)

    def ask_async(self, provider: str, model: str, prompt: str, **kw) -> Future:
        return self._pool.submit(self.ask, provider, model, prompt, **kw)

    def speed_test(self, prompt: Optional[str] = None,
                   targets: Optional[list[tuple[str, str]]] = None,
                   only_available: bool = True) -> list[LLMResult]:
        """Run prompt across (provider, model) targets concurrently; fastest first.

        Note: a local model's FIRST call pays a cold model-load; run twice for a
        fair warm comparison. The UI labels cold vs warm.
        """
        prompt = prompt or self.SPEED_PROMPT
        if targets is None:
            avail = self.availability()
            targets = [
                (n, p.default_model())
                for n, p in self._providers.items()
                if (not only_available) or avail[n][0]
            ]
        futs = {self._pool.submit(self.ask, prov, mod, prompt): (prov, mod)
                for prov, mod in targets}
        results: list[LLMResult] = []
        for fut in futs:
            try:
                results.append(fut.result())
            except Exception as e:  # noqa: BLE001
                prov, mod = futs[fut]
                results.append(LLMResult(prov, mod, False, error=str(e)))
        results.sort(key=lambda r: (not r.ok, r.latency_ms))
        return results

    # -- refine + pipeline (the ABM flow: local refines -> cloud answers) -- #
    def refine(self, raw: str, model: Optional[str] = None,
               max_tokens: int = 200) -> LLMResult:
        """Local LLM rewrites rough text into a clean prompt. Offline."""
        local = self._providers["local"]
        model = model or local.default_model()
        return local.ask(model, raw, system=self.REFINER_SYSTEM,
                         max_tokens=max_tokens, temperature=0.0)

    def pipeline(self, raw: str, target_provider: str,
                 target_model: Optional[str] = None,
                 refiner_model: Optional[str] = None,
                 answer_tokens: int = 400) -> tuple[LLMResult, Optional[LLMResult]]:
        """raw text -> local refine -> cloud answer. Returns (refined, answer).

        answer is None if the refine step failed.
        """
        refined = self.refine(raw, refiner_model)
        if not refined.ok or not refined.text.strip():
            return refined, None
        tgt = self._providers.get(target_provider)
        model = target_model or (tgt.default_model() if tgt else "")
        answer = self.ask(target_provider, model, refined.text.strip(),
                          max_tokens=answer_tokens)
        return refined, answer

    def pipeline_async(self, raw: str, target_provider: str, **kw) -> Future:
        return self._pool.submit(self.pipeline, raw, target_provider, **kw)

    def refine_async(self, raw: str, model: Optional[str] = None) -> Future:
        return self._pool.submit(self.refine, raw, model)

    def shutdown(self) -> None:
        self._pool.shutdown(wait=False, cancel_futures=True)
