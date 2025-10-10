/**
 * @file LMS_Test.cpp
 * @brief Pruebas de rendimiento y calidad para LMSFilter
 * @author Sergio Moreno
 * @version 1.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro LMS adaptativo evaluando:
 * 1. Velocidad de procesamiento y convergencia
 * 2. Consumo de recursos (RAM, procesamiento adaptativo)
 * 3. Calidad de adaptación y cancelación de interferencia
 * 4. Rendimiento con diferentes valores de mu (velocidad de adaptación)
 * 
 * El filtro LMS es probado para cancelar interferencia de 60Hz
 * en señal ECG real con ruido del archivo Waveforms.h
 */

#include <Arduino.h>
#include <BioFilterLib.h>
#include "Waveforms.h"

// ========== CONFIGURACIÓN DE PRUEBAS ==========

#define NUM_TAPS_LMS    32      // Número de coeficientes adaptativos
#define MU_FAST         0.05f   // Adaptación rápida (menos estable)
#define MU_MEDIUM       0.02f   // Adaptación balanceada
#define MU_SLOW         0.005f  // Adaptación lenta (más estable)
#define BLOCK_SIZE      1       // Procesamiento muestra por muestra
#define SAMPLE_RATE     250.0f  // Hz
#define POWERLINE_FREQ  60.0f   // Hz - Frecuencia a cancelar
#define SPEED_TEST_ITERATIONS 50

// ========== CONFIGURACIÓN DE SEÑAL ==========

// Usar señal de waveformsTable[6]: 3.6sec ECG con ruido 60Hz @ 250Hz (900 samples)
#define SIGNAL_INDEX    6       // Índice en waveformsTable
#define NUM_SAMPLES     900     // 900 muestras @ 250Hz = 3.6 segundos

// ========== BUFFERS DE DATOS ==========

// Buffers de entrada (necesarios para las 3 pruebas)
float32_t inputSignal[NUM_SAMPLES];           // ECG con ruido
float32_t referenceSignal[NUM_SAMPLES];       // Señal de referencia (60Hz)

// Buffers de salida (reutilizados entre pruebas para ahorrar RAM)
float32_t filteredSignal[NUM_SAMPLES];        // Salida del filtro (reutilizado)
float32_t errorSignal[NUM_SAMPLES];           // Error de adaptación (reutilizado)

// Coeficientes LMS (se adaptan durante el proceso)
float32_t lmsCoeffsFast[NUM_TAPS_LMS];
float32_t lmsCoeffsMedium[NUM_TAPS_LMS];
float32_t lmsCoeffsSlow[NUM_TAPS_LMS];

// Instancias de filtros LMS
LMSFilter* lmsFilterFast;
LMSFilter* lmsFilterMedium;
LMSFilter* lmsFilterSlow;

// ========== FUNCIONES AUXILIARES ==========

/**
 * @brief Genera señal de referencia sinusoidal (60Hz)
 */
void generateReferenceSignal() {
    Serial.println("Generando señal de referencia (60Hz)...");
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        float32_t t = (float32_t)i / SAMPLE_RATE;
        referenceSignal[i] = sin(2.0f * PI * POWERLINE_FREQ * t);
    }
}

/**
 * @brief Inicializa coeficientes LMS a cero
 */
void initializeCoefficients(float32_t* coeffs, uint32_t numTaps) {
    for (uint32_t i = 0; i < numTaps; i++) {
        coeffs[i] = 0.0f;
    }
}

/**
 * @brief Calcula RMS de la señal
 */
float32_t calculateRMS(const float32_t* signal, uint32_t length) {
    float32_t sum = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        sum += signal[i] * signal[i];
    }
    return sqrt(sum / length);
}

/**
 * @brief Calcula potencia de la señal
 */
float32_t calculatePower(const float32_t* signal, uint32_t length) {
    float32_t power = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        power += signal[i] * signal[i];
    }
    return power / length;
}

/**
 * @brief Calcula tiempo de convergencia (cuando error < threshold)
 */
uint32_t calculateConvergenceTime(const float32_t* errorSignal, uint32_t length, float32_t threshold) {
    // Calcular error RMS en ventanas móviles
    const uint32_t windowSize = 50;
    
    for (uint32_t i = windowSize; i < length; i++) {
        float32_t windowError = 0.0f;
        for (uint32_t j = 0; j < windowSize; j++) {
            float32_t err = errorSignal[i - j];
            windowError += err * err;
        }
        windowError = sqrt(windowError / windowSize);
        
        if (windowError < threshold) {
            return i;
        }
    }
    return length; // No convergió
}

