/**
 * @file FIR_Test_DEBUG.ino
 * @brief Suite de pruebas FIR con diagnóstico integrado
 * @version 1.1.0 - Con diagnóstico de señales
 */

#include <BioFilterLib.h>

// ============================================================================
// CONFIGURACIÓN DE PRUEBAS
// ============================================================================

const uint16_t FILTER_ORDER = 50;
const uint16_t NUM_TAPS = FILTER_ORDER + 1;

float32_t lowpassCoeffs[NUM_TAPS] = {
    +0.00096226f, +0.00110652f, +0.00123488f, +0.00128605f, +0.00115012f, +0.00068979f, -0.00022373f, -0.00166613f,
    -0.00361514f, -0.00591523f, -0.00826092f, -0.01020548f, -0.01119776f, -0.01064514f, -0.00799553f, -0.00282748f,
    +0.00506537f, +0.01560960f, +0.02841897f, +0.04280137f, +0.05780805f, +0.07232126f, +0.08517038f, +0.09526185f,
    +0.10170564f, +0.10392083f, +0.10170564f, +0.09526185f, +0.08517038f, +0.07232126f, +0.05780805f, +0.04280137f,
    +0.02841897f, +0.01560960f, +0.00506537f, -0.00282748f, -0.00799553f, -0.01064514f, -0.01119776f, -0.01020548f,
    -0.00826092f, -0.00591523f, -0.00361514f, -0.00166613f, -0.00022373f, +0.00068979f, +0.00115012f, +0.00128605f,
    +0.00123488f, +0.00110652f, +0.00096226f
};

const uint16_t BLOCK_SIZE_SAMPLE = 1;
const uint16_t BLOCK_SIZE_BUFFER = 32;
const uint16_t TEST_SIGNAL_LENGTH = 1500;

float32_t ecgClean[TEST_SIGNAL_LENGTH];
float32_t ecgNoisy[TEST_SIGNAL_LENGTH];
float32_t ecgFiltered[TEST_SIGNAL_LENGTH];

// ============================================================================
// FUNCIONES DE DIAGNÓSTICO
// ============================================================================

void printSignalStats(const char* name, float32_t* signal, uint32_t length) {
    Serial.print("\n[DEBUG] Estadisticas de: ");
    Serial.println(name);
    
    float32_t minVal = signal[0];
    float32_t maxVal = signal[0];
    float32_t sum = 0.0f;
    uint32_t zeroCount = 0;
    uint32_t nanCount = 0;
    uint32_t infCount = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        if (isnan(signal[i])) {
            nanCount++;
            continue;
        }
        if (isinf(signal[i])) {
            infCount++;
            continue;
        }
        if (signal[i] == 0.0f) zeroCount++;
        
        if (signal[i] < minVal) minVal = signal[i];
        if (signal[i] > maxVal) maxVal = signal[i];
        sum += signal[i];
    }
    
    float32_t mean = sum / (length - nanCount - infCount);
    
    Serial.print("  Rango: [");
    Serial.print(minVal, 6);
    Serial.print(" , ");
    Serial.print(maxVal, 6);
    Serial.println("]");
    Serial.print("  Media: ");
    Serial.println(mean, 6);
    Serial.print("  Ceros: ");
    Serial.print(zeroCount);
    Serial.print(" (");
    Serial.print((zeroCount * 100.0f) / length, 1);
    Serial.println("%)");
    
    if (nanCount > 0) {
        Serial.print("  ⚠️  NaN detectados: ");
        Serial.println(nanCount);
    }
    if (infCount > 0) {
        Serial.print("  ⚠️  Inf detectados: ");
        Serial.println(infCount);
    }
    
    // Mostrar primeras 5 muestras
    Serial.println("  Primeras 5 muestras:");
    for (uint32_t i = 0; i < 5 && i < length; i++) {
        Serial.print("    [");
        Serial.print(i);
        Serial.print("]: ");
        Serial.println(signal[i], 6);
    }
    
    // Evaluación
    if (zeroCount == length) {
        Serial.println("  ❌ ERROR: Señal completamente CERO");
    } else if (nanCount > 0 || infCount > 0) {
        Serial.println("  ❌ ERROR: Valores inválidos (NaN/Inf)");
    } else if (minVal == maxVal) {
        Serial.println("  ⚠️  ADVERTENCIA: Señal constante (sin variación)");
    } else {
        Serial.println("  ✅ Señal válida");
    }
}

