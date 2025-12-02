/**
 * @file Test_Bio.ino
 * @brief Test completo para la librería BioFilterLib (FIRFilter)
 * @author Test Suite
 * @version 2.0.0
 * @date 2025
 * 
 * Este test evalúa:
 * - Velocidad de procesamiento (muestra por muestra y por buffer)
 * - Consumo de recursos (RAM)
 * - Calidad de filtrado (SNR, MSE, Correlación)
 * 
 * Usa señales ECG reales de Waveforms.h para pruebas realistas
 */

#include <BioFilterLib.h>

// ============================================================================
// CONFIGURACIÓN DEL TEST
// ============================================================================

// Parámetros del filtro
#define FILTERTAPS 51             // Número de taps del filtro
#define SAMPLE_RATE 960           // Frecuencia de muestreo (Hz)
#define TEST_SAMPLES 1000         // Número de muestras a procesar
#define BLOCK_SIZE 1              // Tamaño de bloque (1 para procesamiento muestra por muestra)

// Coeficientes del filtro paso-bajo (fc=40Hz @ fs=960Hz)
// Diseñado con ventana de Hamming para atenuar ruido de 60Hz
float32_t coefs[FILTERTAPS] = {
    +0.00096226f, +0.00110652f, +0.00123488f, +0.00128605f, +0.00115012f, +0.00068979f, -0.00022373f, -0.00166613f,
    -0.00361514f, -0.00591523f, -0.00826092f, -0.01020548f, -0.01119776f, -0.01064514f, -0.00799553f, -0.00282748f,
    +0.00506537f, +0.01560960f, +0.02841897f, +0.04280137f, +0.05780805f, +0.07232126f, +0.08517038f, +0.09526185f,
    +0.10170564f, +0.10392083f, +0.10170564f, +0.09526185f, +0.08517038f, +0.07232126f, +0.05780805f, +0.04280137f,
    +0.02841897f, +0.01560960f, +0.00506537f, -0.00282748f, -0.00799553f, -0.01064514f, -0.01119776f, -0.01020548f,
    -0.00826092f, -0.00591523f, -0.00361514f, -0.00166613f, -0.00022373f, +0.00068979f, +0.00115012f, +0.00128605f,
    +0.00123488f, +0.00110652f, +0.00096226f
};

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

float32_t cleanSignal[TEST_SAMPLES];        // Señal ECG limpia
float32_t noisySignal[TEST_SAMPLES];        // Señal ECG con ruido 60Hz
float32_t filteredSignal[TEST_SAMPLES];     // Señal filtrada

// Métricas extendidas para este test
struct TestPerformanceMetrics {
    uint32_t processingTimeMicros;
    uint32_t sampleRate;
    float32_t cpuUsagePercent;
    uint32_t freeRAMBefore;
    uint32_t freeRAMAfter;
};

struct TestQualityMetrics {
    float32_t snrInput;      // SNR antes del filtrado
    float32_t snrOutput;     // SNR después del filtrado
    float32_t snrImprovement;
    float32_t mse;
    float32_t correlation;
    float32_t rmsClean;
    float32_t rmsNoisy;
    float32_t rmsFiltered;
};

// ============================================================================
// FUNCIONES AUXILIARES
// ============================================================================

void printHeader(const char* text) {
    printSeparator();
    Serial.print("  ");
    Serial.println(text);
    printSeparator();
}

// ============================================================================
// FUNCIONES DE PRUEBA
// ============================================================================

/**
 * @brief Carga las señales de prueba desde Waveforms.h
 */
bool loadTestSignals() {
    Serial.println("\n>> Cargando señales de prueba...");
    
    // Cargar señal limpia
    if (!loadSignal(cleanSignal, "ecg_clean", TEST_SAMPLES)) {
        Serial.println("ERROR: No se pudo cargar señal limpia");
        return false;
    }
    Serial.print("   - Señal limpia cargada: ");
    Serial.print(TEST_SAMPLES);
    Serial.println(" muestras");
    
    // Cargar señal con ruido
    if (!loadSignal(noisySignal, "ecg_60hz_noised", TEST_SAMPLES)) {
        Serial.println("ERROR: No se pudo cargar señal con ruido");
        return false;
    }
    Serial.print("   - Señal con ruido cargada: ");
    Serial.print(TEST_SAMPLES);
    Serial.println(" muestras");
    
    Serial.println("   OK: Señales cargadas correctamente");
    return true;
}

/**
 * @brief Crea y configura el filtro FIR
 */
