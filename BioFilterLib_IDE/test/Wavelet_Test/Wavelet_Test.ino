/**
 * @file Wavelet_Test.cpp
 * @brief Pruebas de rendimiento y calidad para WaveletFilter
 * @author Sergio Moreno
 * @version 1.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro Wavelet evaluando:
 * 1. Velocidad de procesamiento (descomposición y reconstrucción)
 * 2. Consumo de recursos (RAM, procesamiento multi-resolución)
 * 3. Calidad de descomposición (separación frecuencias)
 * 4. Capacidad de denoising y detección de eventos
 * 
 * Utiliza transformada wavelet Daubechies-4 en banco de filtros
 * sobre señal ECG real con ruido del archivo Waveforms.h
 */

#include <Arduino.h>
#include <BioFilterLib.h>
#include "Waveforms.h"

// ========== CONFIGURACIÓN DE PRUEBAS ==========

#define BLOCK_SIZE_1    1
#define BLOCK_SIZE_32   32
#define BLOCK_SIZE_128  128
#define SAMPLE_RATE     250.0f
#define SPEED_TEST_ITERATIONS 50

// Umbrales para denoising
#define SOFT_THRESHOLD      0.15f  // Umbral suave
#define HARD_THRESHOLD      0.25f  // Umbral duro

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[maxSamplesNum];           // ECG con ruido
float32_t approxCoeffs[maxSamplesNum];          // Coeficientes de aproximación
float32_t detailCoeffs[maxSamplesNum];          // Coeficientes de detalle
float32_t reconstructedSignal[maxSamplesNum];   // Señal reconstruida
float32_t denoisedSignal[maxSamplesNum];        // Señal con denoising
float32_t detectedEvents[maxSamplesNum];        // Eventos detectados (QRS)

// Instancias de filtros wavelet
WaveletFilter* waveletFilter1;
WaveletFilter* waveletFilter32;
WaveletFilter* waveletFilter128;

// ========== FUNCIONES AUXILIARES ==========

/**
 * @brief Aplica umbral suave (soft thresholding)
 */
float32_t softThreshold(float32_t value, float32_t threshold) {
    if (abs(value) < threshold) {
        return 0.0f;
    } else if (value > 0) {
        return value - threshold;
    } else {
        return value + threshold;
    }
}

/**
 * @brief Aplica umbral duro (hard thresholding)
 */
float32_t hardThreshold(float32_t value, float32_t threshold) {
    return (abs(value) < threshold) ? 0.0f : value;
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
 * @brief Calcula energía de la señal
 */
float32_t calculateEnergy(const float32_t* signal, uint32_t length) {
    float32_t energy = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        energy += signal[i] * signal[i];
    }
    return energy;
}

/**
 * @brief Calcula error de reconstrucción
 */
float32_t calculateReconstructionError(const float32_t* original, 
                                       const float32_t* reconstructed, 
                                       uint32_t length) {
    float32_t mse = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        float32_t error = original[i] - reconstructed[i];
        mse += error * error;
    }
    return sqrt(mse / length);
}

/**
 * @brief Detecta eventos (picos QRS) usando coeficientes de detalle
 */
uint32_t detectEvents(const float32_t* detailCoeffs, float32_t* events, 
                      uint32_t length, float32_t threshold) {
    uint32_t eventCount = 0;
    bool inEvent = false;
    
    for (uint32_t i = 0; i < length; i++) {
        events[i] = 0.0f;
        
        if (abs(detailCoeffs[i]) > threshold) {
            if (!inEvent) {
                events[i] = 1.0f;
                eventCount++;
                inEvent = true;
            }
        } else {
            inEvent = false;
        }
    }
    
    return eventCount;
}

/**
 * @brief Calcula SNR de la señal
 */
float32_t calculateSNR(const float32_t* signal, const float32_t* noise, uint32_t length) {
    float32_t signalPower = 0.0f;
    float32_t noisePower = 0.0f;
    
    for (uint32_t i = 0; i < length; i++) {
        signalPower += signal[i] * signal[i];
        float32_t diff = signal[i] - noise[i];
        noisePower += diff * diff;
    }
    
    signalPower /= length;
    noisePower /= length;
    
    if (noisePower < 1e-10f) return 999.9f;
    return 10.0f * log10(signalPower / noisePower);
}

