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

- Arduino Due (basado en ARM Cortex-M3)
- Arduino IDE 1.8.0 o superior
- [CMSIS-DSP Library](https://arm-software.github.io/CMSIS_5/DSP/html/index.html)

## Instalación

### Instalación manual

1. Descarga este repositorio como archivo ZIP
2. En el IDE de Arduino, ve a Sketch -> Include Library -> Add .ZIP Library...
3. Selecciona el archivo ZIP descargado
4. Reinicia el IDE de Arduino

### Instalación de dependencias

BioFilterLib requiere la biblioteca CMSIS-DSP para funcionar. Para instalarla:

1. Descarga la biblioteca CMSIS-DSP desde [aquí](https://github.com/ARM-software/CMSIS_5)
2. Sigue las instrucciones de instalación para Arduino Due

## Uso básico

### Importar la biblioteca

```cpp
#include <BioFilterLib.h>
```

### Ejemplo básico: Filtro FIR para ECG

```cpp
#include <BioFilterLib.h>

// Declarar filtro
FIRFilter ecgFilter;

// Buffers para estado y coeficientes
q15_t firState[ECG_FIR_TAPS + BLOCK_SIZE - 1];
q15_t firCoeffs[ECG_FIR_TAPS];

void setup() {
  Serial.begin(115200);
  
  // Generar coeficientes para un filtro paso banda para ECG (0.5-40Hz)
  // Los coeficientes deberían ser generados con herramientas como MATLAB o Python
  // y luego copiados aquí como constantes

  // Inicializar el filtro
  ecgFilter.init(firCoeffs, ECG_FIR_TAPS, firState, BLOCK_SIZE);
}

void loop() {
  // Leer datos del ADC
  q15_t input[BLOCK_SIZE];
  q15_t output[BLOCK_SIZE];
  
  // Leer datos (ejemplo simplificado)
  for(int i = 0; i < BLOCK_SIZE; i++) {
    input[i] = analogRead(A0);
    delay(1); // 1ms entre muestras = 1kHz
  }
  
  // Aplicar filtro
  ecgFilter.process(input, output, BLOCK_SIZE);
  
  // Enviar datos filtrados por Serial
  for(int i = 0; i < BLOCK_SIZE; i++) {
    Serial.println(output[i]);
  }
}
```

### Ejemplo: Filtro Notch para eliminar interferencia de red (50/60Hz)

```cpp
#include <BioFilterLib.h>

NotchFilter notchFilter;
q31_t notchState[4]; // 2 estados por sección biquad
q31_t notchCoeffs[5]; // 5 coeficientes para una sección biquad

void setup() {
  Serial.begin(115200);
  
  // Configurar coeficientes para un filtro Notch a 50Hz
  // Con frecuencia de muestreo de 1kHz y Q=30
  
  // Coeficientes calculados previamente para un filtro Notch de 50Hz
  // Los valores exactos dependen de la frecuencia de muestreo y factor de calidad deseados
  
  // Inicializar el filtro
  notchFilter.init(notchCoeffs, notchState);
}

void loop() {
  // Leer datos del ADC
  q31_t input[BLOCK_SIZE];
  q31_t output[BLOCK_SIZE];
  
  // Adquirir datos...
  
  // Aplicar filtro
  notchFilter.process(input, output, BLOCK_SIZE);
  
  // Procesar salida...
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

### Clase IIRFilter

```cpp
IIRFilter();
~IIRFilter();
void init(const q31_t *coeffs, q31_t *state, uint32_t numStages);
void process(const q31_t *input, q31_t *output, uint32_t blockSize);
static void filterStatic(const q31_t *input, q31_t *output, const q31_t *coeffs, q31_t *state, uint32_t numStages, uint32_t blockSize);
void end();
```

### Clase NotchFilter

```cpp
NotchFilter();
~NotchFilter();
void init(const q31_t *coeffs, q31_t *state);
void process(const q31_t *input, q31_t *output, uint32_t blockSize);
static void filterStatic(const q31_t *input, q31_t *output, const q31_t *coeffs, q31_t *state, uint32_t blockSize);
void end();
```

### Clase LMSFilter

```cpp
LMSFilter();
~LMSFilter();
void init(const q15_t *coeffs, q15_t *state, uint32_t numTaps, q15_t mu);
void process(const q15_t *input, const q15_t *desired, q15_t *output, uint32_t blockSize);
static void filterStatic(const q15_t *input, const q15_t *desired, q15_t *output, const q15_t *coeffs, q15_t *state, uint32_t numTaps, q15_t mu, uint32_t blockSize);
void end();
```

### Clase WaveletFilter

```cpp
WaveletFilter();
~WaveletFilter();
void init(const q15_t *coeffs, uint32_t numTaps, q15_t *state, uint32_t blockSize);
void process(const q15_t *input, q15_t *output, uint32_t blockSize);
static void filterStatic(const q15_t *input, q15_t *output, const q15_t *coeffs, uint32_t numTaps, q15_t *state, uint32_t blockSize);
void end();
```

### Clase MedianFilter

```cpp
MedianFilter();
~MedianFilter();
void init(uint32_t windowSize);
void process(const q15_t *input, q15_t *output, uint32_t blockSize);
static void filterStatic(const q15_t *input, q15_t *output, uint32_t windowSize, uint32_t blockSize);
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
#include <BioFilterLib.h>

// Definiciones y constantes
#define SAMPLE_RATE 500
#define BLOCK_SIZE 32
#define ECG_FIR_TAPS 101

// Instancias de filtros
FIRFilter baselineFilter;       // Para eliminar deriva de línea base
NotchFilter powerLineFilter;    // Para eliminar interferencia de red eléctrica
MedianFilter spikeFilter;       // Para eliminar artefactos impulsivos

// Buffers de estado y coeficientes
q15_t baselineState[ECG_FIR_TAPS + BLOCK_SIZE - 1];
q15_t baselineCoeffs[ECG_FIR_TAPS];
q31_t notchState[4];
q31_t notchCoeffs[5];

// Buffers de datos
q15_t rawData[BLOCK_SIZE];
q15_t tempData[BLOCK_SIZE];
q31_t notchInput[BLOCK_SIZE];
q31_t notchOutput[BLOCK_SIZE];
q15_t filteredData[BLOCK_SIZE];

void setup() {
  Serial.begin(115200);
  
  // Inicializar filtros
  // ... configurar coeficientes ...
  
  baselineFilter.init(baselineCoeffs, ECG_FIR_TAPS, baselineState, BLOCK_SIZE);
  powerLineFilter.init(notchCoeffs, notchState);
  spikeFilter.init(5); // Ventana de 5 muestras
  
  // Configurar ADC
  analogReadResolution(12);
}

void loop() {
  // Adquirir datos
  for(int i = 0; i < BLOCK_SIZE; i++) {
    rawData[i] = analogRead(A0);
    delay(1000/SAMPLE_RATE);
  }
  
  // Aplicación en cascada de filtros
  
  // 1. Eliminar artefactos impulsivos con filtro de mediana
  spikeFilter.process(rawData, tempData, BLOCK_SIZE);
  
  // 2. Eliminar interferencia de red eléctrica con filtro notch
  for(int i = 0; i < BLOCK_SIZE; i++) {
    notchInput[i] = (q31_t)tempData[i] << 16; // Escalar a q31_t
  }
  powerLineFilter.process(notchInput, notchOutput, BLOCK_SIZE);
  
  // 3. Eliminar deriva de línea base con filtro FIR paso-alto
  for(int i = 0; i < BLOCK_SIZE; i++) {
    tempData[i] = (q15_t)(notchOutput[i] >> 16); // Volver a q15_t
  }
  baselineFilter.process(tempData, filteredData, BLOCK_SIZE);
  
  // Enviar datos procesados
  for(int i = 0; i < BLOCK_SIZE; i++) {
    Serial.println(filteredData[i]);
  }
}
```

### Implementación de filtro adaptativo LMS para eliminación de artefactos

```cpp
#include <BioFilterLib.h>

#define SAMPLE_RATE 1000
#define BLOCK_SIZE 32
#define LMS_TAPS 32

LMSFilter adaptiveFilter;
q15_t lmsState[LMS_TAPS + BLOCK_SIZE - 1];
q15_t lmsCoeffs[LMS_TAPS];

q15_t primaryInput[BLOCK_SIZE];    // Señal contaminada (ECG + ruido)
q15_t referenceInput[BLOCK_SIZE];  // Señal de referencia (aproximación del ruido)
q15_t filteredOutput[BLOCK_SIZE];  // Salida limpia

void setup() {
  Serial.begin(115200);
  
  // Inicializar coeficientes en cero
  for(int i = 0; i < LMS_TAPS; i++) {
    lmsCoeffs[i] = 0;
  }
  
  // Inicializar filtro adaptativo con paso de adaptación μ = 0.01
  q15_t mu = 0.01 * 32768; // Escalar a formato q15_t
  adaptiveFilter.init(lmsCoeffs, lmsState, LMS_TAPS, mu);
  
  // Configurar ADCs
  analogReadResolution(12);
}

void loop() {
  // Leer señal principal (contaminada) del canal A0
  for(int i = 0; i < BLOCK_SIZE; i++) {
    primaryInput[i] = analogRead(A0);
    delay(1);
  }
  
  // Leer señal de referencia (aproximación del ruido) del canal A1
  for(int i = 0; i < BLOCK_SIZE; i++) {
    referenceInput[i] = analogRead(A1);
  }
  
  // Aplicar filtro adaptativo LMS
  adaptiveFilter.process(referenceInput, primaryInput, filteredOutput, BLOCK_SIZE);
  
  // Enviar datos filtrados
  for(int i = 0; i < BLOCK_SIZE; i++) {
    Serial.println(filteredOutput[i]);
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
   - Asegúrese de que CMSIS-DSP está correctamente instalado
   - Verifique que la ruta de inclusión es correcta

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

1. Utilice señales de prueba conocidas (sinusoides, pulsos, etc.)
2. Compare la salida con resultados simulados en MATLAB/Python
3. Visualice la entrada y salida en tiempo real mediante Serial Plotter
4. Analice el espectro de frecuencia de la señal antes y después del filtrado

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
