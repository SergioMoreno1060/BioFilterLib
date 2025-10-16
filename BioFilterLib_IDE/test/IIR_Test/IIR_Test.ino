/**
 * @file IIR_Test.ino
 * @brief Pruebas de rendimiento y calidad para IIRFilter usando TestRunner
 * @author Sergio Moreno
 * @version 2.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro IIR Biquad evaluando:
 * 1. Velocidad de procesamiento con diferentes tamaños de bloque
 * 2. Calidad de filtrado de diferentes configuraciones (highpass, notch, lowpass)
 * 3. Cascada de filtros para procesamiento completo de ECG
 * 
 * Usa señal ECG con ruido 60Hz (waveformsTable[7]) @ 240Hz con 900 samples
 */

#include <Arduino.h>
#include <BioFilterLib.h>

// ========== CONFIGURACIÓN DE PRUEBAS ==========

#define SIGNAL_TAG      "ecg_60hz_noised_fs240"
#define BLOCK_SIZE_1    1
#define BLOCK_SIZE_32   32
#define BLOCK_SIZE_128  128
#define SPEED_TEST_ITERATIONS 100

// ========== COEFICIENTES DE FILTROS BIQUAD (2º orden) ==========

// Filtro pasa-altas (fc=0.5Hz @ 240Hz) - Elimina deriva de línea base
float32_t highpassCoeffs[5] = {
    0.99585628f, -1.99171257f, 0.99585628f,  // b0, b1, b2
    -1.99170034f, 0.99172479f                 // a1, a2 (NEGADOS)
};

// Filtro Notch (f0=60Hz, Q=30 @ 240Hz) - Elimina interferencia de línea eléctrica
float32_t notchCoeffs[5] = {
    0.96596178f, -1.19322822f, 0.96596178f,   // b0, b1, b2
    -1.19322822f, 0.93192356f                 // a1, a2 (NEGADOS)
};

// Filtro pasa-bajas (fc=100Hz @ 240Hz) - Elimina ruido de alta frecuencia
float32_t lowpassCoeffs[5] = {
    0.29289322f, 0.58578644f, 0.29289322f,   // b0, b1, b2
    -0.17157288f, 0.34314575f                // a1, a2 (NEGADOS)
};

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[maxSamplesNum];
float32_t filteredHighpass[maxSamplesNum];
float32_t filteredNotch[maxSamplesNum];
float32_t filteredLowpass[maxSamplesNum];

// Instancias de filtros
IIRFilter* highpassFilter;
IIRFilter* notchFilter;
IIRFilter* lowpassFilter;

// Para pruebas de velocidad
IIRFilter* testFilter1;
IIRFilter* testFilter32;
IIRFilter* testFilter128;

// TestRunner
TestRunner testRunner;

// ========== FUNCIONES AUXILIARES ==========

float32_t calculateRMS(const float32_t* signal, uint32_t length) {
    float32_t sum = 0.0f;
    for (uint32_t i = 0; i < length; i++) {
        sum += signal[i] * signal[i];
    }
    return sqrt(sum / length);
}

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
    return sqrt(variance / length);
}

// ========== FUNCIONES DE TEST ==========

void testSpeedSampleBySample(IIRFilter* filter) {
    Serial.println("\n=== VELOCIDAD: Muestra por muestra ===");
    
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
    Serial.print(avgTime, 2);
    Serial.println(" µs");
    Serial.print("Tiempo/muestra: ");
    Serial.print(timePerSample, 3);
    Serial.println(" µs");
    Serial.print("Fs máxima: ");
    Serial.print(maxSampleRate, 0);
    Serial.println(" Hz");
}

