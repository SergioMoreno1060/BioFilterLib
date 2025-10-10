/**
 * @file Wavelet_Example.cpp
 * @brief Ejemplo de uso de la clase WaveletFilter para análisis tiempo-frecuencia de bioseñales
 * @author Sergio
 * @version 1.0.0
 * @date 2025
 *
 * @details
 * Este ejemplo demuestra cómo configurar y utilizar un filtro Wavelet Daubechies-4 
 * para el análisis tiempo-frecuencia de bioseñales en tiempo real.
 *
 * El ejemplo realiza lo siguiente:
 * 1. Inicializa un filtro wavelet Daubechies-4 optimizado para bioseñales
 * 2. Genera una señal de prueba que simula ECG con ruido superpuesto
 * 3. Aplica descomposición wavelet para separar componentes de frecuencia
 * 4. Demuestra diferentes técnicas de filtrado usando coeficientes wavelet:
 *    - Supresión de ruido (usando solo aproximación)  
 *    - Detección de eventos (analizando coeficientes de detalle)
 *    - Reconstrucción selectiva (filtrado adaptativo)
 * 5. Envía los resultados al puerto serie para visualización en tiempo real
 *
 * @note
 * Para visualizar los resultados, abre el Trazador Serie de Arduino IDE.
 * Verás múltiples trazas que muestran:
 * - Señal original (azul): ECG sintético con ruido
 * - Aproximación (rojo): Componentes de baja frecuencia (morfología base)
 * - Detalle (verde): Componentes de alta frecuencia (ruido y transitorios) 
 * - Señal filtrada (amarillo): ECG reconstruido con supresión de ruido
 *
 * @par Aplicaciones demostradas:
 * - Análisis tiempo-frecuencia de ECG para detección de arritmias
 * - Supresión adaptativa de ruido preservando morfología cardíaca
 * - Detección de complejos QRS usando análisis de coeficientes de detalle
 * - Filtrado selectivo basado en características de la señal
 */

#include <Arduino.h>
#include "BioFilterLib.h"  // Asegúrate de que WaveletFilter.h esté en la carpeta 'src'

// --- 1. Parámetros del Sistema y Señal ---

/**
 * @brief Parámetros de la señal de prueba para simulación de ECG
 * 
 * Estos parámetros definen las características de la señal sintética
 * que simula un ECG realista con diferentes componentes de frecuencia.
 */
const float32_t SAMPLING_FREQUENCY = 250.0f;   // Hz - típico para ECG clínico
const float32_t ECG_HEART_RATE = 72.0f;        // latidos por minuto (normal)
const float32_t QRS_FREQUENCY = 15.0f;         // Hz - frecuencia dominante del QRS
const float32_t T_WAVE_FREQUENCY = 5.0f;       // Hz - frecuencia de onda T
const float32_t NOISE_FREQUENCY = 60.0f;       // Hz - interferencia de línea eléctrica
const float32_t ECG_AMPLITUDE = 1.0f;          // Amplitud base del ECG normalizado
const float32_t NOISE_AMPLITUDE = 0.3f;        // 30% de ruido sobre señal base

/**
 * @brief Parámetros del filtro wavelet y procesamiento
 */
const uint16_t WAVELET_BLOCK_SIZE = 1;          // Procesamiento tiempo real (muestra por muestra)
const float32_t QRS_DETECTION_THRESHOLD = 0.4f; // Umbral para detección de QRS en coeficientes detalle
const float32_t NOISE_SUPPRESSION_FACTOR = 0.1f; // Factor de supresión para filtrado adaptativo

// --- 2. Variables Globales ---

WaveletFilter* ecgWaveletFilter;                // Instancia del filtro wavelet
uint32_t sampleIndex = 0;                      // Índice de muestra para generación de señal
bool qrsDetected = false;                       // Flag para detección de QRS
uint32_t qrsCount = 0;                         // Contador de complejos QRS detectados
uint32_t lastQrsTime = 0;                      // Tiempo de último QRS para cálculo de frecuencia cardíaca

/**
 * @brief Función para generar señal ECG sintética con ruido
 * 
 * Genera una señal que simula un ECG realista combinando:
 * - Onda P: Despolarización auricular (baja frecuencia)
 * - Complejo QRS: Despolarización ventricular (alta frecuencia) 
 * - Onda T: Repolarización ventricular (frecuencia media)
 * - Ruido de línea eléctrica: Interferencia de 60 Hz
 * - Ruido blanco: Ruido aleatorio de instrumentación
 * 
 * @param timeSeconds Tiempo actual en segundos
 * @return float32_t Muestra de ECG sintético
 */