FIRFilter* createFilter() {
    Serial.println("\n>> Configurando filtro FIR (BioFilterLib)...");
    
    // Crear filtro con coeficientes, número de taps y tamaño de bloque
    FIRFilter* filter = new FIRFilter(coefs, FILTERTAPS, BLOCK_SIZE);
    
    Serial.print("   - Número de taps: ");
    Serial.println(FILTERTAPS);
    Serial.print("   - Frecuencia de muestreo: ");
    Serial.print(SAMPLE_RATE);
    Serial.println(" Hz");
    Serial.print("   - Tamaño de bloque: ");
    Serial.println(BLOCK_SIZE);
    Serial.println("   OK: Filtro configurado");
    
    return filter;
}

/**
 * @brief Procesa la señal con ruido y mide rendimiento (método muestra por muestra)
 */
TestPerformanceMetrics processSignalSampleBysample(FIRFilter& filter) {
    TestPerformanceMetrics metrics;
    
    Serial.println("\n>> Procesando señal (muestra por muestra)...");
    
    // Medir RAM antes
    metrics.freeRAMBefore = getFreeRAM();
    
    // Medir tiempo de procesamiento
    uint32_t startTime = micros();
    
    for (uint32_t i = 0; i < TEST_SAMPLES; i++) {
        filteredSignal[i] = filter.processSample(noisySignal[i]);
    }
    
    uint32_t endTime = micros();
    
    // Medir RAM después
    metrics.freeRAMAfter = getFreeRAM();
    
    // Calcular métricas
    metrics.processingTimeMicros = endTime - startTime;
    
    // Calcular tasa de muestras por segundo
    float32_t timeInSeconds = metrics.processingTimeMicros / 1000000.0f;
    metrics.sampleRate = (uint32_t)(TEST_SAMPLES / timeInSeconds);
    
    // Calcular uso de CPU
    float32_t realTimeRequired = TEST_SAMPLES / (float32_t)SAMPLE_RATE;
    metrics.cpuUsagePercent = (timeInSeconds / realTimeRequired) * 100.0f;
    
    Serial.print("   - Muestras procesadas: ");
    Serial.println(TEST_SAMPLES);
    Serial.print("   - Tiempo de procesamiento: ");
    Serial.print(metrics.processingTimeMicros);
    Serial.println(" µs");
    Serial.println("   OK: Señal procesada");
    
    return metrics;
}

/**
 * @brief Procesa la señal con ruido y mide rendimiento (método por buffer)
 */
TestPerformanceMetrics processSignalBuffer(FIRFilter& filter) {
    TestPerformanceMetrics metrics;
    
    Serial.println("\n>> Procesando señal (por buffer)...");
    
    // Medir RAM antes
    metrics.freeRAMBefore = getFreeRAM();
    
    // Limpiar señal filtrada
    for (uint32_t i = 0; i < TEST_SAMPLES; i++) {
        filteredSignal[i] = 0.0f;
    }
    
    // Medir tiempo de procesamiento
    uint32_t startTime = micros();
    
    filter.processBuffer(noisySignal, filteredSignal, TEST_SAMPLES);
    
    uint32_t endTime = micros();
    
    // Medir RAM después
    metrics.freeRAMAfter = getFreeRAM();
    
    // Calcular métricas
    metrics.processingTimeMicros = endTime - startTime;
    
    // Calcular tasa de muestras por segundo
    float32_t timeInSeconds = metrics.processingTimeMicros / 1000000.0f;
    metrics.sampleRate = (uint32_t)(TEST_SAMPLES / timeInSeconds);
    
    // Calcular uso de CPU
    float32_t realTimeRequired = TEST_SAMPLES / (float32_t)SAMPLE_RATE;
    metrics.cpuUsagePercent = (timeInSeconds / realTimeRequired) * 100.0f;
    
    Serial.print("   - Muestras procesadas: ");
    Serial.println(TEST_SAMPLES);
    Serial.print("   - Tiempo de procesamiento: ");
    Serial.print(metrics.processingTimeMicros);
    Serial.println(" µs");
    Serial.println("   OK: Señal procesada");
    
    return metrics;
}

/**
 * @brief Evalúa la calidad del filtrado
 */
