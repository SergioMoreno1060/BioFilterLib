# BioFilterLib

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Arduino%20Due-orange.svg)

BioFilterLib es una biblioteca de filtros digitales optimizada para Arduino Due, diseñada específicamente para el procesamiento de bioseñales en tiempo real. Esta librería proporciona una capa de abstracción sobre CMSIS-DSP, simplificando el uso de algoritmos avanzados de procesamiento digital de señales en aplicaciones biomédicas.

## Características

- **Optimización para bioseñales**: Filtros preconfigurados para ECG, EMG, EEG y otras señales biomédicas.
- **Alto rendimiento**: Implementación optimizada utilizando CMSIS-DSP para ARM Cortex-M3.
- **Facilidad de uso**: API simplificada que oculta la complejidad de CMSIS-DSP.
- **Múltiples tipos de filtros**:
  - Filtros FIR (respuesta impulsional finita)
  - Filtros IIR (respuesta impulsional infinita)
  - Filtros Notch para eliminación de ruido de línea eléctrica
  - Filtros adaptativos LMS
  - Filtros Wavelet para análisis multiresolución
  - Filtros de Mediana para eliminación de artefactos

## Requisitos

- Arduino Due
- Visual Studio Code
- Extension PlatformIO

## Instalación

### Instalación manual

1. Descarga este repositorio como archivo ZIP
2. Colocar el archivo comprimido en la carpeta lib/ de tu proyecto de PlatformIO
3. Descomprimir el archivo ZIP

### Instalación de dependencias

BioFilterLib requiere la biblioteca CMSIS-DSP para funcionar. Al descargar este repositorio se descargan los archivos necesarios de CMSIS 5 y CMSIS DSP.

## Uso básico

### Importar la biblioteca

```cpp
#include <BioFilterLib.h>
```

### Ejemplo básico: Filtro FIR para ECG

