# pid_tuner_plot.py — 2x2: [θ&SP | erro] / [u&duty | painel de ganhos]
import serial, time, threading, collections, tkinter as tk
from tkinter import ttk
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import numpy as np

PORT = "/dev/ttyUSB0"
BAUD = 115200
WIN_SEC = 15
HZ = 50

ser = serial.Serial(PORT, BAUD, timeout=0.4, write_timeout=0.8)

def drain():
    try:
        ser.reset_input_buffer(); ser.reset_output_buffer()
    except Exception:
        pass
    time.sleep(0.03)

def send_line(s: str):
    ser.write((s + "\n").encode()); ser.flush()

def read_until_prefix(prefix: str, tmo=2.0):
    t0 = time.time()
    while time.time() - t0 < tmo:
        line = ser.readline().decode(errors="ignore").strip()
        if not line: 
            continue
        if line.startswith(prefix):
            return line
    return None

def do_get():
    try:
        drain(); send_line("GET")
        line = read_until_prefix("G ")
        if not line: return
        _, skp, ski, skd = line.split()
        kp_var.set(float(skp)); ki_var.set(float(ski)); kd_var.set(float(skd))
    except Exception:
        pass

def do_apply():
    try:
        drain()
        send_line(f"G {kp_var.get():.6f} {ki_var.get():.6f} {kd_var.get():.6f}")
        _ = read_until_prefix("G ")
    except Exception:
        pass

def do_save():
    try:
        do_apply(); time.sleep(0.05); drain(); send_line("SAVE")
        ok = read_until_prefix("OK")
        if ok != "OK": _ = read_until_prefix("OK")
    except Exception:
        pass

root = tk.Tk()
root.title("PID Tuner + Plot (grade 2×2)")

gridf = ttk.Frame(root)
gridf.pack(fill="both", expand=True, padx=8, pady=8)
for c in (0,1):
    gridf.columnconfigure(c, weight=1, uniform="cols")
for r in (0,1):
    gridf.rowconfigure(r, weight=1, uniform="rows")

panel = ttk.LabelFrame(gridf, text="Ganhos (Enter = Salvar)")
panel.grid(row=1, column=1, sticky="nsew", padx=6, pady=6)

kp_var = tk.DoubleVar(value=290.0)
ki_var = tk.DoubleVar(value=0.0)
kd_var = tk.DoubleVar(value=0.0)

def make_compact_control(parent, label, var, to, step, width=9):
    frm = ttk.Frame(parent)
    ttk.Label(frm, text=label, width=4).pack(side="left")
    ent = ttk.Entry(frm, width=width, textvariable=var, justify="right")
    ent.pack(side="left", padx=(2,2))

    def clamp(*_):
        try: v = float(var.get())
        except Exception: return
        if v < 0: v = 0.0
        if v > to: v = to
        var.set(v)
    var.trace_add("write", lambda *_: clamp())

    def round_to_step(v, s): return round(v/s)*s
    def nudge(d):
        try: v = float(var.get())
        except Exception: v = 0.0
        v = round_to_step(v + d, step)
        v = 0.0 if v < 0 else (to if v > to else v)
        var.set(v); return "break"

    ent.bind("<Left>",  lambda e: nudge(-step))
    ent.bind("<Right>", lambda e: nudge(+step))
    def on_wheel(event):
        if getattr(event, "num", None) == 4 or (hasattr(event, "delta") and event.delta > 0):
            return nudge(+step)
        return nudge(-step)
    ent.bind("<MouseWheel>", on_wheel)
    ent.bind("<Button-4>", on_wheel)
    ent.bind("<Button-5>", on_wheel)
    ent.bind("<Return>", lambda e: (do_save(), "break"))

    ttk.Button(frm, text="−", width=2, command=lambda: nudge(-step)).pack(side="left", padx=(2,0))
    ttk.Button(frm, text="+", width=2, command=lambda: nudge(+step)).pack(side="left", padx=(2,0))
    return frm

make_compact_control(panel, "Kp", kp_var, to=10000.0, step=1.0).pack(anchor="w", padx=6, pady=4)
make_compact_control(panel, "Ki", ki_var, to=10000.0,  step=0.1).pack(anchor="w", padx=6, pady=4)
make_compact_control(panel, "Kd", kd_var, to=10000.0,  step=1).pack(anchor="w", padx=6, pady=4)

btns = ttk.Frame(panel); btns.pack(fill="x", padx=6, pady=6)
ttk.Button(btns, text="GET", command=do_get).pack(side="left", padx=2)
ttk.Button(btns, text="Aplicar", command=do_apply).pack(side="left", padx=2)
ttk.Button(btns, text="Salvar", command=do_save).pack(side="left", padx=2)
root.bind("<Return>", lambda e: do_save())

