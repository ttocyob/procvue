# Procvue

**Procvue** — an ambient system monitor written using the Enlightenment Foundation Libraries, powered by the [enigmatic](https://git.enlightenment.org/netstar/enigmatic.git) daemon. A GKrellM homage.

![Procvue screenshot](https://github.com/user-attachments/assets/a4eab3f7-2903-4dce-8777-289a3b894c8e)

---

Procvue is a second-generation [enigmatic](https://git.enlightenment.org/netstar/enigmatic.git) client. System data is sourced from the enigmatic daemon via libenigmatic_client — procvue attaches to the running daemon and receives system snapshots in real time.

---

## Panels

- **CPU** — smoothed utilization average across all cores, with bar indicator
- **FREQ** — average CPU frequency across all cores (MHz / GHz)
- **TEMP** — average CPU temperature across all cores (°C), with warm/hot threshold indicators
- **PROC / USERS** — process count and logged-in user count
- **DISK** — aggregate read/write rates across all mounted filesystems
- **NET** — RX/TX LED indicators with auto-detected active interface and decay timers
- **RAM** — memory utilization percentage with bar indicator
- **UPTIME** — system uptime

---

## Dependencies
- EFL (ecore, ecore-evas, edje, elementary)
- [enigmatic](https://git.enlightenment.org/netstar/enigmatic.git) daemon + libenigmatic_client
- Meson + Ninja

---

## Building

Procvue requires the enigmatic daemon. Build and install it first:

```bash
git clone https://git.enlightenment.org/netstar/enigmatic.git
cd enigmatic
meson setup build
ninja -C build
sudo ninja -C build install
enigmatic
```

Then build procvue:

```bash
git clone https://github.com/ttocyob/procvue.git
cd procvue
meson setup build
ninja -C build
sudo ninja -C build install
```

---

## Planned

- Client-side decorations (CSD)
- Battery panel
- Other panels?

---

## Acknowledgements

Procvue uses [enigmatic](https://git.enlightenment.org/netstar/enigmatic.git), a system monitoring daemon written by Alastair Poole (netstar / [haxworx](https://github.com/haxworx)). Alastair is also the author of [Evisum](https://www.enlightenment.org/about-evisum), the established EFL system monitor. 

---

## License

BSD-2-Clause
