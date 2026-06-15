#ifndef UTILS_EXTENDED_H
#define UTILS_EXTENDED_H

#include <Arduino.h>
#include <arm_math.h>
#include "filters/FIRFilter.h"

/**
 * @brief Estructura para almacenar resultados de pruebas de rendimiento
 */
struct PerformanceMetrics {
    uint32_t processingTimeMicros;      // Tiempo de procesamiento en microsegundos
    uint32_t sampleRate;                // Muestras procesadas por segundo
    float32_t cpuUsagePercent;          // Porcentaje de uso de CPU
    uint32_t freeRAM;                   // RAM libre en bytes
    uint32_t targetSampleRate;          // Frecuencia de muestreo objetivo (Hz)
};

/**
 * @brief Estructura para almacenar resultados de calidad de filtrado
 */
struct QualityMetrics {
    float32_t snr;                      // Relación señal/ruido en dB
    float32_t mse;                      // Error cuadrático medio
    float32_t correlation;              // Correlación con señal de referencia
    float32_t rms;                      // Valor RMS de la señal filtrada
};

/**
 * @brief Imprime encabezado de sección de pruebas
 */
void printTestHeader(const char* testName);

/**
 * @brief Imprime separador visual
 */
void printSeparator();

/**
 * @brief Obtiene la RAM libre disponible en el Arduino Due
 */
uint32_t getFreeRAM();

/**
 * @brief Prueba de rendimiento: mide velocidad de filtrado muestra por muestra
 * @param fs Frecuencia de muestreo de la señal en Hz (default: 1000)
 */
PerformanceMetrics testFilterSpeed_Sample(FIRFilter& filter, 
                                          float32_t* testSignal, 
                                          uint32_t signalLength,
                                          uint32_t fs = 1000);

/**
 * @brief Prueba de rendimiento: mide velocidad de filtrado por buffer
 * @param fs Frecuencia de muestreo de la señal en Hz (default: 1000)
 */
PerformanceMetrics testFilterSpeed_Buffer(FIRFilter& filter, 
                                          float32_t* testSignal,
                                          float32_t* outputBuffer,
                                          uint32_t signalLength,
                                          uint32_t fs = 1000);

/**
 * @brief Prueba de calidad: evalúa la efectividad del filtrado
 */
QualityMetrics testFilterQuality(const float32_t* cleanSignal,
                                 const float32_t* noisySignal,
                                 const float32_t* filteredSignal,
                                 uint32_t signalLength);

/**
 * @brief Imprime métricas de rendimiento de forma estructurada
 */
void printPerformanceMetrics(const PerformanceMetrics& metrics);

/**
 * @brief Imprime métricas de calidad de forma estructurada
 */
void printQualityMetrics(const QualityMetrics& metrics);

/**
 * @brief Prueba de consumo de memoria: reporta uso de RAM antes y después
 */
void testMemoryUsage(uint16_t numTaps, uint16_t blockSize);

#endif  // UTILS_EXTENDED_H