TestQualityMetrics evaluateQuality() {
    TestQualityMetrics metrics;
    
    Serial.println("\n>> Evaluando calidad del filtrado...");
    
    // SNR de entrada (señal limpia vs señal con ruido)
    metrics.snrInput = calculateSNR(cleanSignal, noisySignal, TEST_SAMPLES);
    
    // SNR de salida (señal limpia vs señal filtrada)
    // Compensar el delay del filtro (FILTERTAPS/2 = 25 muestras)
    uint32_t delay = FILTERTAPS / 2;
    metrics.snrOutput = calculateSNR(cleanSignal, filteredSignal + delay, TEST_SAMPLES - delay);
    
    // Mejora de SNR
    metrics.snrImprovement = metrics.snrOutput - metrics.snrInput;
    
    // Error cuadrático medio entre señal limpia y filtrada
    metrics.mse = calculateMSE(cleanSignal, filteredSignal + delay, TEST_SAMPLES - delay);
    
    // Correlación entre señal limpia y filtrada
    metrics.correlation = calculateCorrelation(cleanSignal, filteredSignal + delay, TEST_SAMPLES - delay);
    
    // Valores RMS
    metrics.rmsClean = calculateRMS(cleanSignal, TEST_SAMPLES);
    metrics.rmsNoisy = calculateRMS(noisySignal, TEST_SAMPLES);
    metrics.rmsFiltered = calculateRMS(filteredSignal, TEST_SAMPLES);
    
    Serial.println("   OK: Métricas calculadas");
    
    return metrics;
}

/**
 * @brief Imprime resultados de rendimiento
 */
void printPerformanceResults(const TestPerformanceMetrics& metrics, const char* method) {
    printHeader("RESULTADOS DE RENDIMIENTO");
    
    Serial.print("\n--- Método: ");
    Serial.print(method);
    Serial.println(" ---");
    
    Serial.println("\n--- Tiempos de Procesamiento ---");
    Serial.print("  Tiempo total: ");
    Serial.print(metrics.processingTimeMicros);
    Serial.println(" µs");
    
    Serial.print("  Tiempo por muestra: ");
    Serial.print((float)metrics.processingTimeMicros / TEST_SAMPLES, 2);
    Serial.println(" µs/muestra");
    
    Serial.println("\n--- Throughput ---");
    Serial.print("  Tasa alcanzada: ");
    Serial.print(metrics.sampleRate);
    Serial.println(" muestras/s");
    
    Serial.print("  Tasa objetivo: ");
    Serial.print(SAMPLE_RATE);
    Serial.println(" Hz");
    
    Serial.print("  Factor de velocidad: ");
    Serial.print((float)metrics.sampleRate / SAMPLE_RATE, 1);
    Serial.println("x");
    
    Serial.println("\n--- Uso de CPU ---");
    Serial.print("  CPU (@ ");
    Serial.print(SAMPLE_RATE);
    Serial.print(" Hz): ");
    Serial.print(metrics.cpuUsagePercent, 2);
    Serial.println(" %");
    
    Serial.println("\n--- Memoria RAM ---");
    Serial.print("  RAM libre antes: ");
    Serial.print(metrics.freeRAMBefore);
    Serial.println(" bytes");
    
    Serial.print("  RAM libre después: ");
    Serial.print(metrics.freeRAMAfter);
    Serial.println(" bytes");
    
    Serial.print("  RAM utilizada: ");
    Serial.print(metrics.freeRAMBefore - metrics.freeRAMAfter);
    Serial.println(" bytes");
    
    // Evaluación
    Serial.println("\n--- EVALUACIÓN DE RENDIMIENTO ---");
    
    if (metrics.sampleRate >= SAMPLE_RATE * 10) {
        Serial.println("  >> EXCELENTE: Throughput muy superior al requerido");
    } else if (metrics.sampleRate >= SAMPLE_RATE * 5) {
        Serial.println("  >> BUENO: Throughput adecuado con margen");
    } else if (metrics.sampleRate >= SAMPLE_RATE) {
        Serial.println("  >> ACEPTABLE: Throughput suficiente");
    } else {
        Serial.println("  >> ADVERTENCIA: Throughput insuficiente para tiempo real");
    }
    
    if (metrics.cpuUsagePercent < 10) {
        Serial.println("  >> Uso de CPU muy eficiente");
    } else if (metrics.cpuUsagePercent < 50) {
        Serial.println("  >> Uso de CPU aceptable");
    } else if (metrics.cpuUsagePercent < 100) {
        Serial.println("  >> ADVERTENCIA: Alto uso de CPU");
    } else {
        Serial.println("  >> CRÍTICO: CPU sobrecargada");
    }
}

/**
 * @brief Imprime resultados de calidad
 */
