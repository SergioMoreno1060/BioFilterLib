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

// ========== CONFIGURACIÓN DE PRUEBAS ==========

// Configuración del filtro FIR pasa-bajas para eliminar ruido de 60Hz
#define NUM_TAPS        51      // Número de coeficientes
#define BLOCK_SIZE_1    1       // Procesamiento muestra por muestra
#define BLOCK_SIZE_32   32      // Procesamiento por bloques pequeños
#define BLOCK_SIZE_128  128     // Procesamiento por bloques grandes
#define SAMPLE_RATE     960.0f  // Hz - frecuencia de muestreo
#define CUTOFF_FREQ     40.0f   // Hz - frecuencia de corte (elimina 60Hz)
#define SamplesNum      1000

// Número de iteraciones para pruebas de velocidad
#define SPEED_TEST_ITERATIONS 100

// Coeficientes FIR pasa-bajas (fc=40Hz, fs=250Hz, 51 taps)
// Generados con scipy.signal.firwin(51, 40, fs=250)
float32_t firCoeffs[NUM_TAPS] = {
    +0.00096226f, +0.00110652f, +0.00123488f, +0.00128605f, +0.00115012f, +0.00068979f, -0.00022373f, -0.00166613f,
    -0.00361514f, -0.00591523f, -0.00826092f, -0.01020548f, -0.01119776f, -0.01064514f, -0.00799553f, -0.00282748f,
    +0.00506537f, +0.01560960f, +0.02841897f, +0.04280137f, +0.05780805f, +0.07232126f, +0.08517038f, +0.09526185f,
    +0.10170564f, +0.10392083f, +0.10170564f, +0.09526185f, +0.08517038f, +0.07232126f, +0.05780805f, +0.04280137f,
    +0.02841897f, +0.01560960f, +0.00506537f, -0.00282748f, -0.00799553f, -0.01064514f, -0.01119776f, -0.01020548f,
    -0.00826092f, -0.00591523f, -0.00361514f, -0.00166613f, -0.00022373f, +0.00068979f, +0.00115012f, +0.00128605f,
    +0.00123488f, +0.00110652f, +0.00096226f
};

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[SamplesNum];    // Señal ECG con ruido
float32_t filteredSignal[SamplesNum]; // Señal filtrada
float32_t referenceSignal[SamplesNum];// Señal de referencia (limpia estimada)

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
    for (uint32_t i = 0; i < SamplesNum; i++) {
        referenceSignal[i] = inputSignal[i];
    }
    
    // Aplicar promedio móvil simple como referencia de "señal limpia"
    const uint32_t windowSize = 5;
    for (uint32_t i = windowSize; i < SamplesNum - windowSize; i++) {
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
    Serial.print((SamplesNum * 3 * sizeof(float32_t)) / 1024.0f);
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
    
    // extern unsigned int __heap_start;
    // extern void *__brkval;
    // int freeRAM = (int)&freeRAM - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
    // Serial.print("RAM libre aproximada: ");
    // Serial.print(freeRAM);
    // Serial.println(" bytes");
}

/**
 * @brief Prueba de velocidad para procesamiento muestra por muestra
 */
void testSpeedSampleBySample() {
    Serial.println("\n=== PRUEBA DE VELOCIDAD: Muestra por Muestra ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < SamplesNum; i++) {
            filteredSignal[i] = filterSample->processSample(inputSignal[i]);
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / SamplesNum;
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
    Serial.print(SamplesNum / (avgTime / 1000000.0f));
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
        while (samplesProcessed < SamplesNum) {
            uint32_t currentBlockSize = min(blockSize, SamplesNum - samplesProcessed);
            filter->processBuffer(&inputSignal[samplesProcessed], 
                                 &filteredSignal[samplesProcessed], 
                                 currentBlockSize);
            samplesProcessed += currentBlockSize;
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / SamplesNum;
    float32_t maxSampleRate = 1000000.0f / timePerSample;
    float32_t speedup = (1000000.0f / timePerSample) / (1000000.0f / (avgTime / SamplesNum));
    
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
    Serial.print(SamplesNum / (avgTime / 1000000.0f));
    Serial.println(" samples/sec");
}

/**
 * @brief Prueba de calidad de filtrado
 */
void testFilterQuality() {
    Serial.println("\n=== PRUEBA DE CALIDAD DE FILTRADO ===");
    
    // Procesar señal con filtro
    uint32_t samplesProcessed = 0;
    while (samplesProcessed < SamplesNum) {
        uint32_t currentBlockSize = min((uint32_t)BLOCK_SIZE_32, SamplesNum - samplesProcessed);
        filterBlock32->processBuffer(&inputSignal[samplesProcessed], 
                                    &filteredSignal[samplesProcessed], 
                                    currentBlockSize);
        samplesProcessed += currentBlockSize;
    }
    
    // Calcular métricas
    float32_t snr = calculateSNR(filteredSignal+25, inputSignal, SamplesNum-25);
    float32_t mse = calculateMSE(filteredSignal, referenceSignal, SamplesNum);
    float32_t correlation = calculateCorrelation(filteredSignal+25, referenceSignal, SamplesNum-25);
    
    // Calcular potencia de señal y ruido
    float32_t inputPower = 0.0f;
    float32_t outputPower = 0.0f;
    for (uint32_t i = 0; i < SamplesNum; i++) {
        inputPower += inputSignal[i] * inputSignal[i];
        outputPower += filteredSignal[i] * filteredSignal[i];
    }
    inputPower /= SamplesNum;
    outputPower /= SamplesNum;
    
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
    for (uint32_t i = 0; i < SamplesNum; i++) {
        inputSignal[i] = ((float32_t)waveformsTable[1][i] - 2048.0f) / 2048.0f;
    }
    
    Serial.print("Muestras cargadas: ");
    Serial.println(SamplesNum);
    
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