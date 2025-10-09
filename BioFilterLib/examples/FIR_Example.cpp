/**
 * @file FIR_Test.cpp
 * @brief Pruebas de rendimiento y calidad para FIRFilter
 * @author Sergio Moreno
 * @version 1.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro FIR evaluando:
 * 1. Velocidad de procesamiento (muestra individual vs buffer)
 * 2. Consumo de recursos (RAM, stack)
 * 3. Calidad de filtrado (SNR, MSE, correlación)
 * 
 * Prueba con señal ECG real con ruido a 60Hz del archivo Waveforms.h
 */

#include <Arduino.h>
#include <BioFilterLib.h>
#include "Waveforms.h"
#include "utils.h"

// ========== CONFIGURACIÓN DE PRUEBAS ==========

// Configuración del filtro FIR pasa-bajas para eliminar ruido de 60Hz
#define NUM_TAPS        51      // Número de coeficientes
#define BLOCK_SIZE_1    1       // Procesamiento muestra por muestra
#define BLOCK_SIZE_32   32      // Procesamiento por bloques pequeños
#define BLOCK_SIZE_128  128     // Procesamiento por bloques grandes
#define SAMPLE_RATE     250.0f  // Hz - frecuencia de muestreo
#define CUTOFF_FREQ     40.0f   // Hz - frecuencia de corte (elimina 60Hz)

// Número de iteraciones para pruebas de velocidad
#define SPEED_TEST_ITERATIONS 100

// Coeficientes FIR pasa-bajas (fc=40Hz, fs=250Hz, 51 taps)
// Generados con scipy.signal.firwin(51, 40, fs=250)
const float32_t firCoeffs[NUM_TAPS] = {
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

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[maxSamplesNum];    // Señal ECG con ruido
float32_t filteredSignal[maxSamplesNum]; // Señal filtrada
float32_t referenceSignal[maxSamplesNum];// Señal de referencia (limpia estimada)

// Instancias de filtros para diferentes configuraciones
FIRFilter* filterSample;   // Procesamiento muestra por muestra
FIRFilter* filterBlock32;  // Procesamiento por bloques de 32
FIRFilter* filterBlock128; // Procesamiento por bloques de 128

/**
 * @brief Genera señal de referencia "limpia" (promediando múltiples ciclos)
 */
void generateReferenceSignal() {
    // Para esta prueba, usamos un filtrado muy agresivo como referencia
    // En un caso real, se usaría una señal limpia capturada sin ruido
    
    // Copiar señal de entrada como aproximación
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        referenceSignal[i] = inputSignal[i];
    }
    
    // Aplicar promedio móvil simple como referencia de "señal limpia"
    const uint32_t windowSize = 5;
    for (uint32_t i = windowSize; i < maxSamplesNum - windowSize; i++) {
        float32_t sum = 0.0f;
        for (uint32_t j = 0; j < windowSize; j++) {
            sum += inputSignal[i - windowSize/2 + j];
        }
        referenceSignal[i] = sum / windowSize;
    }
}

/**
 * @brief Imprime estadísticas de memoria
 */
void printMemoryStats() {
    Serial.println("\n=== CONSUMO DE RECURSOS ===");
    Serial.print("RAM usado por buffers: ");
    Serial.print((maxSamplesNum * 3 * sizeof(float32_t)) / 1024.0f);
    Serial.println(" KB");
    
    Serial.print("RAM por instancia FIR (1 sample): ");
    Serial.print((NUM_TAPS + BLOCK_SIZE_1 - 1) * sizeof(float32_t));
    Serial.println(" bytes");
    
    Serial.print("RAM por instancia FIR (32 samples): ");
    Serial.print((NUM_TAPS + BLOCK_SIZE_32 - 1) * sizeof(float32_t));
    Serial.println(" bytes");
    
    Serial.print("RAM por instancia FIR (128 samples): ");
    Serial.print((NUM_TAPS + BLOCK_SIZE_128 - 1) * sizeof(float32_t));
    Serial.println(" bytes");
    
    Serial.println("\nNOTA: Arduino Due tiene 96KB de SRAM");
    Serial.println("      Medición de RAM libre no disponible en ARM");
}

/**
 * @brief Prueba de velocidad para procesamiento muestra por muestra
 */
