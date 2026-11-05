# Análisis del plugin original

Plugin analizado: `StereoExpander.dll` (VST 2.x, x86 de 32 bits, 352 256 bytes).
Metadatos de recursos: *Wavelab StereoExpander 1.1.0.0 — Spectral Design GmbH 2001*.
SHA-256: `2a825c27c51307e20cef17087502700d0a19e9f7b3225a0badcca03645c321f4`.

Herramienta: REA + Ghidra 12.1.4. Aquí **solo se documenta el comportamiento observado**; ni el código
decompilado ni la DLL forman parte de este repositorio.

## Interfaz VST2

| Campo | Valor |
|---|---|
| Export | `main` (VSTPluginMain clásico) |
| `uniqueID` | `0x53457853` (`'SExS'`) |
| Entradas / salidas | 2 / 2 |
| Parámetros | 1 (ancho, rango −100…+100 %, por defecto 0 %; normalizado 0…1, defecto 0.5) |
| Programas | 4 (todos "Init") |
| `initialDelay` | 0 (sin latencia) |

## Algoritmo

`w = (p + 100) / 100`, con `p` ∈ [−100, +100] %  →  `w` ∈ [0, 2].

**Entrada estéreo (las dos entradas conectadas)** — mid/side:

```
mid  = (L + R) / √2
side = (L − R) / √2 · w
L'   = (mid + side) / √2
R'   = (mid − side) / √2
```

`w = 1` es transparente, `w = 0` es mono, `w = 2` duplica el lado.

**Entrada mono (una sola entrada conectada)** — pseudo-estéreo:

```
f  = Butterworth_pasabanda(1 kHz, 3 kHz, orden 4)( retardo_7ms(x) )
L' = (x + w · f) · 0.9988149
R' = (x − w · f) · 0.9988149
```

* Retardo: `trunc(0.007 · fs)` muestras (308 a 44.1 kHz).
* Filtro: prototipo Butterworth de orden 4 (2 secciones), transformación paso-bajo → paso-banda y
  bilineal con *prewarping* `tan(π f / fs)`. Cada sección es un IIR de 4.º orden (5 coeficientes).

## Verificación

`StereoExpanderDSP.h` se comparó contra la DLL original cargada en un host VST2 de 32 bits propio:
ruido gaussiano, 20 480 muestras, parámetro en 0, 0.25, 0.5, 0.75 y 1 (estéreo) y 0.25, 0.5 y 1 (mono).

| Modo | Diferencia máxima | 
|---|---|
| Estéreo | 2.4·10⁻⁷ |
| Mono | 1.2·10⁻⁷ |

Esas diferencias son redondeo de coma flotante (float vs. el orden de operaciones de x87).

## Diferencias deliberadas

* **Suavizado del parámetro** (10 ms, lineal) en el plugin JUCE para evitar *zipper noise*. El original
  aplicaba el valor de golpe. Con el control quieto, la salida es la misma.
* **Interfaz**: el original usaba VSTGUI con mapas de bits propios; aquí es un control JUCE sencillo.
* Los 4 programas del original son idénticos ("Init"), por eso aquí hay un único programa.
