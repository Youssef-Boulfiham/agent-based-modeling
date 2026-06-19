"""Headless smoke: construct App, draw a few frames, no real window. Catches crashes."""
import os
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
os.environ.setdefault("SDL_AUDIODRIVER", "dummy")

import time
from app import App

a = App()
# let the availability thread finish
time.sleep(2.0)
a._build_buttons()
for _ in range(5):
    a._poll()
    a._draw()
    time.sleep(0.05)
# simulate selecting each cloud provider + drawing
for p in ("claude", "gpt"):
    a._sel_provider(p)
    a._draw()
a._toggle_mode(); a._draw(); a._toggle_mode()
a.prompt = "hello world"
a._draw()
print("UI smoke OK: providers=", a.mgr.providers(),
      "| selected=", a.provider, a.model,
      "| avail=", {k: v[0] for k, v in a.avail.items()})
a.mgr.shutdown()
import pygame
pygame.quit()