void printQualityResults(const TestQualityMetrics& metrics) {
    printHeader("RESULTADOS DE CALIDAD");
    
    Serial.println("\n--- SNR (Relación Señal/Ruido) ---");
    Serial.print("  SNR entrada (con ruido): ");
    Serial.print(metrics.snrInput, 2);
    Serial.println(" dB");
    
    Serial.print("  SNR salida (filtrada): ");
    Serial.print(metrics.snrOutput, 2);
    Serial.println(" dB");
    
    Serial.print("  Mejora de SNR: ");
    Serial.print(metrics.snrImprovement, 2);
    Serial.println(" dB");
    
    Serial.println("\n--- Error y Correlación ---");
    Serial.print("  MSE (vs señal limpia): ");
    Serial.print(metrics.mse, 6);
    Serial.println("");
    
    Serial.print("  Correlación (vs señal limpia): ");
    Serial.print(metrics.correlation, 4);
    Serial.println("");
    
    Serial.println("\n--- Valores RMS ---");
    Serial.print("  RMS señal limpia: ");
    Serial.print(metrics.rmsClean, 4);
    Serial.println("");
    
    Serial.print("  RMS señal con ruido: ");
    Serial.print(metrics.rmsNoisy, 4);
    Serial.println("");
    
    Serial.print("  RMS señal filtrada: ");
    Serial.print(metrics.rmsFiltered, 4);
    Serial.println("");
    
    // Evaluación
    Serial.println("\n--- EVALUACIÓN DE CALIDAD ---");
    
    if (metrics.snrImprovement > 10) {
        Serial.println("  >> EXCELENTE: Gran mejora en la calidad");
    } else if (metrics.snrImprovement > 5) {
        Serial.println("  >> BUENO: Mejora significativa");
    } else if (metrics.snrImprovement > 2) {
        Serial.println("  >> ACEPTABLE: Mejora moderada");
    } else {
        Serial.println("  >> ADVERTENCIA: Mejora limitada");
    }
    
    if (metrics.correlation > 0.95) {
        Serial.println("  >> Excelente preservación de la forma de onda");
    } else if (metrics.correlation > 0.85) {
        Serial.println("  >> Buena preservación de la forma de onda");
    } else if (metrics.correlation > 0.70) {
        Serial.println("  >> Preservación aceptable");
    } else {
        Serial.println("  >> ADVERTENCIA: Señal significativamente alterada");
    }
    
    if (metrics.snrOutput > 20) {
        Serial.println("  >> Ruido residual muy bajo");
    } else if (metrics.snrOutput > 10) {
        Serial.println("  >> Ruido residual bajo");
    } else {
        Serial.println("  >> Ruido residual perceptible");
    }
}

/**
 * @brief Imprime resumen de la configuración
 */
void printConfiguration() {
    printHeader("CONFIGURACIÓN DEL TEST");
    
    Serial.println("\n--- Filtro ---");
    Serial.print("  Tipo: FIR (BioFilterLib/CMSIS-DSP)");
    Serial.println("");
    Serial.print("  Número de taps: ");
    Serial.println(FILTERTAPS);
    Serial.print("  Tipo: Paso-bajo");
    Serial.println("");
    Serial.print("  Frecuencia de corte: ~40 Hz");
    Serial.println("");
    Serial.print("  Tamaño de bloque: ");
    Serial.println(BLOCK_SIZE);
    
    Serial.println("\n--- Señales ---");
    Serial.print("  Señal limpia: ");
    Serial.println(getSignalName("ecg_clean"));
    Serial.print("  Señal con ruido: ");
    Serial.println(getSignalName("ecg_60hz_noised"));
    Serial.print("  Frecuencia de muestreo: ");
    Serial.print(SAMPLE_RATE);
    Serial.println(" Hz");
    Serial.print("  Muestras a procesar: ");
    Serial.println(TEST_SAMPLES);
    Serial.print("  Duración: ");
    Serial.print((float)TEST_SAMPLES / SAMPLE_RATE, 2);
    Serial.println(" segundos");
    
    Serial.println("\n--- Hardware ---");
    Serial.print("  Plataforma: Arduino Due");
    Serial.println("");
    Serial.print("  CPU: ARM Cortex-M3 @ 84 MHz");
    Serial.println("");
    Serial.print("  RAM disponible: ");
    Serial.print(getFreeRAM());
    Serial.println(" bytes");
}