```cpp
#include <Arduino.h>
#include <BioFilterLib.h>
#include "Waveforms.h"

// Parámetros del filtro
#define BLOCK_SIZE  32
#define NUM_TAPS    51
#define SAMPLE_RATE 1000  // Hz típico para ECG
#define CUTOFF_FREQ 50   // Hz - frecuencia de corte para eliminar ruido

// Coeficientes FIR simulados (pasa bajas, por ejemplo)
const float32_t ecgFilterCoeffs[NUM_TAPS] = {
  1.01602337e-03,  1.05219578e-03,  1.05485683e-03,  9.52665359e-04,
  6.39612342e-04, -6.52666866e-19, -1.05692964e-03, -2.55869546e-03,
  -4.43506165e-03, -6.49496992e-03, -8.42139827e-03, -9.78815640e-03,
  -1.00992192e-02, -8.84737843e-03, -5.58538708e-03,  2.65242472e-18,
  8.02209630e-03,  1.83472206e-02,  3.05752787e-02,  4.40532964e-02,
  5.79227190e-02,  7.11964426e-02,  8.28570731e-02,  9.19645190e-02,
  9.77592660e-02,  9.97478615e-02,  9.77592660e-02,  9.19645190e-02,
  8.28570731e-02,  7.11964426e-02,  5.79227190e-02,  4.40532964e-02,
  3.05752787e-02,  1.83472206e-02,  8.02209630e-03,  2.65242472e-18,
  -5.58538708e-03, -8.84737843e-03, -1.00992192e-02, -9.78815640e-03,
  -8.42139827e-03, -6.49496992e-03, -4.43506165e-03, -2.55869546e-03,
  -1.05692964e-03, -6.52666866e-19,  6.39612342e-04,  9.52665359e-04,
  1.05485683e-03,  1.05219578e-03,  1.01602337e-03
};

// Buffers para procesamiento
float32_t inputSignal[maxSamplesNum];
float32_t filteredSignal[maxSamplesNum];

// Instancia del filtro
FIRFilter* ecgFilter;

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("=== BioFilterLib ECG Filtering Demo ===");
    Serial.println("Filtering noisy ECG signal with FIR low-pass filter");
    Serial.println("Filter specs: 51 taps, fc=40Hz, fs=250Hz");
    Serial.println();
    
    // Convertir señal ECG de uint16_t a float32_t y normalizar
    Serial.println("Converting and normalizing ECG signal...");
    for (int i = 0; i < maxSamplesNum; i++) {
        // Normalizar de rango 0-4095 (12-bit) a rango ±1.0
        inputSignal[i] = ((float32_t)waveformsTable[4][i] - 2048.0f) / 2048.0f;
    }
    
    // Inicializar filtro FIR
    Serial.println("Initializing FIR filter...");
    ecgFilter = new FIRFilter(ecgFilterCoeffs, NUM_TAPS, BLOCK_SIZE);
    
    // Procesar señal por bloques
    Serial.println("Processing signal...");
    uint32_t samplesProcessed = 0;
    
    while (samplesProcessed < maxSamplesNum) {
        uint32_t currentBlockSize = min((uint32_t)BLOCK_SIZE, maxSamplesNum - samplesProcessed);
        ecgFilter->processBuffer(&inputSignal[samplesProcessed], 
                                 &filteredSignal[samplesProcessed], 
                                 currentBlockSize);
        samplesProcessed += currentBlockSize;
    }
    
    Serial.println("Filtering complete!");
    Serial.println();
    
    // Mostrar header para CSV
    Serial.println("Sample,Original,Filtered");
    
    // Mostrar datos en formato CSV
    for (int i = 0; i < maxSamplesNum; i++) {
        Serial.print(i);
        Serial.print(",");
        Serial.print(inputSignal[i], 6);
        Serial.print(",");
        Serial.println(filteredSignal[i], 6);
        
        // Pequeña pausa para evitar overflow del buffer serial
        if (i % 50 == 0) {
            delay(10);
        }
    }
    
    Serial.println();
    Serial.println("=== Data transmission complete ===");
    Serial.println("You can now save this data to CSV using:");
    Serial.println("python -c \"import serial; s=serial.Serial('COM6',115200); [print(s.readline().decode().strip()) for _ in range(1010)]\" > ecg_data.csv");
}

void loop() {
    // Demostración en tiempo real (opcional)
    static unsigned long lastTime = 0;
    static int sampleIndex = 0;
    
    if (millis() - lastTime > 1) {  // Simular fs=1000Hz (1ms entre muestras)
        if (sampleIndex < maxSamplesNum) {
            float32_t currentSample = inputSignal[sampleIndex];
            float32_t filteredSample = ecgFilter->processSample(currentSample);
            
            // Mostrar muestra en tiempo real (comentar si no se necesita)
            /*
            Serial.print("RT: ");
            Serial.print(sampleIndex);
            Serial.print(",");
            Serial.print(currentSample, 4);
            Serial.print(",");
            Serial.println(filteredSample, 4);
            */
            
            sampleIndex++;
            if (sampleIndex >= maxSamplesNum) {
                sampleIndex = 0;  // Reiniciar para loop continuo
            }
        }
        lastTime = millis();
    }
}

```

## Documentación de la API

### Clase FIRFilter

```cpp
FIRFilter();
~FIRFilter();
void init(const q15_t *coeffs, uint32_t numTaps, q15_t *state, uint32_t blockSize);
void process(const q15_t *input, q15_t *output, uint32_t blockSize);
static void filterStatic(const q15_t *input, q15_t *output, const q15_t *coeffs, uint32_t numTaps, q15_t *state, uint32_t blockSize);
void end();
```

## Tipos de filtros y sus aplicaciones

### Filtros FIR

Los filtros FIR (Finite Impulse Response) son ideales para:
- Procesamiento de ECG y EEG donde la preservación de la fase es crítica
- Eliminación de artefactos de baja frecuencia (deriva de línea base)
- Filtrado de banda para aislar componentes específicos de bioseñales

### Filtros IIR