void printMemoryStats() {
    Serial.println("\n=== CONSUMO DE RECURSOS WAVELET ===");
    Serial.print("RAM usado por buffers: ");
    Serial.print((maxSamplesNum * 5 * sizeof(float32_t)) / 1024.0f);
    Serial.println(" KB");
    
    // Wavelet usa 4 filtros FIR (approx, detail, synth_approx, synth_detail)
    // Cada filtro Daubechies-4 tiene 8 coeficientes
    Serial.println("\nMemoria por instancia Wavelet:");
    Serial.println("  - 4 filtros FIR (8 taps c/u)");
    Serial.println("  - Aprox: 32 bytes coefs + estado");
    Serial.println("  - Total: ~256 bytes por filtro");
    
    Serial.println("\nComparación con otros filtros:");
    Serial.println("  - FIR 51 taps: ~204 bytes");
    Serial.println("  - IIR 2º orden: ~16 bytes");
    Serial.println("  - LMS 32 taps: ~256 bytes");
    Serial.println("  - Wavelet DB4: ~1024 bytes (4 filtros)");
    
    Serial.println("\nNOTA: Arduino Due tiene 96KB de SRAM");
    Serial.println("      Medición de RAM libre no disponible en ARM");
}

void testSpeedDecomposition(WaveletFilter* filter, uint32_t blockSize, const char* name) {
    Serial.print("\n=== VELOCIDAD DESCOMPOSICIÓN: ");
    Serial.print(name);
    Serial.print(" (block=");
    Serial.print(blockSize);
    Serial.println(") ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        if (blockSize == 1) {
            // Procesamiento muestra por muestra
            for (uint32_t i = 0; i < maxSamplesNum; i++) {
                filter->processSample(inputSignal[i], &approxCoeffs[i], &detailCoeffs[i]);
            }
        } else {
            // Procesamiento por bloques
            uint32_t samplesProcessed = 0;
            while (samplesProcessed < maxSamplesNum) {
                uint32_t currentBlockSize = min(blockSize, maxSamplesNum - samplesProcessed);
                filter->processBuffer(&inputSignal[samplesProcessed],
                                     &approxCoeffs[samplesProcessed],
                                     &detailCoeffs[samplesProcessed],
                                     currentBlockSize);
                samplesProcessed += currentBlockSize;
            }
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / maxSamplesNum;
    float32_t maxSampleRate = 1000000.0f / timePerSample;
    
    Serial.print("Tiempo promedio: ");
    Serial.print(avgTime);
    Serial.println(" µs");
    Serial.print("Tiempo/muestra: ");
    Serial.print(timePerSample, 2);
    Serial.println(" µs");
    Serial.print("Fs máxima: ");
    Serial.print(maxSampleRate);
    Serial.println(" Hz");
    Serial.print("Throughput: ");
    Serial.print(maxSamplesNum / (avgTime / 1000000.0f));
    Serial.println(" samples/s");
}

void testSpeedReconstruction(WaveletFilter* filter) {
    Serial.println("\n=== VELOCIDAD RECONSTRUCCIÓN ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < maxSamplesNum; i++) {
            reconstructedSignal[i] = filter->reconstruct(approxCoeffs[i], detailCoeffs[i]);
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / maxSamplesNum;
    
    Serial.print("Tiempo promedio: ");
    Serial.print(avgTime);
    Serial.println(" µs");
    Serial.print("Tiempo/muestra: ");
    Serial.print(timePerSample, 2);
    Serial.println(" µs");
}

void testDecompositionQuality() {
    Serial.println("\n=== CALIDAD DE DESCOMPOSICIÓN ===");
    
    // Realizar descomposición
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        waveletFilter32->processSample(inputSignal[i], &approxCoeffs[i], &detailCoeffs[i]);
    }
    
    // Calcular energías
    float32_t inputEnergy = calculateEnergy(inputSignal, maxSamplesNum);
    float32_t approxEnergy = calculateEnergy(approxCoeffs, maxSamplesNum);
    float32_t detailEnergy = calculateEnergy(detailCoeffs, maxSamplesNum);
    float32_t totalEnergy = approxEnergy + detailEnergy;
    
    Serial.println("\nDISTRIBUCIÓN DE ENERGÍA:");
    Serial.print("  Energía entrada: ");
    Serial.println(inputEnergy, 2);
    Serial.print("  Energía aproximación: ");
    Serial.print(approxEnergy, 2);
    Serial.print(" (");
    Serial.print(100.0f * approxEnergy / inputEnergy);
    Serial.println("%)");
    Serial.print("  Energía detalle: ");
    Serial.print(detailEnergy, 2);
    Serial.print(" (");
    Serial.print(100.0f * detailEnergy / inputEnergy);
    Serial.println("%)");
    Serial.print("  Energía total reconstructed: ");
    Serial.print(totalEnergy, 2);
    Serial.print(" (");
    Serial.print(100.0f * totalEnergy / inputEnergy);
    Serial.println("%)");
    
    // Conservación de energía
    float32_t energyError = abs(inputEnergy - totalEnergy) / inputEnergy * 100.0f;
    Serial.print("  Error conservación energía: ");
    Serial.print(energyError);
    Serial.println("%");
    
    // Reconstrucción perfecta
    Serial.println("\nRECONSTRUCCIÓN PERFECTA:");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        reconstructedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], detailCoeffs[i]);
    }
    
    float32_t reconError = calculateReconstructionError(inputSignal, reconstructedSignal, maxSamplesNum);
    Serial.print("  RMSE reconstrucción: ");
    Serial.println(reconError, 8);
    Serial.print("  Error relativo: ");
    Serial.print(reconError / calculateRMS(inputSignal, maxSamplesNum) * 100.0f);
    Serial.println("%");
    
    // RMS de componentes
    float32_t approxRMS = calculateRMS(approxCoeffs, maxSamplesNum);
    float32_t detailRMS = calculateRMS(detailCoeffs, maxSamplesNum);
    Serial.println("\nRMS DE COMPONENTES:");
    Serial.print("  RMS aproximación: ");
    Serial.println(approxRMS, 6);
    Serial.print("  RMS detalle: ");
    Serial.println(detailRMS, 6);
    Serial.print("  Relación approx/detail: ");
    Serial.println(approxRMS / detailRMS, 2);
}

