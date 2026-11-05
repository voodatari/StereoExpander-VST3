# Stereo Expander (VST3 / AU, Windows x64 + macOS Apple Silicon)

Reimplementación en JUCE 8 de un ensanchador estéreo VST2 de 32 bits de 2001. Formatos: **VST3**, **AU** y
**Standalone**. Compila en **Windows x64** y **macOS arm64** (M1–M4) con GitHub Actions.

El comportamiento se dedujo analizando el plugin original y está contrastado muestra a muestra contra él
(diferencia máxima ~10⁻⁷). Detalles en [docs/ANALISIS.md](docs/ANALISIS.md).

## Qué hace

Un parámetro, **Width**, de −100 % a +100 % (0 % = sin cambios):

* **Entrada estéreo:** ensanchador mid/side. −100 % = mono, +100 % = lado ×2.
* **Entrada mono:** pseudo-estéreo (retardo 7 ms + pasa-banda 1–3 kHz, sumado y restado a L/R).

## Compilar

Requisitos: CMake ≥ 3.22 y un compilador C++17 (MSVC o Xcode). JUCE se descarga solo.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Los binarios quedan en `build/StereoExpander_artefacts/Release/`.
Tests del DSP (sin JUCE): `g++ -std=c++17 tests/dsp_test.cpp -o dsp_test && ./dsp_test`.

## GitHub Actions

`.github/workflows/build.yml` ejecuta los tests y compila Windows x64 y macOS arm64 en cada *push*.
Los `.zip` salen como artefactos de la ejecución. Si empujas una etiqueta `v*` (por ejemplo `v1.1.0`) se
crea además un *release* en borrador.

## Instalar en macOS

Los binarios llevan firma *ad-hoc* (sin cuenta de desarrollador de Apple), así que Gatekeeper los pone en
cuarentena al descargarlos. Después de descomprimir:

```bash
xattr -cr "Stereo Expander.vst3"
cp -R "Stereo Expander.vst3" ~/Library/Audio/Plug-Ins/VST3/
```

(AU: `~/Library/Audio/Plug-Ins/Components/`). Para distribuirlo sin esos pasos hace falta firmar con un
Developer ID y notarizar.

## Aviso legal

El plugin original es software comercial de terceros (© Spectral Design GmbH / Steinberg). Este repositorio
contiene **código propio**: ni la DLL original ni código decompilado se incluyen. Las ideas y
algoritmos (mid/side, retardo, Butterworth) no son material con copyright, pero el nombre y la imagen del
producto original sí pueden estar protegidos: **no publiques ni vendas esto con el nombre del original**
sin permiso. Por eso el repo es privado.