Los filtros IIR (Infinite Impulse Response) son más eficientes computacionalmente y útiles para:
- Aplicaciones que requieren órdenes de filtro más bajos
- Filtrado paso-alto y paso-bajo con pendientes pronunciadas
- Situaciones donde la distorsión de fase es aceptable

### Filtros Notch

Especializados en eliminar interferencias de frecuencia específica:
- Eliminar ruido de línea eléctrica (50/60Hz)
- Suprimir artefactos de frecuencia fija en EEG y ECG
- Eliminar interferencias de equipos médicos cercanos

### Filtros LMS Adaptativos

Útiles en situaciones donde las características del ruido cambian:
- Cancelación adaptativa de ruido en EMG
- Eliminación de artefactos de movimiento
- Separación de señales materno-fetales en ECG

### Filtros Wavelet

Permiten análisis tiempo-frecuencia y son efectivos para:
- Detección de eventos específicos en EEG (como puntas epilépticas)
- Compresión de señales biomédicas
- Eliminación de ruido preservando características morfológicas importantes

### Filtros de Mediana

Especialmente útiles para:
- Eliminación de artefactos impulsivos (spikes)
- Reducción de ruido en ECG preservando complejos QRS
- Preprocesamiento de señales biomédicas contaminadas con ruido impulsivo

## Ejemplos avanzados

### Filtrado completo de ECG