void testDenoising() {
    Serial.println("\n=== PRUEBAS DE DENOISING ===");
    
    // 1. Denoising con umbral suave
    Serial.println("\n1. UMBRAL SUAVE (soft threshold):");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        float32_t thresholdedDetail = softThreshold(detailCoeffs[i], SOFT_THRESHOLD);
        denoisedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], thresholdedDetail);
    }
    
    float32_t softRMS = calculateRMS(denoisedSignal, maxSamplesNum);
    float32_t inputRMS = calculateRMS(inputSignal, maxSamplesNum);
    float32_t softSNR = calculateSNR(inputSignal, denoisedSignal, maxSamplesNum);
    
    Serial.print("  RMS señal filtrada: ");
    Serial.println(softRMS, 6);
    Serial.print("  Reducción amplitud: ");
    Serial.print(100.0f * (1.0f - softRMS / inputRMS));
    Serial.println("%");
    Serial.print("  SNR mejora: ");
    Serial.print(softSNR);
    Serial.println(" dB");
    
    // 2. Denoising con umbral duro
    Serial.println("\n2. UMBRAL DURO (hard threshold):");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        float32_t thresholdedDetail = hardThreshold(detailCoeffs[i], HARD_THRESHOLD);
        denoisedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], thresholdedDetail);
    }
    
    float32_t hardRMS = calculateRMS(denoisedSignal, maxSamplesNum);
    float32_t hardSNR = calculateSNR(inputSignal, denoisedSignal, maxSamplesNum);
    
    Serial.print("  RMS señal filtrada: ");
    Serial.println(hardRMS, 6);
    Serial.print("  Reducción amplitud: ");
    Serial.print(100.0f * (1.0f - hardRMS / inputRMS));
    Serial.println("%");
    Serial.print("  SNR mejora: ");
    Serial.print(hardSNR);
    Serial.println(" dB");
    
    // 3. Solo aproximación (máximo denoising)
    Serial.println("\n3. SOLO APROXIMACIÓN (detail=0):");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        denoisedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], 0.0f);
    }
    
    float32_t approxOnlyRMS = calculateRMS(denoisedSignal, maxSamplesNum);
    float32_t approxOnlySNR = calculateSNR(inputSignal, denoisedSignal, maxSamplesNum);
    
    Serial.print("  RMS señal filtrada: ");
    Serial.println(approxOnlyRMS, 6);
    Serial.print("  Reducción amplitud: ");
    Serial.print(100.0f * (1.0f - approxOnlyRMS / inputRMS));
    Serial.println("%");
    Serial.print("  SNR mejora: ");
    Serial.print(approxOnlySNR);
    Serial.println(" dB");
    
    Serial.println("\nCOMPARACIÓN:");
    Serial.println("  Método             | RMS    | SNR (dB)");
    Serial.println("  -------------------|--------|----------");
    Serial.print("  Original           | ");
    Serial.print(inputRMS, 4);
    Serial.println(" | N/A");
    Serial.print("  Umbral suave       | ");
    Serial.print(softRMS, 4);
    Serial.print(" | ");
    Serial.println(softSNR, 2);
    Serial.print("  Umbral duro        | ");
    Serial.print(hardRMS, 4);
    Serial.print(" | ");
    Serial.println(hardSNR, 2);
    Serial.print("  Solo aproximación  | ");
    Serial.print(approxOnlyRMS, 4);
    Serial.print(" | ");
    Serial.println(approxOnlySNR, 2);
}

