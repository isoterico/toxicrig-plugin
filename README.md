# ToxicRig — amp sim + pedaliera (ToxicZone)

Plugin VST3/AU per chitarra elettrica: pedaliera riordinabile → 1 o 2 ampli valvolari
in parallelo → casse con convoluzione IR e microfono virtuale → effetti in loop.
Nome provvisorio: si cambia in `CMakeLists.txt` (`PRODUCT_NAME`).

## Struttura

```
dsp/tz/        Libreria DSP riusabile in tutto il bundle (C++17 puro, niente JUCE)
  Core.h         smoothing parametri, utilità
  Filters.h      biquad, one-pole TPT, DC blocker, allpass, IIR 3° ordine
  Tube.h         triodo 12AX7 (Koren + retta di carico), tone stack Yeh, finale push-pull con sag, ADAA
  Amp.h          ampli completi (British / American / Modern × Clean / Crunch / Lead)
  Pedals.h       gate, comp, treble booster, overdrive, fuzz, EQ, chorus, phaser, tremolo, delay, riverbero
  Cab.h          IR delle casse generate da modello + microfono virtuale
  DelayLine.h    linea di ritardo con interpolazione Hermite
  Tuner.h        accordatore (YIN)
plugin/        Wrapper JUCE: parametri, motore audio (oversampling, routing), preset, interfaccia
tests/         Banco di prova offline (DI sintetico, aliasing, click, latenza, CPU, render)
.github/       Build automatica Windows + macOS
```

## Compilare (automatico, con GitHub Actions)

Vedi le istruzioni passo passo nella chat / nel file `docs/ISTRUZIONI.md`.

## Compilare a mano

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target ToxicRig_VST3
```

Test offline: `cmake --build build --target ToxicRigTests` poi `ToxicRigTests test-out`.
