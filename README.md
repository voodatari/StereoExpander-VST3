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

## AAX (Pro Tools)

El proyecto ya está preparado, pero **AAX no se compila por defecto** porque necesita el SDK de Avid.

1. Crea una cuenta en <https://developer.avid.com> y descarga el *AAX SDK* (licencia propia de Avid:
   **no lo subas a un repositorio público**).
2. Local: `cmake -S . -B build -DAAX_SDK_PATH=/ruta/al/aax-sdk` y compila; sale `Stereo Expander.aaxplugin`.
3. GitHub Actions: sube el SDK a un repo **privado** tuyo, y en este repo define la variable
   `AAX_SDK_REPO` (por ejemplo `voodatari/aax-sdk`) y el secreto `AAX_SDK_TOKEN` (PAT con lectura).
   El CI generará `StereoExpander-AAX-UNSIGNED-*.zip`.
4. **Firma:** Pro Tools comercial solo carga AAX firmados con PACE (`wraptool`, vía tu cuenta de
   desarrollador de Avid). Sin firma solo funciona en *Pro Tools Developer*. El CI **no firma**.
   Para M4: Pro Tools es nativo Apple Silicon desde 2022.12 y el binario sale arm64.
   Instalación: `/Library/Application Support/Avid/Audio/Plug-Ins/`.

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