float32_t generateSyntheticECG(float32_t timeSeconds) {
    // Calcular fase del ciclo cardíaco
    float32_t heartCyclePhase = 2.0f * PI * (ECG_HEART_RATE / 60.0f) * timeSeconds;
    
    // Componente QRS (complejo principal) - onda cuadrada modulada
    float32_t qrsComponent = 0.0f;
    float32_t qrsPhase = fmod(heartCyclePhase, 2.0f * PI);
    if (qrsPhase > 0.3f && qrsPhase < 0.7f) {
        // QRS ocurre en ~20% del ciclo cardíaco
        qrsComponent = ECG_AMPLITUDE * sin(2.0f * PI * QRS_FREQUENCY * timeSeconds);
    }
    
    // Onda T (repolarización) - sinusoide de menor frecuencia
    float32_t tWavePhase = heartCyclePhase + PI;  // Desfase para ubicar después del QRS
    float32_t tWaveComponent = 0.3f * ECG_AMPLITUDE * sin(2.0f * PI * T_WAVE_FREQUENCY * timeSeconds) *
                               (0.5f + 0.5f * cos(tWavePhase)); // Ventana para ubicar la onda T
    
    // Línea de base (deriva lenta)
    float32_t baselineComponent = 0.1f * ECG_AMPLITUDE * sin(2.0f * PI * 0.2f * timeSeconds);
    
    // Ruido de línea eléctrica (60 Hz)
    float32_t lineNoiseComponent = NOISE_AMPLITUDE * 0.5f * sin(2.0f * PI * NOISE_FREQUENCY * timeSeconds);
    
    // Ruido aleatorio (simulado con función trigonométrica de alta frecuencia)
    float32_t randomNoiseComponent = NOISE_AMPLITUDE * 0.3f * 
                                    sin(2.0f * PI * 150.0f * timeSeconds + sampleIndex * 0.1f) +
                                    NOISE_AMPLITUDE * 0.2f * 
                                    sin(2.0f * PI * 200.0f * timeSeconds + sampleIndex * 0.07f);
    
    // Combinar todos los componentes
    return qrsComponent + tWaveComponent + baselineComponent + lineNoiseComponent + randomNoiseComponent;
}

/**
 * @brief Función para detectar complejos QRS usando coeficientes de detalle wavelet
 * 
 * Implementa un detector de QRS simple basado en el análisis de los coeficientes
 * de detalle de la transformada wavelet, que son sensibles a los cambios bruscos
 * característicos del complejo QRS.
 * 
 * @param detailCoeff Coeficiente de detalle actual de la transformada wavelet
 * @param currentTime Tiempo actual en milisegundos
 * @return bool True si se detecta un QRS, false en caso contrario
 */
bool detectQRSComplex(float32_t detailCoeff, uint32_t currentTime) {
    static float32_t maxDetailInWindow = 0.0f;
    static uint32_t windowStartTime = 0;
    static bool inRefractory = false;
    static uint32_t refractoryStartTime = 0;
    
    const uint32_t DETECTION_WINDOW_MS = 100;   // Ventana de detección de 100 ms
    const uint32_t REFRACTORY_PERIOD_MS = 200;  // Período refractario de 200 ms
    
    // Período refractario para evitar detecciones múltiples del mismo QRS
    if (inRefractory) {
        if (currentTime - refractoryStartTime > REFRACTORY_PERIOD_MS) {
            inRefractory = false;
        } else {
            return false;  // Estamos en período refractario
        }
    }
    
    // Actualizar ventana deslizante
    if (currentTime - windowStartTime > DETECTION_WINDOW_MS) {
        windowStartTime = currentTime;
        maxDetailInWindow = 0.0f;
    }
    
    // Actualizar máximo en la ventana actual
    if (abs(detailCoeff) > maxDetailInWindow) {
        maxDetailInWindow = abs(detailCoeff);
    }
    
    // Detectar QRS si el coeficiente supera el umbral
    if (abs(detailCoeff) > QRS_DETECTION_THRESHOLD && abs(detailCoeff) == maxDetailInWindow) {
        inRefractory = true;
        refractoryStartTime = currentTime;
        return true;
    }
    
    return false;
}

/**
 * @brief Función de configuración inicial del sistema
 * 
 * Inicializa el puerto serie, crea la instancia del filtro wavelet
 * y muestra información del sistema en el monitor serie.
 */
void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("=== BioFilterLib Wavelet Analysis Demo ===");
    Serial.println("Real-time ECG analysis using Daubechies-4 Wavelet Filter");
    Serial.println("Features demonstrated:");
    Serial.println("- Time-frequency decomposition");
    Serial.println("- Noise suppression using approximation coefficients");
    Serial.println("- QRS detection using detail coefficients");  
    Serial.println("- Adaptive reconstruction with selective filtering");
    Serial.println();
    Serial.println("Open Serial Plotter to visualize results!");
    Serial.println("Expected traces: Original, Approximation, Detail, Filtered");
    Serial.println();
    
    // Inicializar filtro wavelet Daubechies-4 para procesamiento en tiempo real
    Serial.println("Initializing Daubechies-4 Wavelet Filter...");
    ecgWaveletFilter = new WaveletFilter(WAVELET_BLOCK_SIZE);
    
    Serial.println("System ready! Starting real-time wavelet analysis...");
    Serial.println();
    
    delay(2000);  // Pausa para leer mensajes iniciales
}