/**
 * @brief Calcula la mejora en SNR (dB)
 */
float32_t calculateSNRImprovement(const float32_t* input, const float32_t* output, uint32_t length) {
    float32_t inputPower = calculatePower(input, length);
    float32_t outputPower = calculatePower(output, length);
    
    if (outputPower < 1e-10f) return 999.9f;
    return 10.0f * log10(inputPower / outputPower);
}

/**
 * @brief Analiza estabilidad de coeficientes
 */
void analyzeCoefficients(const float32_t* coeffs, uint32_t numTaps, const char* name) {
    Serial.print("\nAnálisis de coeficientes - ");
    Serial.println(name);
    
    // Encontrar coeficiente máximo
    float32_t maxCoeff = 0.0f;
    uint32_t maxIdx = 0;
    for (uint32_t i = 0; i < numTaps; i++) {
        if (abs(coeffs[i]) > abs(maxCoeff)) {
            maxCoeff = coeffs[i];
            maxIdx = i;
        }
    }
    
    // Calcular norma L2
    float32_t norm = 0.0f;
    for (uint32_t i = 0; i < numTaps; i++) {
        norm += coeffs[i] * coeffs[i];
    }
    norm = sqrt(norm);
    
    Serial.print("  Coef. máximo: ");
    Serial.print(maxCoeff, 6);
    Serial.print(" (índice ");
    Serial.print(maxIdx);
    Serial.println(")");
    Serial.print("  Norma L2: ");
    Serial.println(norm, 6);
    
    // Verificar estabilidad (coeficientes no deben explotar)
    bool stable = (abs(maxCoeff) < 10.0f && norm < 50.0f);
    Serial.print("  Estabilidad: ");
    Serial.println(stable ? "ESTABLE" : "INESTABLE");
}

void printMemoryStats() {
    Serial.println("\n=== CONSUMO DE RECURSOS LMS ===");
    
    uint32_t bufferRAM = (NUM_SAMPLES * 4 * sizeof(float32_t)); // 4 buffers principales
    Serial.print("RAM usado por buffers: ");
    Serial.print(bufferRAM / 1024.0f);
    Serial.println(" KB");
    
    Serial.print("  - inputSignal: ");
    Serial.print((NUM_SAMPLES * sizeof(float32_t)) / 1024.0f);
    Serial.println(" KB");
    Serial.print("  - referenceSignal: ");
    Serial.print((NUM_SAMPLES * sizeof(float32_t)) / 1024.0f);
    Serial.println(" KB");
    Serial.print("  - filteredSignal (reutilizado): ");
    Serial.print((NUM_SAMPLES * sizeof(float32_t)) / 1024.0f);
    Serial.println(" KB");
    Serial.print("  - errorSignal (reutilizado): ");
    Serial.print((NUM_SAMPLES * sizeof(float32_t)) / 1024.0f);
    Serial.println(" KB");
    
    Serial.print("\nRAM por coeficientes LMS (32 taps): ");
    Serial.print(NUM_TAPS_LMS * sizeof(float32_t));
    Serial.println(" bytes");
    
    Serial.print("RAM total por instancia LMS: ");
    Serial.print((NUM_TAPS_LMS * 2) * sizeof(float32_t)); // coefs + state
    Serial.println(" bytes");
    
    Serial.println("\nComparación con otros filtros:");
    Serial.println("  - FIR 51 taps: ~204 bytes");
    Serial.println("  - IIR 2º orden: ~16 bytes");
    Serial.println("  - LMS 32 taps: ~256 bytes (adaptativo)");
    
    Serial.println("\nNOTA: Arduino Due tiene 96KB de SRAM");
    Serial.println("      Medición de RAM libre no disponible en ARM");
    
    float32_t percentUsed = (bufferRAM / (96.0f * 1024.0f)) * 100.0f;
    Serial.print("\nUso estimado de RAM: ");
    Serial.print(percentUsed, 1);
    Serial.println("% del total disponible");
}

// Estructura para guardar resultados de pruebas
struct TestResults {
    float32_t avgTime;
    float32_t timePerSample;
    uint32_t convergenceTime;
    float32_t initialError;
    float32_t finalError;
    float32_t snrImprovement;
    float32_t outputRMS;
};

TestResults resultFast, resultMedium, resultSlow;