def mk_plot_cell(parent, title, ylabel):
    fig = plt.Figure(figsize=(4.6, 3.1), dpi=100, constrained_layout=True)
    ax = fig.add_subplot(1,1,1)
    ax.set_title(title, fontsize=11, pad=6)
    ax.set_ylabel(ylabel, fontsize=10)
    ax.set_xlabel("tempo (s)", fontsize=10)
    ax.grid(True, linewidth=0.4, alpha=0.3)
    canvas = FigureCanvasTkAgg(fig, master=parent)
    widget = canvas.get_tk_widget()
    widget.configure(highlightthickness=0)
    return fig, ax, canvas, widget

# (0,0): θ & SP
fig11, ax11, cv11, w11 = mk_plot_cell(gridf, "θ & setpoint", "deg")
w11.grid(row=0, column=0, sticky="nsew", padx=6, pady=6)

# (0,1): erro
fig12, ax12, cv12, w12 = mk_plot_cell(gridf, "erro", "deg")
w12.grid(row=0, column=1, sticky="nsew", padx=6, pady=6)

# (1,0): u & duty
fig21, ax21, cv21, w21 = mk_plot_cell(gridf, "u & duty", "u / duty")
w21.grid(row=1, column=0, sticky="nsew", padx=6, pady=6)

def L(ax, *args, **kw):
    kw.setdefault("linewidth", 1.9)
    kw.setdefault("alpha", 0.96)
    return ax.plot(*args, **kw)[0]

ln_th = L(ax11, [], [], label="θ")
ln_sp = L(ax11, [], [], label="SP")
ax11.legend(loc="upper right", fontsize=9, frameon=False, ncol=2)
ax11.axhline(0, ls="--", lw=0.8, alpha=0.4)
ax11.set_ylim(-180, 180)     # θ fixo em [-180, 180]


ln_e = L(ax12, [], [], label="erro")
ax12.legend(loc="upper right", fontsize=9, frameon=False)
ax12.axhline(0, ls="--", lw=0.8, alpha=0.4)
ax12.set_ylim(-45, 45)     # <<< erro fixo em [-180, 180]

err_text = ax12.text(
    0.02, 0.95, "e = --.-°",
    transform=ax12.transAxes, ha="left", va="top",
    fontsize=10,
    bbox=dict(boxstyle="round", fc="white", ec="0.6", alpha=0.85)
)

ln_u    = L(ax21, [], [], label="u")
ln_duty = L(ax21, [], [], label="duty")
ax21.legend(loc="upper right", fontsize=9, frameon=False, ncol=2)
ax21.axhline(0, ls="--", lw=0.8, alpha=0.4)

N = WIN_SEC*HZ
buf_t  = collections.deque(maxlen=N)
buf = {k: collections.deque(maxlen=N) for k in ["TH","SP","E","U","DUTY"]}

def wrap180(x): 
    return ((x + 180.0) % 360.0) - 180.0

def parse_kv(line: str):
    try:
        parts = line.strip().split(',')
        d, i = {}, 0
        while i < len(parts)-1:
            k = parts[i].strip(); v = parts[i+1].strip()
            d[k] = v; i += 2
        return d
    except Exception:
        return {}

C2_KEYS = {"TH","SP","E","U"}

stop = False
def reader():
    while not stop:
        try:
            line = ser.readline().decode(errors="ignore")
            if not line or not line.startswith("T,"): 
                continue
            d = parse_kv(line)
            tms = int(float(d.get("T","0"))); t = tms/1000.0
            buf_t.append(t)
            for k in buf.keys():
                raw = d.get(k, "nan")
                try: val = float(raw)
                except Exception: val = float("nan")
                if k in C2_KEYS and np.isfinite(val): 
                    val /= 100.0
                if k in ("TH","SP"):
                    val = wrap180(val)
                buf[k].append(val)
        except Exception:
            time.sleep(0.01)

threading.Thread(target=reader, daemon=True).start()

def _autoscale(ax, series_list):
    arr = np.array([v for s in series_list for v in s], dtype=float)
    arr = arr[np.isfinite(arr)]
    if arr.size < 3: 
        return
    p5, p95 = np.percentile(arr, [5, 95])
    span = max(1e-6, p95 - p5)
    pad = 0.15 * span
    ax.set_ylim(p5 - pad, p95 + pad)

def refresh():
    if buf_t:
        t1 = buf_t[-1]
        t0 = max(buf_t[0], t1 - WIN_SEC)
        for ax in (ax11, ax12, ax21):
            ax.set_xlim(t0, t1)

        ln_th.set_data(buf_t, buf["TH"])
        ln_sp.set_data(buf_t, buf["SP"])
        ln_e.set_data(buf_t, buf["E"])
        err_text.set_text(f"e = {buf['E'][-1]:.1f}°")
        ln_u.set_data(buf_t, buf["U"])
        ln_duty.set_data(buf_t, buf["DUTY"])

        # ax11 e ax12 têm Y fixo em [-180,180]; autoscale só no u&duty
        _autoscale(ax21, [buf["U"], buf["DUTY"]])

        cv11.draw_idle(); cv12.draw_idle(); cv21.draw_idle()
    root.after(int(1000/HZ), refresh)

try:
    do_get()
except Exception:
    pass

refresh()

def on_close():
    global stop
    stop = True
    try: ser.close()
    except Exception: pass
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_close)
root.mainloop()
