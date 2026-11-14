# Procvue

**Procvue** — an ambient system monitor written using the Enlightenment Foundation Libraries, powered by the [enigmatic](https://git.enlightenment.org/enlightenment/evisum) daemon. A GKrellM homage.

![Procvue screenshot](https://github.com/user-attachments/assets/a4eab3f7-2903-4dce-8777-289a3b894c8e)

---

Procvue is a second-generation [enigmatic](https://git.enlightenment.org/enlightenment/evisum) client. System data is sourced from the enigmatic daemon via libenigmatic_client — procvue attaches to the running daemon and receives system snapshots in real time.

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
- EFL >= 1.27.0 (ecore, ecore-evas, edje, eio, eina, elementary, eet) and `edje_cc`
- enigmatic daemon + libenigmatic_client, both provided by [Evisum](https://git.enlightenment.org/enlightenment/evisum)
- Meson + Ninja

> **Note:** the standalone `netstar/enigmatic` repository is outdated and does not install
> libenigmatic_client, its headers or `enigmatic_client.pc`. Use the copy of enigmatic that
> ships with Evisum.

---

## Building

Procvue requires the enigmatic daemon and libenigmatic_client, which are built and installed
as part of Evisum. Build and install Evisum first:

```bash
git clone https://git.enlightenment.org/enlightenment/evisum.git
cd evisum
meson setup build
ninja -C build
sudo ninja -C build install
sudo ldconfig
```

This installs the `enigmatic` daemon, `libenigmatic_client`, its headers (under
`include/enigmatic/`) and `enigmatic_client.pc`. Check that pkg-config can find it:

```bash
pkg-config --modversion enigmatic_client
```

If it isn't found, make sure the install prefix's pkgconfig directory (for example
`/usr/local/lib/x86_64-linux-gnu/pkgconfig`) is in your pkg-config search path.

Then build procvue:

```bash
git clone https://github.com/ttocyob/procvue.git
cd procvue
meson setup build
ninja -C build
sudo ninja -C build install
```

Procvue will launch the enigmatic daemon automatically.

---

## Planned

- Client-side decorations (CSD)
- Battery panel
- Other panels?

---

## Acknowledgements

Procvue uses enigmatic, a system monitoring daemon that ships with Evisum and was written by Alastair Poole (netstar / [haxworx](https://github.com/haxworx)). Alastair is also the author of [Evisum](https://www.enlightenment.org/about-evisum), the established EFL system monitor. 

---

## License

BSD-2-Clause
