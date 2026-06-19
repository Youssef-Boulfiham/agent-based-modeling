"""
app.py -- pygame sandbox UI for the LLM control layer.

Reads as a literal pipeline so it is obvious what happens:

   [ 1. YOUR MESSAGE ]  --clean up-->  [ 2. LOCAL LLM rewrites ]  --send-->  [ 3. CLOUD LLM answers ]

  * box 1 = exactly what you typed
  * box 2 = the offline model's rewritten prompt (shows which local model)
  * box 3 = the cloud model's answer (shows Claude/GPT + sub-model)
  * the active box lights up while its model is working
  * everything runs async -> the UI never freezes (== the C++ ABM pattern)

MODE toggle:  "local refine -> cloud"  (default)  vs  "direct ask" (skip box 2).

Run:  python3 app.py
"""

from __future__ import annotations

import threading
from concurrent.futures import Future

import pygame

from llm_manager import LLMManager

# ----- window / theme ------------------------------------------------------- #
W, H = 1200, 800
BG = (16, 18, 24)
CARD = (28, 31, 40)
CARD_DIM = (23, 25, 32)
SLOT = (12, 13, 17)
INK = (230, 233, 240)
MUTE = (138, 145, 160)
ACCENT = (94, 162, 255)
GOOD = (88, 200, 130)
BAD = (224, 96, 96)
WARN = (235, 190, 90)
LOCAL_COL = (120, 210, 150)
CLOUD_COL = (150, 170, 255)
YOU_COL = (255, 196, 84)

PROV_LABELS = {"claude": "Claude", "gpt": "GPT", "local": "Local"}

# ----- layout --------------------------------------------------------------- #
M = 24
PY, PH = 96, 250                                   # pipeline row
AW = 44                                            # arrow gap
CW = (W - 2 * M - 2 * AW) // 3                      # card width
C1X = M
C2X = M + CW + AW
C3X = M + 2 * (CW + AW)
CY = PY + PH + 26                                  # controls start


class Button:
    def __init__(self, rect, label, on_click):
        self.rect = pygame.Rect(rect)
        self.label = label
        self.on_click = on_click
        self.enabled = True
        self.active = False

    def draw(self, surf, font):
        if not self.enabled:
            bg, fg = CARD_DIM, MUTE
        elif self.active:
            bg, fg = ACCENT, (12, 14, 18)
        else:
            bg, fg = (38, 42, 54), INK
        pygame.draw.rect(surf, bg, self.rect, border_radius=7)
        if self.active and self.enabled:
            pygame.draw.rect(surf, ACCENT, self.rect, width=2, border_radius=7)
        t = font.render(self.label, True, fg)
        surf.blit(t, t.get_rect(center=self.rect.center))

    def hit(self, pos):
        return self.enabled and self.rect.collidepoint(pos)


def wrap(font, text, width):
    lines = []
    for para in text.split("\n"):
        cur = ""
        for w in para.split(" "):
            trial = (cur + " " + w).strip()
            if font.size(trial)[0] <= width:
                cur = trial
            else:
                if cur:
                    lines.append(cur)
                cur = w
        lines.append(cur)
    return lines


