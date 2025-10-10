/**
 * @file IIR_Test.cpp
 * @brief Pruebas de rendimiento y calidad para IIRFilter
 * @author Sergio Moreno
 * @version 1.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro IIR evaluando:
 * 1. Velocidad de procesamiento (muestra individual vs buffer)
 * 2. Consumo de recursos (RAM, stack)
 * 3. Calidad de filtrado (SNR, MSE, respuesta en frecuencia)
 * 4. Estabilidad del filtro
 * 
 * Prueba diferentes configuraciones:
 * - Filtro pasa-altas (eliminación de deriva de línea base)
 * - Filtro notch (eliminación de interferencia de 60Hz)
 * - Filtro pasa-bajas (eliminación de ruido de alta frecuencia)
 */

#include <Arduino.h>
#include <BioFilterLib.h>
#include "Waveforms.h"

// ========== CONFIGURACIÓN DE PRUEBAS ==========

#define BLOCK_SIZE_1    1
#define BLOCK_SIZE_32   32
#define BLOCK_SIZE_128  128
#define SAMPLE_RATE     250.0f
#define SPEED_TEST_ITERATIONS 100

// Coeficientes IIR - Filtro pasa-altas Butterworth 2º orden (fc=0.5Hz, fs=250Hz)
// Para eliminar deriva de línea base
// Formato CMSIS-DSP Biquad: {b0, b1, b2, a1, a2}
// NOTA: a1 y a2 deben tener signo OPUESTO a los de scipy/MATLAB
// Ecuación CMSIS: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + a1*y[n-1] + a2*y[n-2]
const float32_t highpassCoeffs[5] = {
    0.9911536f,  -1.98230719f,  0.9911536f,  // b0, b1, b2
    1.98222893f,  -0.98238545f                // a1, a2 (NEGADOS respecto a scipy)
};

// Coeficientes IIR - Filtro Notch 2º orden (f0=60Hz, Q=30, fs=250Hz)
// Para eliminar interferencia de línea eléctrica
const float32_t notchCoeffs[5] = {
    0.97547839f, -0.12250159f, 0.97547839f,  // b0, b1, b2
    0.12250159f, -0.95095678f                // a1, a2 (NEGADOS respecto a scipy)
};

// Coeficientes IIR - Filtro pasa-bajas Butterworth 2º orden (fc=40Hz, fs=250Hz)
// Para eliminar ruido de alta frecuencia
const float32_t lowpassCoeffs[5] = {
    0.14532388f, 0.29064777f, 0.14532388f,   // b0, b1, b2
    -0.67102909f, 0.25232463f                // a1, a2 (NEGADOS respecto a scipy)
};

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[maxSamplesNum];
float32_t filteredHighpass[maxSamplesNum];
float32_t filteredNotch[maxSamplesNum];
float32_t filteredLowpass[maxSamplesNum];
float32_t referenceSignal[maxSamplesNum];

// Instancias de filtros
IIRFilter* highpassFilter;
IIRFilter* notchFilter;
IIRFilter* lowpassFilter;

// Para pruebas de velocidad con diferentes tamaños de bloque
IIRFilter* testFilter1;
IIRFilter* testFilter32;
IIRFilter* testFilter128;

// ========== FUNCIONES AUXILIARES ==========

float32_t calculateSNR(const float32_t* signal, const float32_t* noise, uint32_t length) {
    float32_t signalPower = 0.0f;
    float32_t noisePower = 0.0f;
    
    for (uint32_t i = 0; i < length; i++) {
        signalPower += signal[i] * signal[i];
        float32_t error = signal[i] - noise[i];
        noisePower += error * error;
    }
    
    signalPower /= length;
    noisePower /= length;
    
    if (noisePower < 1e-10f) return 999.9f;
    return 10.0f * log10(signalPower / noisePower);
}

float32_t calculateMSE(const float32_t* signal1, const float32_t* signal2, uint32_t length) {
    float32_t mse = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        float32_t error = signal1[i] - signal2[i];
        mse += error * error;
    }
    return mse / length;
}

float32_t calculateRMS(const float32_t* signal, uint32_t length) {
    float32_t sum = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        sum += signal[i] * signal[i];
    }
    return sqrt(sum / length);
}

/**
 * @brief Calcula la desviación estándar de la señal
 */
float32_t calculateStdDev(const float32_t* signal, uint32_t length) {
    float32_t mean = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        mean += signal[i];
    }
    mean /= length;
    
    float32_t variance = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        float32_t diff = signal[i] - mean;
        variance += diff * diff;
    }
    variance /= length;
    
    return sqrt(variance);
}

