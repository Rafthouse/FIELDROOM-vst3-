# FIELDROOM

**Professional headphone monitoring VST3 / AU audio plugin** — binaural virtual loudspeaker and acoustic control-room monitoring for studio headphones.

[![Build](https://github.com/Rafthouse/FIELDROOM-vst3-/actions/workflows/build.yml/badge.svg)](https://github.com/Rafthouse/FIELDROOM-vst3-/actions/workflows/build.yml)

---

## English

### What it does
FIELDROOM reproduces the experience of listening to calibrated studio monitors in a treated room, through ordinary studio headphones. It is an offline, real-time-safe audio effect: no telemetry, no network access, no background services.

### Requirements
- Windows 10/11 (x64) or macOS 11+ (Universal 2: Apple Silicon + Intel)
- A VST3 or AU host (Ableton Live, Reaper, Cubase, Logic Pro, Studio One, ...)

### Installation — Windows
1. Download `FIELDROOM-Windows-x64.zip` from [Releases](../../releases).
2. Unzip it.
3. Copy the `FIELDROOM.vst3` folder to:
   `C:\Program Files\Common Files\VST3\`
4. Restart your DAW and rescan plugins.
5. Insert **FIELDROOM** (vendor: AcousticLab) on a track or the master bus.

> If the plugin does not appear, install the Microsoft Visual C++ Redistributable (x64) and rescan again.

### Installation — macOS
1. Download `FIELDROOM-macOS-Universal2.zip` (or the `.pkg` installer) from [Releases](../../releases).
2. Unzip and copy `FIELDROOM.vst3` to `/Library/Audio/Plug-Ins/VST3/`, and `FIELDROOM.component` to `/Library/Audio/Plug-Ins/Components/`.
   With the `.pkg` installer, just run it and follow the prompts.
3. Unsigned build: on first launch use **right-click → Open**, or allow it in
   *System Settings → Privacy & Security*.
4. Restart your host and rescan plugins.

### Usage
1. Insert FIELDROOM on the master bus (or a monitor bus).
2. Select your headphone model in the calibration panel.
3. Choose a room preset and speaker layout, or set distances and angles manually.
4. Adjust room damping and output trim to taste.
5. A/B the processed and bypassed signal with the host's bypass to hear the difference.

### Notes
- The plugin processes audio only; it does not phone home.
- Presets are stored with the project, user profiles are stored locally.
- Builds are produced automatically by GitHub Actions on every push to `main`.

---

## Українською

### Що це
FIELDROOM відтворює звучання відкаліброваних студійних моніторів в обробленій кімнаті — через звичайні студійні навушники. Це офлайн аудіоефект, безпечний для реального часу: без телеметрії, без мережі, без фонових сервісів.

### Вимоги
- Windows 10/11 (x64) або macOS 11+ (Universal 2: Apple Silicon + Intel)
- VST3- або AU-хост (Ableton Live, Reaper, Cubase, Logic Pro, Studio One, ...)

### Встановлення — Windows
1. Завантаж `FIELDROOM-Windows-x64.zip` з розділу [Releases](../../releases).
2. Розпакуй архів.
3. Скопіюй папку `FIELDROOM.vst3` у:
   `C:\Program Files\Common Files\VST3\`
4. Перезапусти DAW і зроби rescan плагінів.
5. Додай **FIELDROOM** (вендор: AcousticLab) на доріжку або мастер-шину.

> Якщо плагін не зʼявився — встанови Microsoft Visual C++ Redistributable (x64) і повтори сканування.

### Встановлення — macOS
1. Завантаж `FIELDROOM-macOS-Universal2.zip` (або `.pkg`-інсталятор) з [Releases](../../releases).
2. Розпакуй і скопіюй `FIELDROOM.vst3` у `/Library/Audio/Plug-Ins/VST3/`, а `FIELDROOM.component` — у `/Library/Audio/Plug-Ins/Components/`.
   З `.pkg`-інсталятором просто запусти його і дотримуйся підказок.
3. Збірка без підпису: при першому запуску — **правий клік → Open**, або дозволь у
   *System Settings → Privacy & Security*.
4. Перезапусти хост і зроби rescan плагінів.

### Як користуватися
1. Додай FIELDROOM на мастер-шину (або монітор-шину).
2. Обери модель навушників у панелі калібрації.
3. Обери пресет кімнати й розстановку моніторів, або задай відстані та кути вручну.
4. Налаштуй демпфування кімнати та вихідний рівень.
5. Порівняй оброблений і байпасний сигнал штатним bypass хоста.

### Примітки
- Плагін лише обробляє аудіо; жодних звернень до мережі.
- Пресети зберігаються з проєктом, користувацькі профілі — локально.
- Збірки створюються автоматично через GitHub Actions на кожен пуш у `main`.

---

## Build from source

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Requirements: CMake 3.22+, C++20 compiler, JUCE 7.0.9 (fetched automatically).

## License

See [LICENSES/LICENSE](LICENSES/LICENSE).