// ============================================================================
// SETUP Y LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000); // Esperar puerto serial
    
    delay(1000); // Dar tiempo para abrir monitor serial
    
    // Título
    Serial.println("\n\n");
    printSeparator();
    Serial.println("  TEST COMPLETO - LIBRERÍA BIOFILTERLIB (FIRFilter)");
    Serial.println("  Evaluación de Rendimiento y Calidad");
    printSeparator();
    
    // Mostrar configuración
    printConfiguration();
    
    // Cargar señales
    if (!loadTestSignals()) {
        Serial.println("\nERROR CRÍTICO: No se pudieron cargar las señales");
        while (1); // Detener ejecución
    }
    
    // Crear filtro
    FIRFilter* filter = createFilter();
    
    if (!filter) {
        Serial.println("\nERROR CRÍTICO: No se pudo crear el filtro");
        while (1);
    }
    
    // ========================================================================
    // TEST 1: Procesamiento muestra por muestra
    // ========================================================================
    Serial.println("\n");
    printHeader("TEST 1: PROCESAMIENTO MUESTRA POR MUESTRA");
    
    TestPerformanceMetrics perfMetrics1 = processSignalSampleBysample(*filter);
    TestQualityMetrics qualMetrics1 = evaluateQuality();
    
    printPerformanceResults(perfMetrics1, "Muestra por muestra (processSample)");
    Serial.println("\n");
    printQualityResults(qualMetrics1);
    
    // ========================================================================
    // TEST 2: Procesamiento por buffer
    // ========================================================================
    Serial.println("\n");
    printHeader("TEST 2: PROCESAMIENTO POR BUFFER");
    
    // Recrear el filtro para resetear el estado
    delete filter;
    filter = createFilter();
    
    TestPerformanceMetrics perfMetrics2 = processSignalBuffer(*filter);
    TestQualityMetrics qualMetrics2 = evaluateQuality();
    
    printPerformanceResults(perfMetrics2, "Por buffer (processBuffer)");
    Serial.println("\n");
    printQualityResults(qualMetrics2);
    
    // ========================================================================
    // COMPARACIÓN Y RESUMEN FINAL
    // ========================================================================
    Serial.println("\n");
    printHeader("COMPARACIÓN DE MÉTODOS");
    
    Serial.println("\n--- Rendimiento ---");
    Serial.print("  Muestra por muestra: ");
    Serial.print(perfMetrics1.processingTimeMicros);
    Serial.print(" µs (");
    Serial.print((float)perfMetrics1.sampleRate / SAMPLE_RATE, 1);
    Serial.println("x velocidad)");
    
    Serial.print("  Por buffer: ");
    Serial.print(perfMetrics2.processingTimeMicros);
    Serial.print(" µs (");
    Serial.print((float)perfMetrics2.sampleRate / SAMPLE_RATE, 1);
    Serial.println("x velocidad)");
    
    float speedup = (float)perfMetrics1.processingTimeMicros / perfMetrics2.processingTimeMicros;
    Serial.print("  Mejora de velocidad (buffer vs muestra): ");
    Serial.print(speedup, 2);
    Serial.println("x");
    
    // Resumen final
    Serial.println("\n");
    printHeader("RESUMEN FINAL");
    Serial.println("\nLa librería BioFilterLib (FIRFilter con CMSIS-DSP) ha completado todas las pruebas.");
    Serial.println("\nVeredicto:");
    
    bool performanceOK = (perfMetrics1.sampleRate >= SAMPLE_RATE) && 
                        (perfMetrics1.cpuUsagePercent < 100);
    bool qualityOK = (qualMetrics1.snrImprovement > 2) && 
                     (qualMetrics1.correlation > 0.7);
    
    if (performanceOK && qualityOK) {
        Serial.println("  >> APTO para procesamiento en tiempo real");
        Serial.println("  >> Rendimiento y calidad satisfactorios");
    } else if (performanceOK) {
        Serial.println("  >> Rendimiento adecuado");
        Serial.println("  >> Calidad de filtrado mejorable");
    } else if (qualityOK) {
        Serial.println("  >> Buena calidad de filtrado");
        Serial.println("  >> Rendimiento insuficiente para tiempo real");
    } else {
        Serial.println("  >> Requiere optimización");
    }
    
    Serial.println("\n--- Recomendaciones ---");
    if (speedup > 1.5) {
        Serial.println("  >> El método por buffer es significativamente más rápido");
        Serial.println("  >> Se recomienda usar processBuffer() para aplicaciones críticas");
    } else {
        Serial.println("  >> Ambos métodos tienen rendimiento similar");
        Serial.println("  >> Usar processSample() para tiempo real, processBuffer() para lotes");
    }
    
    // Liberar memoria
    delete filter;
    
    printSeparator();
    Serial.println("\nTest completado. Sistema en espera.");
}

void loop() {
    // Nada que hacer en loop
    delay(1000);
}