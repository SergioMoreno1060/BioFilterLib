# BioFilterLib

Librería de filtros digitales para Arduino Due orientada al procesamiento de bioseñales (ECG, EMG, EEG). Encapsula CMSIS-DSP en una API C++ sencilla sin sacrificar las optimizaciones ARM Cortex-M3.

![Arduino](https://img.shields.io/badge/Arduino-Due%20(SAM3X8E)-00979D?logo=arduino&logoColor=white)
![CMSIS-DSP](https://img.shields.io/badge/CMSIS--DSP-5.x-0091BD)
![Versión](https://img.shields.io/badge/versión-1.0.0-6bd8cb)
![Licencia](https://img.shields.io/badge/licencia-MIT-green)

---

## Filtros incluidos

| Clase | Tipo | CMSIS struct | Aplicaciones típicas |
|---|---|---|---|
| `FIRFilter` | FIR (fase lineal) | `arm_fir_instance_f32` | Pasa-bajas ECG, pasa-banda EMG |
| `IIRFilter` | IIR (biquad cascada) | `arm_biquad_casd_df1_inst_f32` | Notch 50/60 Hz, Butterworth |
| `LMSFilter` | NLMS adaptativo | `arm_lms_norm_instance_f32` | Cancelación de artefactos en tiempo real |
| `WaveletFilter` | DWT Daubechies-4 | 4 × `FIRFilter` | Denoising ECG/EEG multi-resolución |

---

## Requisitos

- **Hardware:** Arduino Due (ARM Cortex-M3 @ 84 MHz)
- **IDE:** Arduino IDE 1.8+ o Arduino IDE 2.x
- **Dependencia:** `arm_math.h` (incluida en el core SAM de Arduino Due)

> La librería **no funciona** en Arduino Uno/Mega/Nano (AVR). Solo arquitectura SAM (`architectures=sam`).

---

## Instalación

### Arduino IDE (recomendado)

1. Descarga el ZIP desde la página de releases.
2. Arduino IDE → **Sketch → Include Library → Add .ZIP Library…**
3. Selecciona el archivo descargado.

### Manual

Copia la carpeta `BioFilterLib_IDE` en tu directorio de librerías:

```
Windows:  C:\Users\<usuario>\Documents\Arduino\libraries\
macOS:    ~/Documents/Arduino/libraries/
Linux:    ~/Arduino/libraries/
```

---

## Uso rápido

```cpp
#include <BioFilterLib.h>

// Coeficientes pasa-bajas 40 Hz (generados con scipy o MATLAB)
float32_t coeffs[51] = { /* ... */ };

FIRFilter ecgFilter(coeffs, 51, 1);  // blockSize=1 para tiempo real

void loop() {
    float32_t raw = analogRead(A0) / 2047.5f - 1.0f;  // normalizar 12-bit ADC
    float32_t filtered = ecgFilter.processSample(raw);
    Serial.println(filtered, 6);
}
```

---

## API

### FIRFilter

```cpp
FIRFilter(float32_t* coeffs, uint16_t numTaps, uint16_t blockSize);

float32_t processSample(float32_t input);
void      processBuffer(float32_t* input, float32_t* output, uint32_t length);
```

### IIRFilter

```cpp
IIRFilter(float32_t* coeffs, uint8_t numStages, uint16_t blockSize);

float32_t processSample(float32_t input);
void      processBuffer(float32_t* input, float32_t* output, uint32_t length);
```

> **⚠ Coeficientes IIR:** CMSIS-DSP espera `{b0, b1, b2, a1, a2}` por etapa con `a1` y `a2` **negados** respecto a scipy/MATLAB. Multiplica por -1 antes de pasar los coeficientes `a`.

### LMSFilter

```cpp
LMSFilter(float32_t* coeffs, uint16_t numTaps, float32_t mu, uint16_t blockSize);

void      processSample(float32_t input, float32_t reference,
                        float32_t* output, float32_t* error);
void      processBuffer(float32_t* input,  float32_t* reference,
                        float32_t* output, float32_t* error, uint32_t length);
float32_t getMu() const;
void      setMu(float32_t newMu);
void      resetCoefficients(const float32_t* newCoeffs = nullptr);
```

| Señal | Rango de μ recomendado |
|---|---|
| ECG | 0.01 – 0.05 |
| EMG | 0.001 – 0.01 |
| EEG | 0.0001 – 0.001 |

### WaveletFilter (Daubechies-4)

```cpp
WaveletFilter(uint16_t blockSize);

void      processSample(float32_t input, float32_t* approxCoeff, float32_t* detailCoeff);
void      processBuffer(float32_t* input, float32_t* approx, float32_t* detail, uint32_t length);
float32_t reconstruct(float32_t approxCoeff, float32_t detailCoeff);
void      reset();
```

```cpp
// Denoising: reconstruir solo con la aproximación
float32_t clean = wavelet.reconstruct(approx, 0.0f);
```

---

## Ejemplos incluidos

| Sketch | Descripción |
|---|---|
| `Serial_results_BioFilterLib_FIR` | ECG filtrado con FIR pasa-bajas, visualización Serial Plotter |
| `Serial_results_BioFilterLib_IIR` | Notch 60 Hz con IIR biquad sobre señal sintética |
| `Serial_results_BioFilterLib_LMS` | Cancelación adaptativa de 60 Hz, 4 combinaciones M/μ |
| `Serial_results_BioFilterLib_Wavelet` | Descomposición y reconstrucción DWT con DB-4 y DB-8 |

Abre los sketches desde **File → Examples → BioFilterLib**.

---

## Consumo de RAM (Arduino Due, 96 KB total)

| Clase | Buffer de estado | Ejemplo (N=64, BS=1) |
|---|---|---|
| `FIRFilter` | `(numTaps + blockSize - 1) × 4 B` | 252 B |
| `IIRFilter` | `numStages × 4 × 4 B` | 32 B (2 etapas) |
| `LMSFilter` | `numTaps × 4 B` | 256 B |
| `WaveletFilter` | `4 × FIRFilter(8 taps)` | ~224 B |

---

## Documentación completa

**[SergioMoreno1060.github.io/BioFilterLib](https://SergioMoreno1060.github.io/BioFilterLib_IDE/)**

Incluye explicaciones matemáticas (KaTeX), diagramas de arquitectura CMSIS-DSP y ejemplos comentados para ECG, EMG y EEG.

---

## Estructura del repositorio

```
BioFilterLib_IDE/
├── src/
│   ├── BioFilterLib.h          # Header principal
│   ├── filters/
│   │   ├── FIRFilter.h / .cpp
│   │   ├── IIRFilter.h / .cpp
│   │   ├── LMSFilter.h / .cpp
│   │   └── WaveletFilter.h / .cpp
│   └── utils/
│       ├── utils.h / .cpp       # SNR, MSE, RMS, correlación
│       ├── utils_extended.h     # Benchmarking y métricas de calidad
│       └── Waveforms.h          # Señales de prueba sintéticas
├── examples/                    # Sketches con datos de Serial Plotter
├── test/                        # Sketches de test funcional
├── docs/                        # Sitio GitHub Pages
└── library.properties
```

---

## Licencia

MIT © 2024 Sergio Moreno