void testSpeedBlockProcessing(IIRFilter* filter, uint32_t blockSize, const char* name) {
    Serial.print("\n=== VELOCIDAD: ");
    Serial.print(name);
    Serial.println(" ===");
    
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
    Serial.print(avgTime, 2);
    Serial.println(" µs");
    Serial.print("Tiempo/muestra: ");
    Serial.print(timePerSample, 3);
    Serial.println(" µs");
    Serial.print("Fs máxima: ");
    Serial.print(maxSampleRate, 0);
    Serial.println(" Hz");
}

void testCascadeFiltering() {
    Serial.println("\n=== PRUEBA DE CASCADA DE FILTROS ===");
    Serial.println("Aplicando: Highpass -> Notch -> Lowpass");
    
    float32_t rmsInput = calculateRMS(inputSignal, maxSamplesNum);
    Serial.print("\nRMS señal entrada: ");
    Serial.println(rmsInput, 6);
    
    // Aplicar filtros en cascada
    Serial.println("\n1. Aplicando filtro pasa-altas (elimina deriva)...");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        filteredHighpass[i] = highpassFilter->processSample(inputSignal[i]);
    }
    float32_t rmsHighpass = calculateRMS(filteredHighpass, maxSamplesNum);
    Serial.print("   RMS después: ");
    Serial.println(rmsHighpass, 6);
    
    Serial.println("\n2. Aplicando filtro Notch (elimina 60Hz)...");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        filteredNotch[i] = notchFilter->processSample(filteredHighpass[i]);
    }
    float32_t rmsNotch = calculateRMS(filteredNotch, maxSamplesNum);
    Serial.print("   RMS después: ");
    Serial.println(rmsNotch, 6);
    
    Serial.println("\n3. Aplicando filtro pasa-bajas (elimina HF)...");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        filteredLowpass[i] = lowpassFilter->processSample(filteredNotch[i]);
    }
    float32_t rmsLowpass = calculateRMS(filteredLowpass, maxSamplesNum);
    Serial.print("   RMS después: ");
    Serial.println(rmsLowpass, 6);
    
    float32_t totalAttenuation = 10.0f * log10(rmsLowpass / rmsInput);
    Serial.print("\nAtenuación total: ");
    Serial.print(totalAttenuation, 2);
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
    Serial.println("\nBioFilterLib Test Suite v2.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Filtros: IIR Biquad 2º orden");
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
    Serial.println("Inicializando filtros...");
    highpassFilter = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_1);
    notchFilter = new IIRFilter(notchCoeffs, 1, BLOCK_SIZE_1);
    lowpassFilter = new IIRFilter(lowpassCoeffs, 1, BLOCK_SIZE_1);
    
    // Para pruebas de velocidad
    testFilter1 = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_1);
    testFilter32 = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_32);
    testFilter128 = new IIRFilter(highpassCoeffs, 1, BLOCK_SIZE_128);
    
    Serial.println("Filtros inicializados correctamente.");
    
    // ===== PRUEBAS DE VELOCIDAD =====
    delay(1000);
    testSpeedSampleBySample(testFilter1);
    
    delay(500);
    testSpeedBlockProcessing(testFilter32, BLOCK_SIZE_32, "Bloques de 32 muestras");
    
    delay(500);
    testSpeedBlockProcessing(testFilter128, BLOCK_SIZE_128, "Bloques de 128 muestras");
    
    // ===== PRUEBA DE CASCADA =====
    delay(1000);
    testCascadeFiltering();
    
    // ===== PRUEBAS DE CALIDAD CON TESTRUNNER =====
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("  PRUEBAS DE CALIDAD (TestRunner)");
    Serial.println("========================================");
    
    // Probar filtro Notch (mejor para eliminar 60Hz)
    Serial.println("\n--- Prueba individual (Notch 60Hz) ---");
    testRunner.testIIR(notchFilter, SIGNAL_TAG);
    
    // Probar todas las señales disponibles con el filtro Notch
    delay(500);
    testRunner.testAllIIR(notchFilter);
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
    Serial.println("\nSistema listo.");
}

void loop() {
    delay(1000);
}