void testSpeedAndConvergence(LMSFilter* filter, float32_t mu, 
                             const char* name, TestResults* results) {
    Serial.print("\n=== PRUEBA: ");
    Serial.print(name);
    Serial.print(" (mu=");
    Serial.print(mu, 4);
    Serial.println(") ===");
    
    // Prueba de velocidad
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
            filter->processSample(inputSignal[i], referenceSignal[i], 
                                 &filteredSignal[i], &errorSignal[i]);
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    results->avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    results->timePerSample = results->avgTime / NUM_SAMPLES;
    float32_t maxSampleRate = 1000000.0f / results->timePerSample;
    
    Serial.println("\nVELOCIDAD:");
    Serial.print("  Tiempo total: ");
    Serial.print(results->avgTime);
    Serial.println(" µs");
    Serial.print("  Tiempo/muestra: ");
    Serial.print(results->timePerSample, 2);
    Serial.println(" µs");
    Serial.print("  Fs máxima: ");
    Serial.print(maxSampleRate);
    Serial.println(" Hz");
    Serial.print("  Throughput: ");
    Serial.print(NUM_SAMPLES / (results->avgTime / 1000000.0f));
    Serial.println(" samples/s");
    
    // Análisis de convergencia
    Serial.println("\nCONVERGENCIA:");
    results->convergenceTime = calculateConvergenceTime(errorSignal, NUM_SAMPLES, 0.1f);
    Serial.print("  Tiempo de convergencia: ");
    if (results->convergenceTime < NUM_SAMPLES) {
        Serial.print(results->convergenceTime);
        Serial.print(" muestras (");
        Serial.print((float32_t)results->convergenceTime / SAMPLE_RATE * 1000.0f);
        Serial.println(" ms)");
    } else {
        Serial.println("NO CONVERGIÓ");
    }
    
    // Error RMS inicial vs final
    results->initialError = calculateRMS(errorSignal, min(100u, NUM_SAMPLES));
    results->finalError = calculateRMS(&errorSignal[max(0, (int)NUM_SAMPLES - 100)], 
                                       min(100u, NUM_SAMPLES));
    
    Serial.print("  Error RMS inicial: ");
    Serial.println(results->initialError, 6);
    Serial.print("  Error RMS final: ");
    Serial.println(results->finalError, 6);
    Serial.print("  Reducción error: ");
    Serial.print(100.0f * (1.0f - results->finalError / results->initialError));
    Serial.println(" %");
    
    // Calidad de filtrado
    Serial.println("\nCALIDAD:");
    float32_t inputRMS = calculateRMS(inputSignal, NUM_SAMPLES);
    results->outputRMS = calculateRMS(filteredSignal, NUM_SAMPLES);
    results->snrImprovement = calculateSNRImprovement(inputSignal, 
                                                      &filteredSignal[results->convergenceTime], 
                                                      NUM_SAMPLES - results->convergenceTime);
    
    Serial.print("  RMS entrada: ");
    Serial.println(inputRMS, 6);
    Serial.print("  RMS salida: ");
    Serial.println(results->outputRMS, 6);
    Serial.print("  Atenuación interferencia: ");
    Serial.print(10.0f * log10(results->outputRMS / inputRMS));
    Serial.println(" dB");
    Serial.print("  Mejora SNR (post-convergencia): ");
    Serial.print(results->snrImprovement);
    Serial.println(" dB");
}

void compareAdaptationRates() {
    Serial.println("\n=== COMPARACIÓN DE TASAS DE ADAPTACIÓN ===");
    
    Serial.println("\nRESUMEN:");
    Serial.println("----------------------------------------");
    Serial.println("Parámetro          | Rápido | Medio | Lento");
    Serial.println("----------------------------------------");
    
    // Tiempo de convergencia
    Serial.print("Convergencia (ms)  | ");
    Serial.print(resultFast.convergenceTime / SAMPLE_RATE * 1000.0f, 0);
    Serial.print("  | ");
    Serial.print(resultMedium.convergenceTime / SAMPLE_RATE * 1000.0f, 0);
    Serial.print(" | ");
    Serial.println(resultSlow.convergenceTime / SAMPLE_RATE * 1000.0f, 0);
    
    // Error final
    Serial.print("Error RMS final    | ");
    Serial.print(resultFast.finalError, 4);
    Serial.print(" | ");
    Serial.print(resultMedium.finalError, 4);
    Serial.print(" | ");
    Serial.println(resultSlow.finalError, 4);
    
    // SNR improvement
    Serial.print("Mejora SNR (dB)    | ");
    Serial.print(resultFast.snrImprovement, 2);
    Serial.print("  | ");
    Serial.print(resultMedium.snrImprovement, 2);
    Serial.print("  | ");
    Serial.println(resultSlow.snrImprovement, 2);
    
    Serial.println("----------------------------------------");
    Serial.println("\nRECOMENDACIÓN:");
    if (resultMedium.finalError < resultFast.finalError && 
        resultMedium.finalError < resultSlow.finalError) {
        Serial.println("  Mu óptimo: MEDIO (0.02)");
        Serial.println("  Razón: Mejor balance velocidad/estabilidad");
    } else if (resultFast.finalError < resultSlow.finalError) {
        Serial.println("  Mu óptimo: RÁPIDO (0.05)");
        Serial.println("  Razón: Convergencia más rápida sin inestabilidad");
    } else {
        Serial.println("  Mu óptimo: LENTO (0.005)");
        Serial.println("  Razón: Mayor estabilidad y menor error final");
    }
}

