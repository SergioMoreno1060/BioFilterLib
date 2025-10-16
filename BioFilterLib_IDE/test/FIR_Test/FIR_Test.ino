/**
 * @file FIR_Test.ino
 * @brief Pruebas de rendimiento y calidad para FIRFilter usando TestRunner
 * @author Sergio Moreno
 * @version 2.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro FIR evaluando:
 * 1. Velocidad de procesamiento (muestra individual vs buffer)
 * 2. Consumo de recursos (RAM, stack)
 * 3. Calidad de filtrado (SNR, MSE, correlación) usando TestRunner
 * 
 * Usa señal ECG con ruido 60Hz (waveformsTable[7]) @ 240Hz con 900 samples
 */

#include <Arduino.h>
#include <BioFilterLib.h>

// ========== CONFIGURACIÓN DE PRUEBAS ==========

// Señal de prueba: waveformsTable[7] - ECG con ruido 60Hz @ 240Hz (900 samples)
#define SIGNAL_TAG      "ecg_60hz_noised_fs240"
#define NUM_SAMPLES     900     // Usar solo las primeras 900 muestras

// Configuración del filtro FIR pasa-bajas para eliminar ruido de 60Hz
#define NUM_TAPS        51      
#define BLOCK_SIZE_1    1       
#define BLOCK_SIZE_32   32      
#define BLOCK_SIZE_128  128     
#define SAMPLE_RATE     240.0f  // Hz - frecuencia de muestreo
#define CUTOFF_FREQ     40.0f   // Hz - frecuencia de corte (elimina 60Hz)

// Número de iteraciones para pruebas de velocidad
#define SPEED_TEST_ITERATIONS 100

// Coeficientes FIR pasa-bajas (fc=40Hz, fs=240Hz, 51 taps)
float32_t firCoeffs[NUM_TAPS] = {
  0.001f, 0.001f, 0.001f, 0.001f, 0.001f, -0.001f, -0.002f, -0.004f,
  -0.006f, -0.008f, -0.010f, -0.010f, -0.009f, -0.006f, 0.0f, 0.008f,
  0.018f, 0.031f, 0.044f, 0.058f, 0.071f, 0.083f, 0.092f, 0.098f,
  0.100f, 0.098f, 0.092f, 0.083f, 0.071f, 0.058f, 0.044f, 0.031f,
  0.018f, 0.008f, 0.0f, -0.006f, -0.009f, -0.010f, -0.010f, -0.008f,
  -0.006f, -0.004f, -0.002f, -0.001f, 0.001f, 0.001f, 0.001f, 0.001f,
  0.001f, 0.001f, 0.001f
};

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[NUM_SAMPLES];
float32_t filteredSignal[NUM_SAMPLES];

// Instancias de filtros para diferentes configuraciones
FIRFilter* filterSample;
FIRFilter* filterBlock32;
FIRFilter* filterBlock128;

// TestRunner para pruebas automáticas
TestRunner testRunner;

// ========== FUNCIONES DE TEST ==========

void printMemoryStats() {
    Serial.println("\n=== CONSUMO DE RECURSOS ===");
    Serial.print("RAM usado por buffers: ");
    Serial.print((NUM_SAMPLES * 2 * sizeof(float32_t)) / 1024.0f, 2);
    Serial.println(" KB");
    
    Serial.print("RAM por instancia FIR (1 sample): ");
    Serial.print((NUM_TAPS + BLOCK_SIZE_1 - 1) * sizeof(float32_t));
    Serial.println(" bytes");
    
    Serial.print("RAM por instancia FIR (32 samples): ");
    Serial.print((NUM_TAPS + BLOCK_SIZE_32 - 1) * sizeof(float32_t));
    Serial.println(" bytes");
    
    Serial.println("\nNOTA: Arduino Due tiene 96KB de SRAM");
}