void verifyFilterCoefficients() {
    Serial.println("\n[*] Verificando coeficientes del filtro...");
    
    float32_t sumCoeffs = 0.0f;
    float32_t minCoeff = lowpassCoeffs[0];
    float32_t maxCoeff = lowpassCoeffs[0];
    
    for (uint16_t i = 0; i < NUM_TAPS; i++) {
        sumCoeffs += lowpassCoeffs[i];
        if (lowpassCoeffs[i] < minCoeff) minCoeff = lowpassCoeffs[i];
        if (lowpassCoeffs[i] > maxCoeff) maxCoeff = lowpassCoeffs[i];
    }
    
    Serial.print("  Suma de coeficientes: ");
    Serial.println(sumCoeffs, 6);
    Serial.print("  Rango: [");
    Serial.print(minCoeff, 6);
    Serial.print(" , ");
    Serial.print(maxCoeff, 6);
    Serial.println("]");
    
    if (fabs(sumCoeffs - 1.0f) < 0.1f) {
        Serial.println("  ✅ Coeficientes normalizados correctamente");
    } else if (fabs(sumCoeffs) < 0.01f) {
        Serial.println("  ⚠️  ADVERTENCIA: Suma muy cercana a cero");
    } else {
        Serial.print("  ℹ️  Suma es ");
        Serial.print(sumCoeffs, 3);
        Serial.println(" (normal para algunos filtros)");
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial);
    delay(2000);
    
    printSeparator();
    Serial.println("  BIOFILTERLIB - SUITE DE PRUEBAS FIR (DEBUG MODE)");
    Serial.println("  Arduino Due - ARM Cortex-M3 @ 84MHz");
    Serial.println("  Frecuencia de muestreo: 960 Hz");
    printSeparator();
    
    delay(1000);
    
    // ========================================================================
    // DIAGNÓSTICO PREVIO
    // ========================================================================
    
    Serial.println("\n========== DIAGNOSTICO INICIAL ==========");
    
    // Verificar coeficientes
    verifyFilterCoefficients();
    
    Serial.println("\n=========================================\n");
    delay(1000);
    
    // ========================================================================
    // CARGAR SEÑALES CON VERIFICACIÓN
    // ========================================================================
    
    Serial.println("\n[*] Cargando señales de prueba...");
    Serial.println("    Intentando cargar con etiquetas textuales...");
    
    bool cleanLoaded = loadSignal(ecgClean, "ecg_clean", TEST_SIGNAL_LENGTH);
    bool noisyLoaded = loadSignal(ecgNoisy, "ecg_60hz_noised", TEST_SIGNAL_LENGTH);
    
    Serial.print("    loadSignal ECG limpia: ");
    Serial.println(cleanLoaded ? "true" : "false");
    Serial.print("    loadSignal ECG ruidosa: ");
    Serial.println(noisyLoaded ? "true" : "false");
    
    // Si falló, intentar con índices numéricos
    if (!cleanLoaded || !noisyLoaded) {
        Serial.println("\n⚠️  Carga con etiquetas falló, intentando con índices...");
        cleanLoaded = loadSignal(ecgClean, "0", TEST_SIGNAL_LENGTH);
        noisyLoaded = loadSignal(ecgNoisy, "1", TEST_SIGNAL_LENGTH);
        
        Serial.print("    loadSignal con '0': ");
        Serial.println(cleanLoaded ? "true" : "false");
        Serial.print("    loadSignal con '1': ");
        Serial.println(noisyLoaded ? "true" : "false");
    }
    
    if (!cleanLoaded || !noisyLoaded) {
        Serial.println("\n❌ ERROR CRÍTICO: No se pudieron cargar las señales");
        Serial.println("Verifica que Waveforms.h tiene las señales correctas");
        while(1);
    }
    
    Serial.println("[OK] Señales cargadas");
    
    // ========================================================================
    // VERIFICAR SEÑALES CARGADAS
    // ========================================================================
    
    Serial.println("\n========== VERIFICACION DE SEÑALES CARGADAS ==========");
    printSignalStats("ECG Limpia", ecgClean, TEST_SIGNAL_LENGTH);
    printSignalStats("ECG con Ruido 60Hz", ecgNoisy, TEST_SIGNAL_LENGTH);
    Serial.println("======================================================\n");
    
    delay(2000);
    
    // Verificar que las señales no son todas cero
    bool cleanIsZero = true;
    bool noisyIsZero = true;
    for (uint32_t i = 0; i < TEST_SIGNAL_LENGTH; i++) {
        if (ecgClean[i] != 0.0f) cleanIsZero = false;
        if (ecgNoisy[i] != 0.0f) noisyIsZero = false;
    }
    
    if (cleanIsZero || noisyIsZero) {
        Serial.println("\n❌ ERROR: Una o ambas señales son completamente CERO");
        Serial.println("El problema está en loadSignal() o en Waveforms.h");
        while(1);
    }
    
    // ========================================================================
    // TEST 1: MEMORIA
    // ========================================================================
    
    printTestHeader("TEST 1: ANALISIS DE CONSUMO DE MEMORIA");
    Serial.println("\nConfiguracion del filtro:");
    Serial.print("  - Orden: ");
    Serial.println(FILTER_ORDER);
    Serial.print("  - Coeficientes: ");
    Serial.println(NUM_TAPS);
    Serial.println("  - Tipo: Pasa-bajas Hamming (fc=50Hz, fs=960Hz)");
    
    testMemoryUsage(NUM_TAPS, BLOCK_SIZE_SAMPLE);
    delay(2000);
    
    // ========================================================================
    // TEST 2: RENDIMIENTO SAMPLE
    // ========================================================================
    
    printTestHeader("TEST 2: RENDIMIENTO - SAMPLE BY SAMPLE");
    Serial.println("\n[*] Creando filtro...");
    FIRFilter filterSample(lowpassCoeffs, NUM_TAPS, BLOCK_SIZE_SAMPLE);
    Serial.println("[OK] Filtro creado");
    
    Serial.println("\n[*] Ejecutando test...");
    PerformanceMetrics perfSample = testFilterSpeed_Sample(filterSample, 
                                                           ecgNoisy, 
                                                           TEST_SIGNAL_LENGTH);
    printPerformanceMetrics(perfSample);
    delay(2000);
    
    // ========================================================================
    // TEST 3: RENDIMIENTO BUFFER
    // ========================================================================
    
    printTestHeader("TEST 3: RENDIMIENTO - BUFFER MODE");
    Serial.println("\n[*] Creando filtro...");
    FIRFilter filterBuffer(lowpassCoeffs, NUM_TAPS, BLOCK_SIZE_BUFFER);
    Serial.println("[OK] Filtro creado");
    
    Serial.println("\n[*] Ejecutando test...");
    PerformanceMetrics perfBuffer = testFilterSpeed_Buffer(filterBuffer,
                                                           ecgNoisy,
                                                           ecgFiltered,
                                                           TEST_SIGNAL_LENGTH);
    printPerformanceMetrics(perfBuffer);
    
    float32_t speedup = (float32_t)perfBuffer.sampleRate / (float32_t)perfSample.sampleRate;
    Serial.println("\n--- COMPARACION ---");
    Serial.print("  Aceleracion: ");
    Serial.print(speedup, 2);
    Serial.println("x");
    
    delay(2000);
    
    // ========================================================================
    // TEST 4: CALIDAD CON DIAGNÓSTICO
    // ========================================================================
    
    printTestHeader("TEST 4: CALIDAD DE FILTRADO (CON DIAGNOSTICO)");
    Serial.println("\nObjetivo:");
    Serial.println("  - Eliminar ruido 60Hz");
    Serial.println("  - Preservar QRS");
    
    Serial.println("\n========== SEÑALES ANTES DE FILTRAR ==========");
    printSignalStats("ECG Limpia (referencia)", ecgClean, TEST_SIGNAL_LENGTH);
    printSignalStats("ECG Ruidosa (entrada)", ecgNoisy, TEST_SIGNAL_LENGTH);
    Serial.println("==============================================\n");
    
    Serial.println("[*] Aplicando filtro...");
    FIRFilter qualityFilter(lowpassCoeffs, NUM_TAPS, BLOCK_SIZE_BUFFER);
    qualityFilter.processBuffer(ecgNoisy, ecgFiltered, TEST_SIGNAL_LENGTH);
    Serial.println("[OK] Filtrado completado");
    
    Serial.println("\n========== SEÑAL FILTRADA ==========");
    printSignalStats("ECG Filtrada (salida)", ecgFiltered, TEST_SIGNAL_LENGTH);
    Serial.println("====================================\n");
    
    Serial.println("[*] Calculando métricas...");
    QualityMetrics quality = testFilterQuality(ecgClean, 
                                               ecgNoisy, 
                                               ecgFiltered, 
                                               TEST_SIGNAL_LENGTH);
    
    printQualityMetrics(quality);
    
    delay(2000);
    
    // ========================================================================
    // RESUMEN
    // ========================================================================
    
    printTestHeader("RESUMEN");
    Serial.println("\n=== RENDIMIENTO ===");
    Serial.print("Sample mode: ");
    Serial.print(perfSample.sampleRate);
    Serial.println(" muestras/s");
    Serial.print("Buffer mode: ");
    Serial.print(perfBuffer.sampleRate);
    Serial.println(" muestras/s");
    
    Serial.println("\n=== CALIDAD ===");
    Serial.print("SNR: ");
    Serial.print(quality.snr, 2);
    Serial.println(" dB");
    Serial.print("Correlacion: ");
    Serial.println(quality.correlation, 4);
    
    printSeparator();
    Serial.println("\n[*] Pruebas completadas\n");
}

void loop() {
    delay(1000);
}