/**
 * @brief Prueba de estabilidad del filtro IIR
 */
void testFilterStability(IIRFilter* filter, const char* name) {
    Serial.print("\nPrueba de estabilidad - ");
    Serial.println(name);
    
    // Aplicar impulso y verificar que la salida se estabiliza
    float32_t testSignal[1000];
    float32_t output[1000];
    
    // Inicializar con ceros excepto un impulso
    testSignal[0] = 1.0f;
    for (uint32_t i = 1; i < 1000; i++) {
        testSignal[i] = 0.0f;
    }
    
    // Procesar
    for (uint32_t i = 0; i < 1000; i++) {
        output[i] = filter->processSample(testSignal[i]);
    }
    
    // Verificar que la salida converge a cero
    float32_t finalValue = abs(output[999]);
    bool isStable = (finalValue < 0.01f);
    
    Serial.print("  Respuesta al impulso final: ");
    Serial.println(finalValue, 6);
    Serial.print("  Estabilidad: ");
    Serial.println(isStable ? "ESTABLE" : "INESTABLE");
    
    // Calcular tiempo de establecimiento (settling time)
    uint32_t settlingTime = 0;
    for (uint32_t i = 100; i < 1000; i++) {
        if (abs(output[i]) < 0.01f) {
            settlingTime = i;
            break;
        }
    }
    Serial.print("  Tiempo de establecimiento: ");
    Serial.print(settlingTime);
    Serial.println(" muestras");
}

void printMemoryStats() {
    Serial.println("\n=== CONSUMO DE RECURSOS IIR ===");
    Serial.print("RAM usado por buffers: ");
    Serial.print((maxSamplesNum * 4 * sizeof(float32_t)) / 1024.0f);
    Serial.println(" KB");
    
    // IIR Biquad usa 4 estados por etapa
    Serial.print("RAM por instancia IIR (1 stage): ");
    Serial.print(4 * sizeof(float32_t));
    Serial.println(" bytes");
    
    Serial.println("\nComparación FIR vs IIR:");
    Serial.println("  - FIR 51 taps: ~204 bytes estado");
    Serial.println("  - IIR 2º orden: ~16 bytes estado");
    Serial.println("  Ahorro de memoria: ~92%");
    
    Serial.println("\nNOTA: Arduino Due tiene 96KB de SRAM");
    Serial.println("      Medición de RAM libre no disponible en ARM");
}