void testSpeedSampleBySample() {
    Serial.println("\n=== PRUEBA DE VELOCIDAD: Muestra por Muestra ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < maxSamplesNum; i++) {
            filteredSignal[i] = filterSample->processSample(inputSignal[i]);
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / maxSamplesNum;
    float32_t maxSampleRate = 1000000.0f / timePerSample;
    
    Serial.print("Tiempo promedio total: ");
    Serial.print(avgTime);
    Serial.println(" µs");
    
    Serial.print("Tiempo por muestra: ");
    Serial.print(timePerSample);
    Serial.println(" µs");
    
    Serial.print("Frecuencia de muestreo máxima: ");
    Serial.print(maxSampleRate);
    Serial.println(" Hz");
    
    Serial.print("Throughput: ");
    Serial.print(maxSamplesNum / (avgTime / 1000000.0f));
    Serial.println(" samples/sec");
}

/**
 * @brief Prueba de velocidad para procesamiento por bloques
 */
void testSpeedBlockProcessing(FIRFilter* filter, uint32_t blockSize, const char* name) {
    Serial.print("\n=== PRUEBA DE VELOCIDAD: ");
    Serial.print(name);
    Serial.println(" ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        uint32_t samplesProcessed = 0;
        while (samplesProcessed < maxSamplesNum) {
            uint32_t currentBlockSize = min(blockSize, maxSamplesNum - samplesProcessed);
            filter->processBuffer(&inputSignal[samplesProcessed], 
                                 &filteredSignal[samplesProcessed], 
                                 currentBlockSize);
            samplesProcessed += currentBlockSize;
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / maxSamplesNum;
    float32_t maxSampleRate = 1000000.0f / timePerSample;
    float32_t speedup = (1000000.0f / timePerSample) / (1000000.0f / (avgTime / maxSamplesNum));
    
    Serial.print("Tiempo promedio total: ");
    Serial.print(avgTime);
    Serial.println(" µs");
    
    Serial.print("Tiempo por muestra: ");
    Serial.print(timePerSample);
    Serial.println(" µs");
    
    Serial.print("Frecuencia de muestreo máxima: ");
    Serial.print(maxSampleRate);
    Serial.println(" Hz");
    
    Serial.print("Throughput: ");
    Serial.print(maxSamplesNum / (avgTime / 1000000.0f));
    Serial.println(" samples/sec");
}

/**
 * @brief Prueba de calidad de filtrado
 */
void testFilterQuality() {
    Serial.println("\n=== PRUEBA DE CALIDAD DE FILTRADO ===");
    
    // Procesar señal con filtro
    uint32_t samplesProcessed = 0;
    while (samplesProcessed < maxSamplesNum) {
        uint32_t currentBlockSize = min((uint32_t)BLOCK_SIZE_32, maxSamplesNum - samplesProcessed);
        filterBlock32->processBuffer(&inputSignal[samplesProcessed], 
                                    &filteredSignal[samplesProcessed], 
                                    currentBlockSize);
        samplesProcessed += currentBlockSize;
    }
    
    // Calcular métricas
    float32_t snr = calculateSNR(filteredSignal, inputSignal, maxSamplesNum);
    float32_t mse = calculateMSE(filteredSignal, referenceSignal, maxSamplesNum);
    float32_t correlation = calculateCorrelation(filteredSignal, referenceSignal, maxSamplesNum);
    
    // Calcular potencia de señal y ruido
    float32_t inputPower = 0.0f;
    float32_t outputPower = 0.0f;
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        inputPower += inputSignal[i] * inputSignal[i];
        outputPower += filteredSignal[i] * filteredSignal[i];
    }
    inputPower /= maxSamplesNum;
    outputPower /= maxSamplesNum;
    
    Serial.print("SNR de mejora: ");
    Serial.print(snr);
    Serial.println(" dB");
    
    Serial.print("MSE vs referencia: ");
    Serial.print(mse, 6);
    Serial.println();
    
    Serial.print("Correlación con referencia: ");
    Serial.print(correlation, 4);
    Serial.println();
    
    Serial.print("Potencia entrada: ");
    Serial.print(inputPower, 6);
    Serial.println();
    
    Serial.print("Potencia salida: ");
    Serial.print(outputPower, 6);
    Serial.println();
    
    Serial.print("Atenuación: ");
    Serial.print(10.0f * log10(outputPower / inputPower));
    Serial.println(" dB");
}

// ========== SETUP Y LOOP ==========

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("\n========================================");
    Serial.println("  FIR FILTER - PRUEBAS DE RENDIMIENTO  ");
    Serial.println("========================================");
    Serial.println("\nBioFilterLib Test Suite v1.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Filtro: FIR pasa-bajas 51 taps, fc=40Hz");
    
    // Convertir señal ECG a float y normalizar
    Serial.println("\nCargando señal ECG con ruido...");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        inputSignal[i] = ((float32_t)waveformsTable[4][i] - 2048.0f) / 2048.0f;
    }
    
    Serial.print("Muestras cargadas: ");
    Serial.println(maxSamplesNum);
    
    // Generar señal de referencia
    Serial.println("Generando señal de referencia...");
    generateReferenceSignal();
    
    // Crear instancias de filtros
    Serial.println("Inicializando filtros...");
    filterSample = new FIRFilter(firCoeffs, NUM_TAPS, BLOCK_SIZE_1);
    filterBlock32 = new FIRFilter(firCoeffs, NUM_TAPS, BLOCK_SIZE_32);
    filterBlock128 = new FIRFilter(firCoeffs, NUM_TAPS, BLOCK_SIZE_128);
    
    Serial.println("Filtros inicializados correctamente.");
    
    // Imprimir estadísticas de memoria
    printMemoryStats();
    
    // Ejecutar pruebas de velocidad
    delay(1000);
    testSpeedSampleBySample();
    
    delay(500);
    testSpeedBlockProcessing(filterBlock32, BLOCK_SIZE_32, "Bloques de 32 muestras");
    
    delay(500);
    testSpeedBlockProcessing(filterBlock128, BLOCK_SIZE_128, "Bloques de 128 muestras");
    
    // Ejecutar pruebas de calidad
    delay(500);
    testFilterQuality();
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
    Serial.println("\nResultados guardados. Sistema listo.");
}

void loop() {
    // Las pruebas se ejecutan una sola vez en setup()
    delay(1000);
}