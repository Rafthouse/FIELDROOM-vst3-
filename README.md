# FIELDROOM
### Professional Headphone Monitoring VST3 / AU Audio Plugin
**Cross-Platform Binaural Virtual Loudspeaker & Acoustic Control Room Monitoring System**

[![Build Native VST3 & AU](https://github.com/USER/REPO/actions/workflows/build.yml/badge.svg)](https://github.com/USER/REPO/actions/workflows/build.yml)
[![JUCE](https://img.shields.io/badge/JUCE-7.0.9-orange.svg)](https://juce.com/)
[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Formats](https://img.shields.io/badge/Formats-VST3%20%7C%20AU-green.svg)](https://www.steinberg.net/vst3/)
[![Platforms](https://img.shields.io/badge/Platforms-macOS%20(Universal%202)%20%7C%20Windows%20(x64)-lightgrey.svg)]()

FIELDROOM is an offline, real-time safe audio effect plugin engineered for music producers, mixing/mastering engineers, and sound designers. It accurately reproduces listening to high-end calibrated studio monitors in acoustically treated environments through standard studio reference headphones.

---

## ⚡ Як отримати готові збірки (Windows / macOS) через GitHub Actions

Вам **не потрібно встановлювати компілятори** або збирати проєкт вручну. Завдяки налаштованому файлу `.github/workflows/build.yml` хмарні сервери GitHub роблять це автоматично:

1. **Створіть репозиторій на GitHub**:
   - Перейдіть на [github.com/new](https://github.com/new) та створіть новий репозиторій (наприклад `fieldroom-plugin`).
2. **Завантажте проєкт у репозиторій**:
   ```bash
   git remote add origin https://github.com/<ваш-юзернейм>/fieldroom-plugin.git
   git branch -M main
   git push -u origin main
   ```
3. **Завантажте готові VST3 / AU файли**:
   - Відкрийте вкладку **Actions** у вашому репозиторії на GitHub.
   - Натисніть на перший робочий процес **Build Native VST3 & AU Plugins**.
   - У розділі **Artifacts** знизу сторінки завантажте готові архіви:
     - 🍏 **`FIELDROOM-macOS-Universal2.zip`** (містить `FIELDROOM.vst3` та `FIELDROOM.component` для Apple Silicon M1/M2/M3/M4 та Intel).
     - 🪟 **`FIELDROOM-Windows-x64.zip`** (містить `FIELDROOM.vst3` для 64-бітних систем Windows 10/11).
4. **Створення офіційного релізу**:
   - Якщо створити Git-тег версії (наприклад `git tag v1.0.0 && git push origin v1.0.0`), GitHub Actions автоматично створить офіційний **GitHub Release** із прикріпленими файлами інсталяції.

---

## 🎧 Ключові можливості (Features)

1. **Тривимірна віртуальна сцена (Acoustic Stage)**:
   - **Nearfield (1.1 m / 60°)**: Фокусований прямий звук, мінімальний вплив кімнати, точний контроль транзієнтів та низьких частот.
   - **Midfield (2.2 m / 60°)**: Еталонний студійний трикутник, збалансована стереопанорама та ревербераційний баланс.
   - **Farfield (4.2 m / 60°)**: Масштаб головних контрольних моніторів (Main Monitors), перевірка клубного саунду та кінематографічної глибини.
   - **Регулювання кута розкриття моніторів (30° – 110°)**: Інтерактивне симетричне позиціонування віртуальних колонок.
   - **Обертання голови (±90°)**: Інспекція балансу та фантомного центру за моделлю Woodworth-Schlosser.

2. **Акустичні контрольні простори**:
   - **Neutral Reference**: Акустично заглушений контрольний простір (RT60: 0.15s).
   - **Small Studio**: Бюджетна проджект-студія з природним відгуком (RT60: 0.28s).
   - **Professional Control Room**: Студія звукозапису з дифузією та бас-пастками (RT60: 0.38s).
   - **Large Control Room**: Простора майстеринг-апаратна комерційного комплексу (RT60: 0.58s).
   - **Domestic Living Room**: Побутова кімната для фінальної перевірки трансляції міксу (RT60: 0.75s).

3. **Калібрування навушників (10-Band Biquad PEQ)**:
   - Заводські коригувальні профілі з бази **AutoEq / Oratory1990**:
     - **V-MODA M-200** (компенсація резонансу 540 Гц та відновлення вокальної ями 3.5 кГц / 8 кГц)
     - **Sennheiser HD 600 / HD 650**
     - **Beyerdynamic DT 770 Pro (80Ω) / DT 990 Pro (250Ω)**
     - **Audio-Technica ATH-M50x**
     - **Sony MDR-7506**
     - **AKG K371**
     - **Audeze LCD-X**
     - **Focal Clear MG Pro**
     - **Neumann NDH 30**
   - Автоматичний розрахунок цифрового запасу (Digital Headroom Auto-Preamp) для усунення спотворень.

4. **Апаратна консоль моніторингу**:
   - Маршрутизація: **STEREO**, **MONO** (-3 dB pan-law), **SIDE** ((L-R)/2), **L Solo**, **R Solo**, **L/R SWAP**.
   - Атенюатор **DIM -18 dB**.
   - Режим **EQ ONLY** (чиста корекція навушників без моделювання кімнати).
   - Функція **LOUDNESS MATCH** (вирівнювання гучності між Near, Mid та Far).
   - Фазовий корелометр (-1 ... +1) та пікметри з детектором кліпування.

---

## 🛠 Ручна компіляція (Local Build)

### macOS (Universal 2: Apple Silicon + Intel)
```bash
brew install cmake ninja
chmod +x scripts/build-macos.sh
./scripts/build-macos.sh
```
Шляхи встановлення:
- VST3: `/Library/Audio/Plug-Ins/VST3/FIELDROOM.vst3`
- AU: `/Library/Audio/Plug-Ins/Components/FIELDROOM.component`

### Windows 10 / 11 (Visual Studio 2022 x64)
```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```
Шлях встановлення:
- VST3: `C:\Program Files\Common Files\VST3\FIELDROOM.vst3`

---

## 📜 Ліцензія
GNU General Public License v3.0 (див. `LICENSES/LICENSE`). Copyright (c) 2026 FIELDROOM Audio Acoustics Lab.

Проєкт використовує JUCE 7 під GPLv3, тому вбудований редакторний знак JUCE вимкнено (`JUCE_DISPLAY_SPLASH_SCREEN=0`). Всі алгоритми виконані відповідно до стандарту реального часу (Zero-Allocation Audio Thread, Real-Time Safe).
