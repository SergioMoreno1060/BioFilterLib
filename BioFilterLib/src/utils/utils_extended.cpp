#include "utils_extended.h"
#include "utils.h"
#include <Arduino.h>

// Función para obtener RAM libre en Arduino Due
extern "C" char* sbrk(int incr);

uint32_t getFreeRAM() {
    char top;
    return &top - reinterpret_cast<char*>(sbrk(0));
}

void printSeparator() {
    Serial.println("========================================================================");
}

void printTestHeader(const char* testName) {
    printSeparator();
    Serial.print("  ");
    Serial.println(testName);
    printSeparator();
}

PerformanceMetrics testFilterSpeed_Sample(FIRFilter& filter, 
                                          float32_t* testSignal, 
                                          uint32_t signalLength,
                                          uint32_t fs) {
    PerformanceMetrics metrics;
    
    // Guardar frecuencia de muestreo objetivo
    metrics.targetSampleRate = fs;
    
    // Medir RAM antes del test
    metrics.freeRAM = getFreeRAM();
    
    // Medir tiempo de procesamiento
    uint32_t startTime = micros();
    
    for (uint32_t i = 0; i < signalLength; i++) {
        volatile float32_t filtered = filter.processSample(testSignal[i]);
    }
    
    uint32_t endTime = micros();
    
    // Calcular métricas
    metrics.processingTimeMicros = endTime - startTime;
    
    // Calcular tasa de muestras por segundo
    float32_t timeInSeconds = metrics.processingTimeMicros / 1000000.0f;
    metrics.sampleRate = (uint32_t)(signalLength / timeInSeconds);
    
    // Calcular uso de CPU usando la frecuencia de muestreo real de la señal
    float32_t realTimeRequired = signalLength / (float32_t)fs; // segundos
    metrics.cpuUsagePercent = (timeInSeconds / realTimeRequired) * 100.0f;
    
    return metrics;
}

PerformanceMetrics testFilterSpeed_Buffer(FIRFilter& filter, 
                                          float32_t* testSignal,
                                          float32_t* outputBuffer,
                                          uint32_t signalLength,
                                          uint32_t fs) {
    PerformanceMetrics metrics;
    
    // Guardar frecuencia de muestreo objetivo
    metrics.targetSampleRate = fs;
    
    // Medir RAM antes del test
    metrics.freeRAM = getFreeRAM();
    
    // Medir tiempo de procesamiento
    uint32_t startTime = micros();
    
    filter.processBuffer(testSignal, outputBuffer, signalLength);
    
    uint32_t endTime = micros();
    
    // Calcular métricas
    metrics.processingTimeMicros = endTime - startTime;
    
    // Calcular tasa de muestras por segundo
    float32_t timeInSeconds = metrics.processingTimeMicros / 1000000.0f;
    metrics.sampleRate = (uint32_t)(signalLength / timeInSeconds);
    
    // Calcular uso de CPU usando la frecuencia de muestreo real de la señal
    float32_t realTimeRequired = signalLength / (float32_t)fs; // segundos
    metrics.cpuUsagePercent = (timeInSeconds / realTimeRequired) * 100.0f;
    
    return metrics;
}

QualityMetrics testFilterQuality(const float32_t* cleanSignal,
                                 const float32_t* noisySignal,
                                 const float32_t* filteredSignal,
                                 uint32_t signalLength) {
    QualityMetrics metrics;
    
    // Calcular SNR comparando señal filtrada con señal limpia de referencia
    metrics.snr = calculateSNR(cleanSignal, filteredSignal, signalLength);
    
    // Calcular MSE entre señal filtrada y señal limpia
    metrics.mse = calculateMSE(cleanSignal, filteredSignal, signalLength);
    
    // Calcular correlación entre señal filtrada y señal limpia
    metrics.correlation = calculateCorrelation(cleanSignal, filteredSignal, signalLength);
    
    // Calcular RMS de la señal filtrada
    metrics.rms = calculateRMS(filteredSignal, signalLength);
    
    return metrics;
}

