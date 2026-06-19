"""
selftest.py -- exercise llm_manager headlessly (no pygame, no display).

Proves the subprocess/HTTP plumbing + async + speed test work, independent of
the UI. Run:  python3 selftest.py
"""

from llm_manager import LLMManager


def main() -> None:
    mgr = LLMManager()

    print("=" * 70)
    print("AVAILABILITY")
    print("=" * 70)
    for name, (ok, msg) in mgr.availability().items():
        flag = "OK  " if ok else "-- "
        models = ", ".join(mgr.models(name)) or "(none)"
        print(f"{flag}{name:7} {msg}")
        print(f"        models: {models}")

    print()
    print("=" * 70)
    print("SINGLE CALLS (one per available provider, default model)")
    print("=" * 70)
    for name, (ok, _msg) in mgr.availability().items():
        if not ok:
            print(f"-- {name}: skipped (not available)")
            continue
        model = mgr.provider(name).default_model()
        res = mgr.ask(name, model, "Reply with exactly one word: PONG")
        print(res.short())

    print()
    print("=" * 70)
    print("SPEED TEST (available providers, fastest first)")
    print("  note: local first call is COLD (model load); 2nd run is warm")
    print("=" * 70)
    for run in ("cold", "warm"):
        print(f"--- {run} run ---")
        ranking = mgr.speed_test()
        for i, r in enumerate(ranking, 1):
            print(f"  {i}. {r.short()}")

    mgr.shutdown()


if __name__ == "__main__":
    main()