class App:
    def __init__(self):
        pygame.init()
        pygame.display.set_caption("ABM · LLM Pipeline Sandbox")
        self.screen = pygame.display.set_mode((W, H))
        self.clock = pygame.time.Clock()
        self.f = pygame.font.SysFont("menlo,consolas,monospace", 15)
        self.fb = pygame.font.SysFont("menlo,consolas,monospace", 16, bold=True)
        self.fs = pygame.font.SysFont("menlo,consolas,monospace", 13)
        self.fbig = pygame.font.SysFont("menlo,consolas,monospace", 20, bold=True)

        self.mgr = LLMManager()
        self.avail = {n: (False, "checking...") for n in self.mgr.providers()}
        threading.Thread(target=self._refresh_avail, daemon=True).start()

        self.provider = "claude"                       # cloud answerer
        self.model = self.mgr.provider("claude").default_model()
        self.refiner_model = self.mgr.provider("local").default_model()
        self.prompt = ""                               # current input text
        self.pipeline_mode = True

        self.sent = ""                                 # box 1: message that was sent
        self.refined = None                            # box 2: LLMResult
        self.answer = None                             # box 3: LLMResult
        self.stage = "idle"                            # idle|refining|answering|done
        self.note = ""

        self.pending: Future | None = None
        self.pending_kind = ""
        self.speed = []
        self.speed_msg = ""
        self.speed_thread: threading.Thread | None = None

        self.buttons: list[Button] = []
        self.input_rect = pygame.Rect(0, 0, 0, 0)
        self._build_buttons()
        pygame.key.start_text_input()

    # -- availability -- #
    def _refresh_avail(self):
        self.avail = self.mgr.availability()
        self._build_buttons()

    def _manual_refresh(self):
        threading.Thread(target=self._refresh_avail, daemon=True).start()

    # -- buttons -- #
    def _build_buttons(self):
        self.buttons = []
        # cloud provider row
        px = C1X + 150
        for i, p in enumerate(self.mgr.providers()):
            if p == "local":
                continue
            b = Button((px, CY + 22, 96, 32), PROV_LABELS[p],
                       lambda p=p: self._sel_provider(p))
            b.active = (p == self.provider)
            self.buttons.append(b)
            px += 106
        # mode toggle
        tg = Button((C3X - 6, CY + 22, CW + 6, 32),
                    "MODE: refine -> cloud" if self.pipeline_mode else "MODE: direct ask (skip local)",
                    self._toggle_mode)
        tg.active = self.pipeline_mode
        self.buttons.append(tg)
        # cloud model row
        mx = C1X + 150
        for m in self.mgr.provider(self.provider).models()[:5]:
            b = Button((mx, CY + 64, 120, 32), m, lambda m=m: self._sel_model(m))
            b.active = (m == self.model)
            self.buttons.append(b)
            mx += 126
        # input + actions
        self.input_rect = pygame.Rect(C1X, CY + 132, W - 2 * M - 360, 46)
        self.buttons.append(Button((self.input_rect.right + 14, CY + 132, 160, 46),
                                   "SEND  (Enter)", self._send))
        self.buttons.append(Button((self.input_rect.right + 184, CY + 132, 152, 46),
                                   "SPEED TEST", self._speed_test))
        self.buttons.append(Button((C1X, CY + 190, 130, 30), "Refresh status",
                                   self._manual_refresh))

    def _toggle_mode(self):
        self.pipeline_mode = not self.pipeline_mode
        self._build_buttons()

    def _sel_provider(self, p):
        self.provider = p
        self.model = self.mgr.provider(p).default_model()
        self._build_buttons()

    def _sel_model(self, m):
        self.model = m
        self._build_buttons()

    # -- send / async pipeline (two visible phases) -- #
    def _send(self):
        if self.pending is not None or not self.prompt.strip():
            return
        if not self.avail.get(self.provider, (False, ""))[0]:
            self.note = f"{PROV_LABELS[self.provider]} not ready — click Refresh / see status"
            return
        self.sent = self.prompt.strip()
        self.refined = None
        self.answer = None
        self.note = ""
        if self.pipeline_mode:
            if not self.avail.get("local", (False, ""))[0]:
                self.note = "local refiner not ready (start `ollama serve`)"
                return
            self.stage = "refining"
            self.pending_kind = "refine"
            self.pending = self.mgr.refine_async(self.sent, self.refiner_model)
        else:
            self.stage = "answering"
            self.pending_kind = "answer"
            self.pending = self.mgr.ask_async(self.provider, self.model, self.sent,
                                              max_tokens=300)

    def _poll(self):
        if self.pending is None or not self.pending.done():
            return
        res = self.pending.result()
        self.pending = None
        if self.pending_kind == "refine":
            self.refined = res
            if res.ok and res.text.strip():
                # phase 2: hand the refined prompt to the cloud model
                self.stage = "answering"
                self.pending_kind = "answer"
                self.pending = self.mgr.ask_async(self.provider, self.model,
                                                  res.text.strip(), max_tokens=300)
            else:
                self.stage = "done"
                self.pending_kind = ""
        elif self.pending_kind == "answer":
            self.answer = res
            self.stage = "done"
            self.pending_kind = ""

    # -- speed test -- #
    def _speed_test(self):
        if self.speed_thread and self.speed_thread.is_alive():
            return
        self.speed = []
        self.speed_msg = "running across available LLMs..."

        def work():
            self.speed = self.mgr.speed_test()
            self.speed_msg = "fastest first (local 1st call = cold load)"

        self.speed_thread = threading.Thread(target=work, daemon=True)
        self.speed_thread.start()

    # -- events -- #
    def _on_key(self, e):
        if e.key == pygame.K_BACKSPACE:
            self.prompt = self.prompt[:-1]
        elif e.key in (pygame.K_RETURN, pygame.K_KP_ENTER):
            self._send()
        elif e.key == pygame.K_ESCAPE:
            self.prompt = ""

    # -- drawing helpers -- #
    def _card(self, x, num, title, who, who_col, body, body_col, active, skipped=False):
        rect = pygame.Rect(x, PY, CW, PH)
        pygame.draw.rect(self.screen, CARD_DIM if skipped else CARD, rect, border_radius=12)
        if active:
            pygame.draw.rect(self.screen, ACCENT, rect, width=3, border_radius=12)
        # number badge
        self.screen.blit(self.fbig.render(str(num), True, ACCENT if active else MUTE),
                         (rect.x + 14, rect.y + 12))
        self.screen.blit(self.fb.render(title, True, INK), (rect.x + 42, rect.y + 16))
        self.screen.blit(self.fs.render(who, True, who_col), (rect.x + 14, rect.y + 44))
        slot = pygame.Rect(rect.x + 14, rect.y + 70, rect.w - 28, rect.h - 84)
        pygame.draw.rect(self.screen, SLOT, slot, border_radius=8)
        lines = wrap(self.fs, body, slot.w - 20)[:9]
        for i, ln in enumerate(lines):
            self.screen.blit(self.fs.render(ln, True, body_col), (slot.x + 10, slot.y + 10 + i * 18))
        return rect

    def _arrow(self, x, label, lit):
        cx, cy = x + AW // 2, PY + PH // 2
        col = ACCENT if lit else MUTE
        pygame.draw.line(self.screen, col, (cx - 16, cy), (cx + 14, cy), 3)
        pygame.draw.polygon(self.screen, col, [(cx + 14, cy - 7), (cx + 14, cy + 7), (cx + 24, cy)])
        t = self.fs.render(label, True, col)
        self.screen.blit(t, t.get_rect(center=(cx + 4, cy - 18)))

    def _draw(self):
        s = self.screen
        s.fill(BG)
        s.blit(self.fbig.render("LLM PIPELINE", True, INK), (M, 22))
        s.blit(self.fs.render("your text  ->  a small OFFLINE model fixes spelling & grammar (keeps your wording)"
                              "  ->  the CLOUD model you pick answers it",
                              True, MUTE), (M, 54))

        # box 1: your message
        b1 = self.sent or self.prompt or "(type below, press Send)"
        self._card(C1X, 1, "YOUR MESSAGE", "you typed", YOU_COL, b1, INK, False)

        # arrow 1
        self._arrow(C1X + CW, "fix typos", self.stage == "refining")

        # box 2: local refine
        if not self.pipeline_mode:
            self._card(C2X, 2, "LOCAL FIX", "skipped (direct mode)", MUTE,
                       "direct ask: your message goes straight to the cloud model",
                       MUTE, False, skipped=True)
        else:
            who2 = f"LOCAL · {self.refiner_model} (offline)"
            if self.stage == "refining":
                body2, col2 = "fixing spelling & grammar ...", WARN
            elif self.refined and self.refined.ok:
                body2, col2 = self.refined.text, INK
            elif self.refined:
                body2, col2 = "ERROR: " + self.refined.error, BAD
            else:
                body2, col2 = "waiting for your message", MUTE
            self._card(C2X, 2, "LOCAL FIX", who2, LOCAL_COL, body2, col2,
                       self.stage == "refining")

        # arrow 2
        self._arrow(C2X + CW, "send prompt", self.stage == "answering")

        # box 3: cloud answer
        who3 = f"CLOUD · {PROV_LABELS[self.provider]} / {self.model}"
        if self.stage == "answering":
            body3, col3 = "answering ...", WARN
        elif self.answer and self.answer.ok:
            body3, col3 = self.answer.text, INK
        elif self.answer:
            body3, col3 = "ERROR: " + self.answer.error, BAD
        else:
            body3, col3 = "waiting for the refined prompt", MUTE
        self._card(C3X, 3, "CLOUD ANSWER", who3, CLOUD_COL, body3, col3,
                   self.stage == "answering")

        self._draw_controls(s)
        self._draw_speed(s)
        pygame.display.flip()

    def _draw_controls(self, s):
        s.blit(self.fb.render("PICK CLOUD MODEL (who answers):", True, INK), (C1X, CY))
        s.blit(self.f.render("provider", True, MUTE), (C1X, CY + 28))
        s.blit(self.f.render("model", True, MUTE), (C1X, CY + 70))
        # availability dots by provider buttons
        ok_local = self.avail.get("local", (False, ""))[0]
        # status line: who talks to who + readiness
        chain = (f"{PROV_LABELS['local']}({'on' if ok_local else 'off'}) refines  ->  "
                 f"{PROV_LABELS[self.provider]} answers") if self.pipeline_mode else \
                f"direct: {PROV_LABELS[self.provider]} answers"
        s.blit(self.f.render(chain, True, ACCENT), (C1X, CY + 104))
        ok, msg = self.avail.get(self.provider, (False, ""))
        s.blit(self.fs.render(f"{PROV_LABELS[self.provider]} status: {msg}", True,
                              GOOD if ok else WARN), (C1X + 300, CY + 106))
        if self.note:
            s.blit(self.fs.render(self.note, True, BAD), (C1X, CY + 184))
        # input box
        pygame.draw.rect(s, SLOT, self.input_rect, border_radius=8)
        pygame.draw.rect(s, ACCENT, self.input_rect, width=2, border_radius=8)
        shown = self.prompt if self.prompt else "type a rough message here..."
        col = INK if self.prompt else MUTE
        while self.f.size(shown)[0] > self.input_rect.w - 20 and len(shown) > 4:
            shown = shown[1:]
        s.blit(self.f.render(shown, True, col), (self.input_rect.x + 10, self.input_rect.y + 14))
        for b in self.buttons:
            b.draw(s, self.f)

    def _draw_speed(self, s):
        panel = pygame.Rect(C1X, CY + 226, W - 2 * M, H - (CY + 226) - M)
        if panel.h < 60:
            return
        pygame.draw.rect(s, CARD, panel, border_radius=12)
        s.blit(self.fb.render("SPEED TEST", True, INK), (panel.x + 16, panel.y + 12))
        if self.speed_msg:
            s.blit(self.fs.render(self.speed_msg, True, MUTE), (panel.x + 150, panel.y + 14))
        maxlat = max((r.latency_ms for r in self.speed if r.ok), default=1.0) or 1.0
        x = panel.x + 16
        for r in self.speed:
            label = f"{PROV_LABELS.get(r.provider, r.provider)}/{r.model}"
            s.blit(self.fs.render(label, True, INK), (x, panel.y + 40))
            barw = 240
            if r.ok:
                frac = max(0.04, r.latency_ms / maxlat)
                pygame.draw.rect(s, SLOT, (x, panel.y + 60, barw, 12), border_radius=6)
                pygame.draw.rect(s, ACCENT, (x, panel.y + 60, int(barw * frac), 12), border_radius=6)
                s.blit(self.fs.render(f"{r.latency_ms:.0f} ms", True, GOOD), (x, panel.y + 78))
            else:
                s.blit(self.fs.render("error", True, BAD), (x, panel.y + 60))
            x += barw + 40

    def run(self):
        running = True
        while running:
            for e in pygame.event.get():
                if e.type == pygame.QUIT:
                    running = False
                elif e.type == pygame.TEXTINPUT:
                    self.prompt += e.text
                elif e.type == pygame.KEYDOWN:
                    self._on_key(e)
                elif e.type == pygame.MOUSEBUTTONDOWN and e.button == 1:
                    for b in self.buttons:
                        if b.hit(e.pos):
                            b.on_click()
                            break
            self._poll()
            self._draw()
            self.clock.tick(60)
        self.mgr.shutdown()
        pygame.quit()


if __name__ == "__main__":
    App().run()