void printPerformanceMetrics(const PerformanceMetrics& metrics) {
    Serial.println("\n--- RESULTADOS DE RENDIMIENTO ---");
    Serial.print("  Tiempo de procesamiento: ");
    Serial.print(metrics.processingTimeMicros);
    Serial.println(" us");
    
    Serial.print("  Tasa de muestreo alcanzada: ");
    Serial.print(metrics.sampleRate);
    Serial.println(" muestras/s");
    
    Serial.print("  Frecuencia de muestreo objetivo: ");
    Serial.print(metrics.targetSampleRate);
    Serial.println(" Hz");
    
    Serial.print("  Uso de CPU (@ ");
    Serial.print(metrics.targetSampleRate);
    Serial.print("Hz): ");
    Serial.print(metrics.cpuUsagePercent, 2);
    Serial.println(" %");
    
    Serial.print("  RAM libre: ");
    Serial.print(metrics.freeRAM);
    Serial.println(" bytes");
    
    // Evaluación de rendimiento basada en la frecuencia objetivo
    Serial.println("\n--- EVALUACION ---");
    
    // Margen de seguridad: 10x la frecuencia de muestreo objetivo
    uint32_t recommendedRate = metrics.targetSampleRate * 10;
    
    if (metrics.sampleRate >= recommendedRate) {
        Serial.println("  >> EXCELENTE: Throughput muy superior al requerido");
        Serial.print("     (");
        Serial.print(metrics.sampleRate / metrics.targetSampleRate);
        Serial.println("x la frecuencia de muestreo)");
    } else if (metrics.sampleRate >= metrics.targetSampleRate * 5) {
        Serial.println("  >> BUENO: Throughput adecuado con margen de seguridad");
        Serial.print("     (");
        Serial.print(metrics.sampleRate / metrics.targetSampleRate);
        Serial.println("x la frecuencia de muestreo)");
    } else if (metrics.sampleRate >= metrics.targetSampleRate) {
        Serial.println("  >> ACEPTABLE: Throughput suficiente pero justo");
        Serial.print("     (");
        Serial.print(metrics.sampleRate / metrics.targetSampleRate);
        Serial.println("x la frecuencia de muestreo)");
    } else {
        Serial.println("  >> ADVERTENCIA: Throughput insuficiente para tiempo real");
        Serial.print("     Requerido: ");
        Serial.print(metrics.targetSampleRate);
        Serial.print(" Hz, Alcanzado: ");
        Serial.print(metrics.sampleRate);
        Serial.println(" Hz");
    }
    
    if (metrics.cpuUsagePercent < 10) {
        Serial.println("  >> Uso de CPU muy eficiente");
    } else if (metrics.cpuUsagePercent < 50) {
        Serial.println("  >> Uso de CPU aceptable");
    } else if (metrics.cpuUsagePercent < 100) {
        Serial.println("  >> ADVERTENCIA: Alto uso de CPU");
    } else {
        Serial.println("  >> CRITICO: CPU sobrecargada, no apto para tiempo real");
    }
}

void printQualityMetrics(const QualityMetrics& metrics) {
    Serial.println("\n--- RESULTADOS DE CALIDAD ---");
    Serial.print("  SNR (Relacion Señal/Ruido): ");
    Serial.print(metrics.snr, 2);
    Serial.println(" dB");
    
    Serial.print("  MSE (Error Cuadratico Medio): ");
    Serial.print(metrics.mse, 6);
    Serial.println("");
    
    Serial.print("  Correlacion con señal limpia: ");
    Serial.print(metrics.correlation, 4);
    Serial.println("");
    
    Serial.print("  RMS de señal filtrada: ");
    Serial.print(metrics.rms, 4);
    Serial.println("");
    
    // Evaluación de calidad
    Serial.println("\n--- EVALUACION ---");
    if (metrics.snr > 20) {
        Serial.println("  >> EXCELENTE: Filtrado muy efectivo");
    } else if (metrics.snr > 10) {
        Serial.println("  >> BUENO: Filtrado efectivo");
    } else if (metrics.snr > 5) {
        Serial.println("  >> ACEPTABLE: Filtrado moderado");
    } else {
        Serial.println("  >> ADVERTENCIA: Filtrado poco efectivo");
    }
    
    if (metrics.correlation > 0.9) {
        Serial.println("  >> Alta fidelidad con señal original");
    } else if (metrics.correlation > 0.7) {
        Serial.println("  >> Buena preservacion de la forma de onda");
    } else {
        Serial.println("  >> ADVERTENCIA: Señal significativamente alterada");
    }
}

void testMemoryUsage(uint16_t numTaps, uint16_t blockSize) {
    Serial.println("\n--- ANALISIS DE MEMORIA ---");
    
    uint32_t ramBefore = getFreeRAM();
    Serial.print("  RAM libre antes: ");
    Serial.print(ramBefore);
    Serial.println(" bytes");
    
    // Calcular memoria teórica requerida
    uint32_t stateSize = (numTaps + blockSize - 1) * sizeof(float32_t);
    uint32_t coeffsSize = numTaps * sizeof(float32_t);
    uint32_t totalTheoretical = stateSize + coeffsSize + sizeof(arm_fir_instance_f32);
    
    Serial.print("  Memoria teorica requerida: ");
    Serial.print(totalTheoretical);
    Serial.println(" bytes");
    Serial.print("    - Buffer de estados: ");
    Serial.print(stateSize);
    Serial.println(" bytes");
    Serial.print("    - Coeficientes: ");
    Serial.print(coeffsSize);
    Serial.println(" bytes");
    Serial.print("    - Estructura CMSIS: ");
    Serial.print(sizeof(arm_fir_instance_f32));
    Serial.println(" bytes");
}