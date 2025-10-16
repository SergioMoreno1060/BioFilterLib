/**
 * @file Wavelet_Test.ino
 * @brief Pruebas de rendimiento y calidad para WaveletFilter usando TestRunner
 * @author Sergio Moreno
 * @version 2.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro Wavelet evaluando:
 * 1. Velocidad de procesamiento (descomposición y reconstrucción)
 * 2. Calidad de descomposición (separación frecuencias)
 * 3. Capacidad de denoising y detección de eventos
 * 
 * Usa señal ECG con ruido (waveformsTable[7]) @ 240Hz
 */

#include <Arduino.h>
#include <BioFilterLib.h>

// ========== CONFIGURACIÓN DE PRUEBAS ==========

#define SIGNAL_TAG      "ecg_60hz_noised_fs240"
#define BLOCK_SIZE_1    1
#define BLOCK_SIZE_32   32
#define BLOCK_SIZE_128  128
#define SAMPLE_RATE     240.0f
#define SPEED_TEST_ITERATIONS 50

// Umbrales para denoising
#define SOFT_THRESHOLD      0.15f
#define HARD_THRESHOLD      0.25f

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[maxSamplesNum];
float32_t approxCoeffs[maxSamplesNum];
float32_t detailCoeffs[maxSamplesNum];
float32_t reconstructedSignal[maxSamplesNum];
float32_t denoisedSignal[maxSamplesNum];
float32_t detectedEvents[maxSamplesNum];

// Instancias de filtros wavelet
WaveletFilter* waveletFilter1;
WaveletFilter* waveletFilter32;
WaveletFilter* waveletFilter128;

// TestRunner
TestRunner testRunner;

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

// ========== FUNCIONES DE TEST ==========

void testSpeedSampleBySample() {
    Serial.println("\n=== VELOCIDAD: Muestra por muestra ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < maxSamplesNum; i++) {
            waveletFilter1->processSample(inputSignal[i], 
                                         &approxCoeffs[i], 
                                         &detailCoeffs[i]);
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / maxSamplesNum;
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
}

void testSpeedBlockProcessing(WaveletFilter* filter, uint32_t blockSize, const char* name) {
    Serial.print("\n=== VELOCIDAD: ");
    Serial.print(name);
    Serial.println(" ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        filter->processBuffer(inputSignal, approxCoeffs, detailCoeffs, maxSamplesNum);
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / maxSamplesNum;
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
}

void testDecompositionQuality() {
    Serial.println("\n=== CALIDAD DE DESCOMPOSICIÓN ===");
    
    // Descomponer señal
    waveletFilter32->processBuffer(inputSignal, approxCoeffs, detailCoeffs, maxSamplesNum);
    
    // Calcular energías
    float32_t energyInput = calculateEnergy(inputSignal, maxSamplesNum);
    float32_t energyApprox = calculateEnergy(approxCoeffs, maxSamplesNum);
    float32_t energyDetail = calculateEnergy(detailCoeffs, maxSamplesNum);
    float32_t energyTotal = energyApprox + energyDetail;
    
    Serial.print("Energía entrada:      ");
    Serial.println(energyInput, 2);
    Serial.print("Energía aproximación: ");
    Serial.print(energyApprox, 2);
    Serial.print(" (");
    Serial.print(100.0f * energyApprox / energyTotal, 1);
    Serial.println("%)");
    Serial.print("Energía detalle:      ");
    Serial.print(energyDetail, 2);
    Serial.print(" (");
    Serial.print(100.0f * energyDetail / energyTotal, 1);
    Serial.println("%)");
    Serial.print("Conservación energía: ");
    Serial.print(100.0f * energyTotal / energyInput, 2);
    Serial.println("%");
}

void testReconstruction() {
    Serial.println("\n=== PRUEBA DE RECONSTRUCCIÓN ===");
    
    // Reconstruir señal completa
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        reconstructedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], detailCoeffs[i]);
    }
    
    float32_t reconError = calculateReconstructionError(inputSignal, reconstructedSignal, maxSamplesNum);
    
    Serial.print("Error de reconstrucción (RMSE): ");
    Serial.println(reconError, 6);
    Serial.print("Error relativo: ");
    Serial.print(100.0f * reconError / calculateRMS(inputSignal, maxSamplesNum), 3);
    Serial.println("%");
    
    if (reconError < 0.01f) {
        Serial.println("Estado: ✓ Reconstrucción perfecta");
    } else if (reconError < 0.05f) {
        Serial.println("Estado: ✓ Reconstrucción aceptable");
    } else {
        Serial.println("Estado: ✗ Error de reconstrucción alto");
    }
}