```cpp
#include <Arduino.h>
#include <BioFilterLib.h>
#include "Waveforms.h"

// Parámetros del filtro
#define BLOCK_SIZE  32
#define NUM_TAPS    51
#define SAMPLE_RATE 1000  // Hz típico para ECG
#define CUTOFF_FREQ 50   // Hz - frecuencia de corte para eliminar ruido

// Coeficientes FIR simulados (pasa bajas, por ejemplo)
const float32_t ecgFilterCoeffs[NUM_TAPS] = {
  1.01602337e-03,  1.05219578e-03,  1.05485683e-03,  9.52665359e-04,
  6.39612342e-04, -6.52666866e-19, -1.05692964e-03, -2.55869546e-03,
  -4.43506165e-03, -6.49496992e-03, -8.42139827e-03, -9.78815640e-03,
  -1.00992192e-02, -8.84737843e-03, -5.58538708e-03,  2.65242472e-18,
  8.02209630e-03,  1.83472206e-02,  3.05752787e-02,  4.40532964e-02,
  5.79227190e-02,  7.11964426e-02,  8.28570731e-02,  9.19645190e-02,
  9.77592660e-02,  9.97478615e-02,  9.77592660e-02,  9.19645190e-02,
  8.28570731e-02,  7.11964426e-02,  5.79227190e-02,  4.40532964e-02,
  3.05752787e-02,  1.83472206e-02,  8.02209630e-03,  2.65242472e-18,
  -5.58538708e-03, -8.84737843e-03, -1.00992192e-02, -9.78815640e-03,
  -8.42139827e-03, -6.49496992e-03, -4.43506165e-03, -2.55869546e-03,
  -1.05692964e-03, -6.52666866e-19,  6.39612342e-04,  9.52665359e-04,
  1.05485683e-03,  1.05219578e-03,  1.01602337e-03
};

// Buffers para procesamiento
float32_t inputSignal[maxSamplesNum];
float32_t filteredSignal[maxSamplesNum];

// Instancia del filtro
FIRFilter* ecgFilter;

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("=== BioFilterLib ECG Filtering Demo ===");
    Serial.println("Filtering noisy ECG signal with FIR low-pass filter");
    Serial.println("Filter specs: 51 taps, fc=40Hz, fs=250Hz");
    Serial.println();
    
    // Convertir señal ECG de uint16_t a float32_t y normalizar
    Serial.println("Converting and normalizing ECG signal...");
    for (int i = 0; i < maxSamplesNum; i++) {
        // Normalizar de rango 0-4095 (12-bit) a rango ±1.0
        inputSignal[i] = ((float32_t)waveformsTable[4][i] - 2048.0f) / 2048.0f;
    }
    
    // Inicializar filtro FIR
    Serial.println("Initializing FIR filter...");
    ecgFilter = new FIRFilter(ecgFilterCoeffs, NUM_TAPS, BLOCK_SIZE);
    
    // Procesar señal por bloques
    Serial.println("Processing signal...");
    uint32_t samplesProcessed = 0;
    
    while (samplesProcessed < maxSamplesNum) {
        uint32_t currentBlockSize = min((uint32_t)BLOCK_SIZE, maxSamplesNum - samplesProcessed);
        ecgFilter->processBuffer(&inputSignal[samplesProcessed], 
                                 &filteredSignal[samplesProcessed], 
                                 currentBlockSize);
        samplesProcessed += currentBlockSize;
    }
    
    Serial.println("Filtering complete!");
    Serial.println();
    
    // Mostrar header para CSV
    Serial.println("Sample,Original,Filtered");
    
    // Mostrar datos en formato CSV
    for (int i = 0; i < maxSamplesNum; i++) {
        Serial.print(i);
        Serial.print(",");
        Serial.print(inputSignal[i], 6);
        Serial.print(",");
        Serial.println(filteredSignal[i], 6);
        
        // Pequeña pausa para evitar overflow del buffer serial
        if (i % 50 == 0) {
            delay(10);
        }
    }
    
    Serial.println();
    Serial.println("=== Data transmission complete ===");
    Serial.println("You can now save this data to CSV using:");
    Serial.println("python -c \"import serial; s=serial.Serial('COM6',115200); [print(s.readline().decode().strip()) for _ in range(1010)]\" > ecg_data.csv");
}

void loop() {
    // Demostración en tiempo real (opcional)
    static unsigned long lastTime = 0;
    static int sampleIndex = 0;
    
    if (millis() - lastTime > 1) {  // Simular fs=1000Hz (1ms entre muestras)
        if (sampleIndex < maxSamplesNum) {
            float32_t currentSample = inputSignal[sampleIndex];
            float32_t filteredSample = ecgFilter->processSample(currentSample);
            
            // Mostrar muestra en tiempo real (comentar si no se necesita)
            /*
            Serial.print("RT: ");
            Serial.print(sampleIndex);
            Serial.print(",");
            Serial.print(currentSample, 4);
            Serial.print(",");
            Serial.println(filteredSample, 4);
            */
            
            sampleIndex++;
            if (sampleIndex >= maxSamplesNum) {
                sampleIndex = 0;  // Reiniciar para loop continuo
            }
        }
        lastTime = millis();
    }
}

```

## Configuraciones recomendadas para bioseñales comunes

### Electrocardiograma (ECG)
- **Frecuencia de muestreo**: 250-1000 Hz
- **Ancho de banda**: 0.5-40 Hz para monitorización general, 0.05-150 Hz para diagnóstico
- **Filtros recomendados**:
  - Filtro FIR paso-alto (0.5 Hz) para eliminar deriva de línea base
  - Filtro Notch (50/60 Hz) para interferencia de red eléctrica
  - Filtro de Mediana para eliminar artefactos

### Electromiograma (EMG)
- **Frecuencia de muestreo**: 1000-2000 Hz
- **Ancho de banda**: 20-500 Hz
- **Filtros recomendados**:
  - Filtro IIR paso-banda (20-500 Hz)
  - Filtro Notch (50/60 Hz)
  - Filtro LMS adaptativo para artefactos de movimiento

### Electroencefalograma (EEG)
- **Frecuencia de muestreo**: 250-500 Hz
- **Ancho de banda**: 0.5-100 Hz (banda completa)
- **Sub-bandas relevantes**:
  - Delta: 0.5-4 Hz
  - Theta: 4-8 Hz
  - Alpha: 8-13 Hz
  - Beta: 13-30 Hz
  - Gamma: > 30 Hz