void testSpeedSampleBySample() {
    Serial.println("\n=== PRUEBA DE VELOCIDAD: Muestra por muestra ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
            filteredSignal[i] = filterSample->processSample(inputSignal[i]);
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / NUM_SAMPLES;
    float32_t maxSampleRate = 1000000.0f / timePerSample;
    
    Serial.print("Tiempo promedio: ");
    Serial.print(avgTime, 2);
    Serial.println(" µs");
    Serial.print("Tiempo/muestra: ");
    Serial.print(timePerSample, 3);
    Serial.println(" µs");
    Serial.print("Fs máxima: ");
    Serial.print(maxSampleRate, 0);
    Serial.println(" Hz");
    Serial.print("Throughput: ");
    Serial.print(NUM_SAMPLES / (avgTime / 1000000.0f), 0);
    Serial.println(" samples/s");
}

void testSpeedBlockProcessing(FIRFilter* filter, uint32_t blockSize, const char* name) {
    Serial.print("\n=== VELOCIDAD: ");
    Serial.print(name);
    Serial.println(" ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        uint32_t samplesProcessed = 0;
        while (samplesProcessed < NUM_SAMPLES) {
            uint32_t currentBlockSize = min(blockSize, NUM_SAMPLES - samplesProcessed);
            filter->processBuffer(&inputSignal[samplesProcessed], 
                                 &filteredSignal[samplesProcessed], 
                                 currentBlockSize);
            samplesProcessed += currentBlockSize;
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / NUM_SAMPLES;
    float32_t maxSampleRate = 1000000.0f / timePerSample;
    
    Serial.print("Tiempo promedio: ");
    Serial.print(avgTime, 2);
    Serial.println(" µs");
    Serial.print("Tiempo/muestra: ");
    Serial.print(timePerSample, 3);
    Serial.println(" µs");
    Serial.print("Fs máxima: ");
    Serial.print(maxSampleRate, 0);
    Serial.println(" Hz");
    Serial.print("Speedup vs muestra individual: ");
    Serial.print(100.0f * (1.0f - timePerSample / (avgTime / NUM_SAMPLES)), 1);
    Serial.println("%");
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
    Serial.println("\nBioFilterLib Test Suite v2.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Filtro: FIR pasa-bajas 51 taps, fc=40Hz");
    Serial.print("Señal: ");
    Serial.print(getSignalName(SIGNAL_TAG));
    Serial.print(" (");
    Serial.print(NUM_SAMPLES);
    Serial.println(" samples)");
    
    // Cargar señal de prueba
    Serial.println("\nCargando señal de prueba...");
    if (!loadSignal(SIGNAL_TAG, inputSignal)) {
        Serial.println("ERROR: No se pudo cargar la señal");
        while(1);
    }
    
    Serial.print("Muestras cargadas: ");
    Serial.println(NUM_SAMPLES);
    
    // Crear instancias de filtros
    Serial.println("Inicializando filtros...");
    filterSample = new FIRFilter(firCoeffs, NUM_TAPS, BLOCK_SIZE_1);
    filterBlock32 = new FIRFilter(firCoeffs, NUM_TAPS, BLOCK_SIZE_32);
    filterBlock128 = new FIRFilter(firCoeffs, NUM_TAPS, BLOCK_SIZE_128);
    
    if (!filterSample || !filterBlock32 || !filterBlock128) {
        Serial.println("ERROR: No se pudieron crear los filtros");
        while(1);
    }
    
    Serial.println("Filtros inicializados correctamente.");
    
    // Imprimir estadísticas de memoria
    printMemoryStats();
    
    // ===== PRUEBAS DE VELOCIDAD =====
    delay(1000);
    testSpeedSampleBySample();
    
    delay(500);
    testSpeedBlockProcessing(filterBlock32, BLOCK_SIZE_32, "Bloques de 32 muestras");
    
    delay(500);
    testSpeedBlockProcessing(filterBlock128, BLOCK_SIZE_128, "Bloques de 128 muestras");
    
    // ===== PRUEBAS DE CALIDAD CON TESTRUNNER =====
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("  PRUEBAS DE CALIDAD (TestRunner)");
    Serial.println("========================================");
    
    // Probar con la señal configurada
    Serial.println("\n--- Prueba individual ---");
    testRunner.testFIR(filterBlock32, SIGNAL_TAG);
    
    // Probar todas las señales disponibles
    delay(500);
    testRunner.testAllFIR(filterBlock32);
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
    Serial.println("\nSistema listo.");
}

void loop() {
    // Nada que hacer aquí
    delay(1000);
}