/**
 * @brief Bucle principal del programa
 * 
 * Ejecuta continuamente el análisis wavelet en tiempo real:
 * 1. Genera muestra sintética de ECG con ruido
 * 2. Aplica descomposición wavelet 
 * 3. Implementa algoritmos de filtrado y detección
 * 4. Envía resultados al puerto serie para visualización
 * 5. Calcula estadísticas en tiempo real (frecuencia cardíaca)
 */
void loop() {
    // Calcular tiempo actual
    uint32_t currentTimeMs = millis();
    float32_t currentTimeSeconds = currentTimeMs / 1000.0f;
    
    // Generar muestra sintética de ECG con ruido
    float32_t noisyECG = generateSyntheticECG(currentTimeSeconds);
    
    // Aplicar descomposición wavelet
    float32_t approxCoeff, detailCoeff;
    ecgWaveletFilter->processSample(noisyECG, &approxCoeff, &detailCoeff);
    
    // === Análisis y Filtrado ===
    
    // 1. Detección de QRS usando coeficientes de detalle
    if (detectQRSComplex(detailCoeff, currentTimeMs)) {
        qrsDetected = true;
        qrsCount++;
        
        // Calcular frecuencia cardíaca instantánea
        if (lastQrsTime > 0) {
            uint32_t rrInterval = currentTimeMs - lastQrsTime;  // Intervalo R-R en ms
            float32_t heartRate = 60000.0f / rrInterval;       // Convertir a latidos por minuto
            
            // Mostrar detección solo ocasionalmente para no saturar el puerto serie
            if (qrsCount % 10 == 0) {
                Serial.print("QRS #"); Serial.print(qrsCount);
                Serial.print(" detected! HR: "); Serial.print(heartRate, 1);
                Serial.println(" bpm");
            }
        }
        lastQrsTime = currentTimeMs;
    }
    
    // 2. Filtrado adaptativo usando aproximación y detalle modificado
    float32_t adaptiveDetailCoeff = detailCoeff;
    
    // Suprimir detalle si es probable que sea ruido (basado en amplitud relativa)
    float32_t noiseThreshold = NOISE_SUPPRESSION_FACTOR * abs(approxCoeff);
    if (abs(detailCoeff) < noiseThreshold) {
        adaptiveDetailCoeff = 0.0f;  // Suprimir detalle de baja amplitud (probablemente ruido)
    }
    
    // 3. Reconstruir señal con diferentes estrategias de filtrado
    // Señal solo con aproximación (máxima supresión de ruido)
    float32_t smoothedECG = ecgWaveletFilter->reconstruct(approxCoeff, 0.0f);
    
    // Señal con filtrado adaptativo (preserva eventos importantes)
    float32_t adaptiveFilteredECG = ecgWaveletFilter->reconstruct(approxCoeff, adaptiveDetailCoeff);
    
    // === Envío de datos para visualización ===
    // Formato: Original, Aproximación, Detalle, Filtrada
    // Usar escalado para mejor visualización en el trazador serie
    Serial.print(noisyECG, 4);                      // Señal original (azul)
    Serial.print(",");
    Serial.print(approxCoeff * 1.2f, 4);            // Aproximación escalada (rojo)
    Serial.print(","); 
    Serial.print(detailCoeff * 5.0f, 4);            // Detalle amplificado (verde)
    Serial.print(",");
    Serial.print(adaptiveFilteredECG, 4);           // Señal filtrada adaptativa (amarillo)
    Serial.println();
    
    // Control de frecuencia de muestreo simulada
    delay(1000.0f / SAMPLING_FREQUENCY);  // Simular fs = 250 Hz
    
    sampleIndex++;
    
    // Reset periódico para evitar overflow del índice
    if (sampleIndex > 1000000) {
        sampleIndex = 0;
        Serial.println("Sample index reset - continuing analysis...");
    }
}

/**
 * @brief Función auxiliar para mostrar estadísticas del sistema (opcional)
 * 
 * Esta función puede ser llamada periódicamente para mostrar estadísticas
 * acumuladas del análisis wavelet, como frecuencia cardíaca promedio,
 * número de QRS detectados, etc.
 * 
 * @note No se usa en el loop principal para no saturar el puerto serie
 */
void showSystemStats() {
    static uint32_t lastStatsTime = 0;
    uint32_t currentTime = millis();
    
    // Mostrar estadísticas cada 10 segundos
    if (currentTime - lastStatsTime > 10000) {
        Serial.println("=== Wavelet Analysis Statistics ===");
        Serial.print("Total QRS complexes detected: ");
        Serial.println(qrsCount);
        
        if (qrsCount > 1 && lastQrsTime > 0) {
            float32_t avgHeartRate = (qrsCount - 1) * 60000.0f / lastQrsTime;
            Serial.print("Average heart rate: ");
            Serial.print(avgHeartRate, 1);
            Serial.println(" bpm");
        }
        
        Serial.print("Analysis duration: ");
        Serial.print(currentTime / 1000.0f, 1);
        Serial.println(" seconds");
        Serial.println("=====================================");
        Serial.println();
        
        lastStatsTime = currentTime;
    }
}