void testDenoising() {
    Serial.println("\n=== PRUEBAS DE DENOISING ===");
    
    Serial.println("\nMétodo         | RMS    | Mejora (dB)");
    Serial.println("---------------|--------|------------");
    
    float32_t rmsInput = calculateRMS(inputSignal, maxSamplesNum);
    
    // 1. Solo aproximación (sin detalle)
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        denoisedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], 0.0f);
    }
    float32_t rmsApprox = calculateRMS(denoisedSignal, maxSamplesNum);
    float32_t snrApprox = 10.0f * log10(rmsInput / rmsApprox);
    
    Serial.print("Solo aprox.    | ");
    Serial.print(rmsApprox, 4);
    Serial.print(" | ");
    Serial.println(snrApprox, 2);
    
    // 2. Soft thresholding en detalle
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        float32_t softDetail = softThreshold(detailCoeffs[i], SOFT_THRESHOLD);
        denoisedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], softDetail);
    }
    float32_t rmsSoft = calculateRMS(denoisedSignal, maxSamplesNum);
    float32_t snrSoft = 10.0f * log10(rmsInput / rmsSoft);
    
    Serial.print("Soft thresh.   | ");
    Serial.print(rmsSoft, 4);
    Serial.print(" | ");
    Serial.println(snrSoft, 2);
    
    // 3. Hard thresholding en detalle
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        float32_t hardDetail = hardThreshold(detailCoeffs[i], HARD_THRESHOLD);
        denoisedSignal[i] = waveletFilter32->reconstruct(approxCoeffs[i], hardDetail);
    }
    float32_t rmsHard = calculateRMS(denoisedSignal, maxSamplesNum);
    float32_t snrHard = 10.0f * log10(rmsInput / rmsHard);
    
    Serial.print("Hard thresh.   | ");
    Serial.print(rmsHard, 4);
    Serial.print(" | ");
    Serial.println(snrHard, 2);
}

void testEventDetection() {
    Serial.println("\n=== DETECCIÓN DE EVENTOS (QRS) ===");
    
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
    Serial.println("\nBioFilterLib Test Suite v2.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Transformada: Wavelet Daubechies-4");
    Serial.println("Implementación: Banco de filtros FIR");
    Serial.print("Señal: ");
    Serial.println(getSignalName(SIGNAL_TAG));
    
    // Cargar señal ECG
    Serial.println("\nCargando señal de prueba...");
    if (!loadSignal(SIGNAL_TAG, inputSignal)) {
        Serial.println("ERROR: No se pudo cargar la señal");
        while(1);
    }
    Serial.print("Muestras cargadas: ");
    Serial.println(maxSamplesNum);
    
    // Crear instancias de filtros
    Serial.println("Inicializando filtros wavelet...");
    waveletFilter1 = new WaveletFilter(BLOCK_SIZE_1);
    waveletFilter32 = new WaveletFilter(BLOCK_SIZE_32);
    waveletFilter128 = new WaveletFilter(BLOCK_SIZE_128);
    
    if (!waveletFilter1 || !waveletFilter32 || !waveletFilter128) {
        Serial.println("ERROR: No se pudieron crear los filtros");
        while(1);
    }
    
    Serial.println("Filtros inicializados correctamente.");
    
    // ===== PRUEBAS DE VELOCIDAD =====
    delay(1000);
    testSpeedSampleBySample();
    
    delay(500);
    testSpeedBlockProcessing(waveletFilter32, BLOCK_SIZE_32, "Bloques de 32 muestras");
    
    delay(500);
    testSpeedBlockProcessing(waveletFilter128, BLOCK_SIZE_128, "Bloques de 128 muestras");
    
    // ===== PRUEBAS DE DESCOMPOSICIÓN =====
    delay(1000);
    testDecompositionQuality();
    
    delay(500);
    testReconstruction();
    
    delay(500);
    testDenoising();
    
    delay(500);
    testEventDetection();
    
    // ===== PRUEBAS DE CALIDAD CON TESTRUNNER =====
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("  PRUEBAS DE CALIDAD (TestRunner)");
    Serial.println("========================================");
    
    // Probar con la señal configurada
    Serial.println("\n--- Prueba individual ---");
    testRunner.testWavelet(waveletFilter32, SIGNAL_TAG);
    
    // Probar todas las señales disponibles
    delay(500);
    testRunner.testAllWavelet(waveletFilter32);
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
    Serial.println("\nSistema listo.");
}

void loop() {
    delay(1000);
}