// ========== SETUP Y LOOP ==========

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("\n========================================");
    Serial.println("  LMS FILTER - PRUEBAS DE RENDIMIENTO  ");
    Serial.println("========================================");
    Serial.println("\nBioFilterLib Test Suite v1.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Filtro: LMS Adaptativo 32 taps");
    Serial.println("Aplicación: Cancelación de interferencia 60Hz");
    Serial.print("Señal: waveformsTable[");
    Serial.print(SIGNAL_INDEX);
    Serial.print("] - ");
    Serial.print(NUM_SAMPLES);
    Serial.println(" samples @ 250Hz");
    
    // Cargar señal ECG
    Serial.println("\nCargando señal ECG con ruido...");
    for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
        inputSignal[i] = ((float32_t)waveformsTable[SIGNAL_INDEX][i] - 2048.0f) / 2048.0f;
    }
    Serial.print("Muestras cargadas: ");
    Serial.println(NUM_SAMPLES);
    
    // Generar señal de referencia
    generateReferenceSignal();
    
    // Inicializar coeficientes
    Serial.println("Inicializando coeficientes LMS...");
    initializeCoefficients(lmsCoeffsFast, NUM_TAPS_LMS);
    initializeCoefficients(lmsCoeffsMedium, NUM_TAPS_LMS);
    initializeCoefficients(lmsCoeffsSlow, NUM_TAPS_LMS);
    
    // Crear instancias de filtros
    Serial.println("Creando filtros LMS...");
    lmsFilterFast = new LMSFilter(lmsCoeffsFast, NUM_TAPS_LMS, MU_FAST, BLOCK_SIZE);
    lmsFilterMedium = new LMSFilter(lmsCoeffsMedium, NUM_TAPS_LMS, MU_MEDIUM, BLOCK_SIZE);
    lmsFilterSlow = new LMSFilter(lmsCoeffsSlow, NUM_TAPS_LMS, MU_SLOW, BLOCK_SIZE);
    
    Serial.println("Filtros inicializados.");
    
    // Estadísticas de memoria
    printMemoryStats();
    
    // Pruebas de velocidad y convergencia
    delay(1000);
    testSpeedAndConvergence(lmsFilterFast, MU_FAST, "Adaptación Rápida", &resultFast);
    
    delay(500);
    
    // Reinicializar filtro para prueba independiente
    delete lmsFilterMedium;
    initializeCoefficients(lmsCoeffsMedium, NUM_TAPS_LMS);
    lmsFilterMedium = new LMSFilter(lmsCoeffsMedium, NUM_TAPS_LMS, MU_MEDIUM, BLOCK_SIZE);
    testSpeedAndConvergence(lmsFilterMedium, MU_MEDIUM, "Adaptación Media", &resultMedium);
    
    delay(500);
    
    // Reinicializar filtro para prueba independiente
    delete lmsFilterSlow;
    initializeCoefficients(lmsCoeffsSlow, NUM_TAPS_LMS);
    lmsFilterSlow = new LMSFilter(lmsCoeffsSlow, NUM_TAPS_LMS, MU_SLOW, BLOCK_SIZE);
    testSpeedAndConvergence(lmsFilterSlow, MU_SLOW, "Adaptación Lenta", &resultSlow);
    
    // Análisis de coeficientes finales
    delay(500);
    Serial.println("\n=== ANÁLISIS DE COEFICIENTES FINALES ===");
    analyzeCoefficients(lmsCoeffsFast, NUM_TAPS_LMS, "Rápido");
    analyzeCoefficients(lmsCoeffsMedium, NUM_TAPS_LMS, "Medio");
    analyzeCoefficients(lmsCoeffsSlow, NUM_TAPS_LMS, "Lento");
    
    // Comparación final
    delay(500);
    compareAdaptationRates();
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
}

void loop() {
    delay(1000);
}