- **Filtros recomendados**:
  - Filtro FIR para preservación de fase
  - Filtro Wavelet para análisis de sub-bandas
  - Filtro LMS adaptativo para artefactos oculares

## Consideraciones de rendimiento

BioFilterLib está optimizada para el Arduino Due, pero aún así hay limitaciones de recursos que deben considerarse:

- **Memoria RAM**: El Arduino Due tiene 96KB de SRAM. Los buffers de estado y los coeficientes de filtro consumen memoria RAM, especialmente para filtros FIR de orden alto.
- **Tiempo de procesamiento**: Los filtros más complejos (como Wavelets) requieren más tiempo de cálculo. Monitorice los tiempos de ejecución para asegurar que se mantiene dentro de los requisitos de tiempo real.
- **Precisión numérica**: Las implementaciones utilizan formatos de punto fijo (q15_t, q31_t) para maximizar el rendimiento. En señales con gran rango dinámico, considere la posibilidad de normalización previa.

## Debugging y solución de problemas

### Problemas comunes

1. **Errores de compilación relacionados con CMSIS-DSP**
   - Verifique que la ruta de inclusión es correcta
   - Verificar que el archivo platformio.ini tenga lo siguiente:
     ```cpp
     [env:due]
      platform = atmelsam
      board = due
      framework = arduino
      
      build_flags =
          -DARM_MATH_CM3
          -Ilib/CMSIS-DSP/Include
          -Ilib/CMSIS-DSP/Include/DSP
          -Ilib/CMSIS_5/CMSIS/Core/Include
          -Ilib/BioFilterLib/src/utils
          -Ilib/BioFilterLib/src/filters
     ```

2. **Saturación de señal**
   - Las bioseñales pueden tener componentes DC que causan saturación
   - Utilice un filtro paso-alto para eliminar componentes DC antes de amplificar

3. **Inestabilidad en filtros IIR**
   - Los filtros IIR pueden volverse inestables con ciertas configuraciones
   - Reduzca el orden del filtro o utilice estructuras más estables (como biquad en cascada)

4. **Memoria insuficiente**
   - Reduzca el tamaño de los buffers o el orden de los filtros
   - Considere la posibilidad de procesar los datos en bloques más pequeños

### Verificación de funcionamiento

Para verificar el correcto funcionamiento de los filtros:

1. Utilice las señales de prueba conocidas (sinusoides, pulsos, etc.) que están en el archivo Waveforms.h
2. Compare la salida con resultados simulados en MATLAB/Python
3. Visualice la entrada y salida mediante la exportacion del output a un archivo .csv
4. Analice el espectro de frecuencia de la señal antes y después del filtrado con herramientas como scipy.

<br>

<div align="center">
  <img src="https://github.com/user-attachments/assets/c4e7913f-f17a-4331-a17b-4e8a701398da" alt="arduino_laptop_signal_simulator" width="600" />
  <p>Conexión Arduino Due con generador de señales y osciloscopio</p>
</div>

## Contribución

Las contribuciones a BioFilterLib son bienvenidas. Por favor, siga estos pasos:

1. Fork del repositorio
2. Cree una nueva rama (`git checkout -b feature/amazing-feature`)
3. Realice sus cambios y añada pruebas para ellos
4. Compruebe que el código pasa las pruebas existentes
5. Commit de sus cambios (`git commit -m 'Add some amazing feature'`)
6. Push a la rama (`git push origin feature/amazing-feature`)
7. Abra un Pull Request

## Licencia

Este proyecto está licenciado bajo la Licencia MIT - vea el archivo [LICENSE](LICENSE) para más detalles.

## Citar este proyecto

Si utiliza BioFilterLib en su investigación, por favor cite:

```
Moreno, S.E. (2025). BioFilterLib: Una librería de filtros digitales optimizada para procesamiento de bioseñales en Arduino Due. [Software].
```

## Contacto

Si tiene preguntas, sugerencias o comentarios, por favor abra un issue en este repositorio.