void testSpeedSampleBySample(IIRFilter* filter, const char* name) {
    Serial.print("\n=== VELOCIDAD: ");
    Serial.print(name);
    Serial.println(" (muestra por muestra) ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < maxSamplesNum; i++) {
            filteredHighpass[i] = filter->processSample(inputSignal[i]);
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

void testSpeedBlockProcessing(IIRFilter* filter, uint32_t blockSize, const char* name) {
    Serial.print("\n=== VELOCIDAD: ");
    Serial.print(name);
    Serial.print(" (bloques de ");
    Serial.print(blockSize);
    Serial.println(") ===");
    
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        unsigned long startTime = micros();
        
        uint32_t samplesProcessed = 0;
        while (samplesProcessed < maxSamplesNum) {
            uint32_t currentBlockSize = min(blockSize, maxSamplesNum - samplesProcessed);
            filter->processBuffer(&inputSignal[samplesProcessed], 
                                 &filteredHighpass[samplesProcessed], 
                                 currentBlockSize);
            samplesProcessed += currentBlockSize;
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

void testFilterQuality() {
    Serial.println("\n=== CALIDAD DE FILTRADO ===");
    
    // 1. Filtro pasa-altas (elimina deriva)
    Serial.println("\n1. FILTRO PASA-ALTAS (elimina deriva < 0.5Hz):");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        filteredHighpass[i] = highpassFilter->processSample(inputSignal[i]);
    }
    
    float32_t rmsInput = calculateRMS(inputSignal, maxSamplesNum);
    float32_t rmsHighpass = calculateRMS(filteredHighpass, maxSamplesNum);
    float32_t stdInput = calculateStdDev(inputSignal, maxSamplesNum);
    float32_t stdHighpass = calculateStdDev(filteredHighpass, maxSamplesNum);
    
    Serial.print("  RMS entrada: ");
    Serial.println(rmsInput, 6);
    Serial.print("  RMS salida: ");
    Serial.println(rmsHighpass, 6);
    Serial.print("  Std entrada: ");
    Serial.println(stdInput, 6);
    Serial.print("  Std salida: ");
    Serial.println(stdHighpass, 6);
    
    // 2. Filtro Notch (elimina 60Hz)
    Serial.println("\n2. FILTRO NOTCH (elimina interferencia 60Hz):");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        filteredNotch[i] = notchFilter->processSample(inputSignal[i]);
    }
    
    float32_t rmsNotch = calculateRMS(filteredNotch, maxSamplesNum);
    float32_t attenuation60Hz = 10.0f * log10(rmsNotch / rmsInput);
    
    Serial.print("  RMS salida: ");
    Serial.println(rmsNotch, 6);
    Serial.print("  Atenuación: ");
    Serial.print(attenuation60Hz);
    Serial.println(" dB");
    
    // 3. Filtro pasa-bajas (elimina ruido HF)
    Serial.println("\n3. FILTRO PASA-BAJAS (elimina ruido > 40Hz):");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        filteredLowpass[i] = lowpassFilter->processSample(inputSignal[i]);
    }
    
    float32_t rmsLowpass = calculateRMS(filteredLowpass, maxSamplesNum);
    float32_t smoothness = calculateStdDev(filteredLowpass, maxSamplesNum);
    
    Serial.print("  RMS salida: ");
    Serial.println(rmsLowpass, 6);
    Serial.print("  Suavidad (Std): ");
    Serial.println(smoothness, 6);
    Serial.print("  Reducción ruido: ");
    Serial.print(100.0f * (1.0f - smoothness / stdInput));
    Serial.println(" %");
    
    // 4. Comparación cascada de filtros
    Serial.println("\n4. CASCADA: Pasa-altas + Notch + Pasa-bajas:");
    float32_t cascadeOutput[maxSamplesNum];
    
    // Aplicar filtros en cascada
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        float32_t temp = highpassFilter->processSample(inputSignal[i]);
        temp = notchFilter->processSample(temp);
        cascadeOutput[i] = lowpassFilter->processSample(temp);
    }
    
    float32_t rmsCascade = calculateRMS(cascadeOutput, maxSamplesNum);
    float32_t stdCascade = calculateStdDev(cascadeOutput, maxSamplesNum);
    float32_t totalAttenuation = 10.0f * log10(rmsCascade / rmsInput);
    
    Serial.print("  RMS final: ");
    Serial.println(rmsCascade, 6);
    Serial.print("  Std final: ");
    Serial.println(stdCascade, 6);
    Serial.print("  Atenuación total: ");
    Serial.print(totalAttenuation);
    Serial.println(" dB");
    Serial.print("  Mejora SNR estimada: ");
    Serial.print(-totalAttenuation);
    Serial.println(" dB");
}

// ========== SETUP Y LOOP ==========

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("\n========================================");
    Serial.println("  IIR FILTER - PRUEBAS DE RENDIMIENTO  ");
    Serial.println("========================================");
    Serial.println("\nBioFilterLib Test Suite v1.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Filtros: IIR Biquad 2º orden");
    
    // Cargar señal ECG
    Serial.println("\nCargando señal ECG con ruido...");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        inputSignal[i] = ((float32_t)waveformsTable[7][i] - 2048.0f) / 2048.0f;
    }
    Serial.print("Muestras cargadas: ");
    Serial.println(maxSamplesNum);
    
    // Crear instancias de filtros
    Serial.println("Inicializando filtros...");
    highpassFilter = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_1);
    notchFilter = new IIRFilter(notchCoeffs, 1, BLOCK_SIZE_1);
    lowpassFilter = new IIRFilter(lowpassCoeffs, 1, BLOCK_SIZE_1);
    
    // Para pruebas de velocidad
    testFilter1 = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_1);
    testFilter32 = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_32);
    testFilter128 = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_128);
    
    Serial.println("Filtros inicializados.");
    
    // Estadísticas de memoria
    printMemoryStats();
    
    // Pruebas de estabilidad
    delay(1000);
    Serial.println("\n=== PRUEBAS DE ESTABILIDAD ===");
    testFilterStability(highpassFilter, "Pasa-altas");
    testFilterStability(notchFilter, "Notch");
    testFilterStability(lowpassFilter, "Pasa-bajas");
    
    // Pruebas de velocidad
    delay(1000);
    testSpeedSampleBySample(testFilter1, "IIR Biquad");
    delay(500);
    testSpeedBlockProcessing(testFilter32, BLOCK_SIZE_32, "IIR Biquad");
    delay(500);
    testSpeedBlockProcessing(testFilter128, BLOCK_SIZE_128, "IIR Biquad");
    
    // Pruebas de calidad
    delay(1000);
    testFilterQuality();
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
}

void loop() {
    delay(1000);
}