void testEventDetection() {
    Serial.println("\n=== DETECCIÓN DE EVENTOS (QRS) ===");
    
    // Detectar eventos usando diferentes umbrales
    float32_t thresholds[] = {0.2f, 0.3f, 0.4f, 0.5f};
    
    Serial.println("\nUmbral | Eventos | Tasa (events/s)");
    Serial.println("-------|---------|----------------");
    
    for (uint32_t t = 0; t < 4; t++) {
        uint32_t eventCount = detectEvents(detailCoeffs, detectedEvents, 
                                          maxSamplesNum, thresholds[t]);
        float32_t eventRate = eventCount / (maxSamplesNum / SAMPLE_RATE);
        
        Serial.print(thresholds[t], 2);
        Serial.print("   | ");
        Serial.print(eventCount);
        Serial.print("      | ");
        Serial.println(eventRate, 2);
    }
    
    Serial.println("\nNOTA: Frecuencia cardíaca típica: 60-100 bpm (1-1.7 Hz)");
    Serial.println("      Un umbral apropiado debería detectar ~1-1.5 events/s");
}

// ========== SETUP Y LOOP ==========

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("\n========================================");
    Serial.println(" WAVELET FILTER - PRUEBAS DE RENDIMIENTO");
    Serial.println("========================================");
    Serial.println("\nBioFilterLib Test Suite v1.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Transformada: Wavelet Daubechies-4");
    Serial.println("Implementación: Banco de filtros FIR");
    
    // Cargar señal ECG
    Serial.println("\nCargando señal ECG con ruido...");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        inputSignal[i] = ((float32_t)waveformsTable[4][i] - 2048.0f) / 2048.0f;
    }
    Serial.print("Muestras cargadas: ");
    Serial.println(maxSamplesNum);
    
    // Crear instancias de filtros
    Serial.println("Inicializando filtros wavelet...");
    waveletFilter1 = new WaveletFilter(BLOCK_SIZE_1);
    waveletFilter32 = new WaveletFilter(BLOCK_SIZE_32);
    waveletFilter128 = new WaveletFilter(BLOCK_SIZE_128);
    Serial.println("Filtros inicializados.");
    
    // Estadísticas de memoria
    printMemoryStats();
    
    // Pruebas de velocidad - Descomposición
    delay(1000);
    testSpeedDecomposition(waveletFilter1, BLOCK_SIZE_1, "Muestra por muestra");
    delay(500);
    testSpeedDecomposition(waveletFilter32, BLOCK_SIZE_32, "Bloques 32");
    delay(500);
    testSpeedDecomposition(waveletFilter128, BLOCK_SIZE_128, "Bloques 128");
    
    // Pruebas de velocidad - Reconstrucción
    delay(500);
    testSpeedReconstruction(waveletFilter32);
    
    // Pruebas de calidad
    delay(1000);
    testDecompositionQuality();
    
    delay(500);
    testDenoising();
    
    delay(500);
    testEventDetection();
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
    Serial.println("\nRECOMENDACIONES:");
    Serial.println("  1. Para denoising: usar solo aproximación o umbral suave");
    Serial.println("  2. Para detección QRS: usar coeficientes de detalle");
    Serial.println("  3. Block size 32 ofrece mejor balance velocidad/latencia");
}

void loop() {
    delay(1000);
}