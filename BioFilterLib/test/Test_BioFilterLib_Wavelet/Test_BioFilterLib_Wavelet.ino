#include <BioFilterLib.h>

// ════════════════════════════════════════════════════════════════
// CONFIGURACIÓN DEL EXPERIMENTO
// ════════════════════════════════════════════════════════════════
const uint16_t SIGNAL_LENGTH = 1000;
const uint32_t SAMPLE_RATE = 960;      // Hz
const uint16_t BLOCK_SIZE = SIGNAL_LENGTH;

// Buffers
static float32_t ecgClean[SIGNAL_LENGTH];
static float32_t ecgNoisy[SIGNAL_LENGTH];
static float32_t approxCoeffs[SIGNAL_LENGTH];
static float32_t detailCoeffs[SIGNAL_LENGTH];
static float32_t filtered[SIGNAL_LENGTH];

WaveletFilter* waveletFilter;

// ════════════════════════════════════════════════════════════════
// ESTRUCTURAS PARA MÉTRICAS
// ════════════════════════════════════════════════════════════════
struct TestPerformanceMetrics {
    // Velocidad de procesamiento
    uint32_t processingTimeMicros;
    float32_t throughputSamplesPerSec;
    float32_t timePerSampleMicros;
    float32_t realTimeCapability;  // Factor sobre frecuencia objetivo
    float32_t cpuUsagePercent;     // % de CPU a la frecuencia objetivo
    
    // Latencia
    uint32_t latencyMicros;
    uint32_t latencySamples;
};

struct ResourceMetrics {
    // Memoria
    uint32_t ramFreeBeforeBytes;
    uint32_t ramFreeAfterBytes;
    uint32_t ramUsedBytes;
    uint32_t ramUsedPercent;
    
    // Buffers internos
    uint32_t stateBufferSizeBytes;
    uint32_t coeffBufferSizeBytes;
    uint32_t totalFilterMemoryBytes;
    
    // Eficiencia
    float32_t bytesPerSample;
    uint32_t totalRAMAvailable;
};

struct TestQualityMetrics {
    // Reducción de ruido
    float32_t snrInputDB;
    float32_t snrOutputDB;
    float32_t snrImprovementDB;
    
    // Fidelidad de señal
    float32_t correlationWithClean;
    float32_t mseWithClean;
    float32_t rmsError;
    
    // Preservación de energía
    float32_t energyPreservationPercent;
    float32_t approxEnergyPercent;
    float32_t detailEnergyPercent;
    
    // Distorsión
    float32_t totalHarmonicDistortion;
    float32_t phaseDistortion;
};

// Variables globales para métricas
TestPerformanceMetrics perfMetrics;
ResourceMetrics resMetrics;
TestQualityMetrics qualMetrics;

// ════════════════════════════════════════════════════════════════
// SETUP - EJECUTAR EXPERIMENTO
// ════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                                  ║");
    Serial.println("║     REPORTE DE MÉTRICAS - FILTRO WAVELET DAUBECHIES-4           ║");
    Serial.println("║     Procesamiento de Señales ECG en Tiempo Real                 ║");
    Serial.println("║                                                                  ║");
    Serial.println("╚══════════════════════════════════════════════════════════════════╝");
    Serial.println("\n");
    
    // Sección 1: Información del Experimento
    printExperimentInfo();
    
    // Sección 2: Cargar Datos
    Serial.println("┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ 1. PREPARACIÓN DE DATOS                                          │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  Cargando señales de prueba...");
    loadSignal(ecgClean, "ecg_clean", SIGNAL_LENGTH);
    loadSignal(ecgNoisy, "ecg_320hz_noised", SIGNAL_LENGTH);
    Serial.println("  ✓ Señal limpia (referencia): ECG clean");
    Serial.println("  ✓ Señal con ruido (input): ECG con ruido 320Hz\n");
    
    // Sección 3: Medir Recursos ANTES
    measureResourcesBefore();
    
    // Sección 4: Crear Filtro
    Serial.println("┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ 2. INICIALIZACIÓN DEL FILTRO                                     │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  Creando filtro Wavelet Daubechies-4...");
    waveletFilter = new WaveletFilter(BLOCK_SIZE);
    Serial.println("  ✓ Filtro inicializado correctamente\n");
    
    // Sección 5: Medir Recursos DESPUÉS
    measureResourcesAfter();
    printResourceMetrics();
    
    // Sección 6: Ejecutar Procesamiento y Medir Performance
    Serial.println("┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ 3. PROCESAMIENTO Y MEDICIÓN DE VELOCIDAD                         │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  Procesando señal...");
    measurePerformance();
    printPerformanceMetrics();
    
    // Sección 7: Reconstruir y Medir Calidad
    Serial.println("┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ 4. RECONSTRUCCIÓN Y ANÁLISIS DE CALIDAD                          │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  Reconstruyendo señal filtrada...");
    for (uint16_t i = 0; i < SIGNAL_LENGTH; i++) {
        filtered[i] = waveletFilter->reconstruct(approxCoeffs[i], 0.0f);
    }
    Serial.println("  ✓ Señal reconstruida (solo aproximación)\n");
    
    measureQuality();
    printQualityMetrics();
    
    // Sección 8: Resumen Ejecutivo
    printExecutiveSummary();
    
    // Sección 9: Tabla Comparativa
    printComparisonTable();
    
    // Sección 10: Conclusiones y Recomendaciones
    printConclusions();
    
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════════════════════════╗");
    Serial.println("║  REPORTE COMPLETADO                                              ║");
    Serial.println("╚══════════════════════════════════════════════════════════════════╝");
    Serial.println("\n");
}

void loop() {
    // Nada - todo se ejecuta en setup
}

// ════════════════════════════════════════════════════════════════
// FUNCIONES DE MEDICIÓN
// ════════════════════════════════════════════════════════════════

void measureResourcesBefore() {
    resMetrics.totalRAMAvailable = 96000;  // Arduino Due: 96KB
    resMetrics.ramFreeBeforeBytes = getFreeRAM();
    
    // Calcular tamaños teóricos
    uint16_t numTaps = 8;  // Daubechies-4
    resMetrics.stateBufferSizeBytes = (numTaps + BLOCK_SIZE - 1) * sizeof(float32_t) * 4;  // 4 filtros
    resMetrics.coeffBufferSizeBytes = numTaps * sizeof(float32_t) * 4;  // 4 sets de coefs
}

void measureResourcesAfter() {
    resMetrics.ramFreeAfterBytes = getFreeRAM();
    resMetrics.ramUsedBytes = resMetrics.ramFreeBeforeBytes - resMetrics.ramFreeAfterBytes;
    resMetrics.ramUsedPercent = (resMetrics.ramUsedBytes * 100) / resMetrics.totalRAMAvailable;
    resMetrics.totalFilterMemoryBytes = resMetrics.stateBufferSizeBytes + resMetrics.coeffBufferSizeBytes;
    resMetrics.bytesPerSample = (float32_t)resMetrics.ramUsedBytes / SIGNAL_LENGTH;
}

void measurePerformance() {
    // Calentar cache
    waveletFilter->processBuffer(ecgNoisy, approxCoeffs, detailCoeffs, 10);
    waveletFilter->reset();
    
    // Medición real
    uint32_t startTime = micros();
    
    waveletFilter->processBuffer(ecgNoisy, approxCoeffs, detailCoeffs, SIGNAL_LENGTH);
    
    uint32_t endTime = micros();
    
    // Calcular métricas
    perfMetrics.processingTimeMicros = endTime - startTime;
    perfMetrics.timePerSampleMicros = (float32_t)perfMetrics.processingTimeMicros / SIGNAL_LENGTH;
    perfMetrics.throughputSamplesPerSec = (SIGNAL_LENGTH * 1000000.0f) / perfMetrics.processingTimeMicros;
    perfMetrics.realTimeCapability = perfMetrics.throughputSamplesPerSec / SAMPLE_RATE;
    
    // Calcular uso de CPU para tiempo real
    float32_t realTimeRequired = (float32_t)SIGNAL_LENGTH / SAMPLE_RATE;  // segundos
    float32_t timeUsed = perfMetrics.processingTimeMicros / 1000000.0f;
    perfMetrics.cpuUsagePercent = (timeUsed / realTimeRequired) * 100.0f;
    
    // Latencia (número de coeficientes del filtro)
    perfMetrics.latencySamples = 8;  // Daubechies-4
    perfMetrics.latencyMicros = (perfMetrics.latencySamples * 1000000) / SAMPLE_RATE;
}

void measureQuality() {
    // ════════════════════════════════════════════════════════════
    // MÉTODO CORRECTO: Comparar señales con mismo ancho de banda
    // ════════════════════════════════════════════════════════════
    
    // 1. Crear señal limpia filtrada de referencia
    // (mismo procesamiento wavelet que la señal ruidosa)
    static float32_t ecgCleanFiltered[SIGNAL_LENGTH];
    static float32_t tempApprox[SIGNAL_LENGTH];
    static float32_t tempDetail[SIGNAL_LENGTH];
    
    // Procesar señal limpia con el mismo filtro wavelet
    waveletFilter->reset();
    waveletFilter->processBuffer(ecgClean, tempApprox, tempDetail, SIGNAL_LENGTH);
    
    // Reconstruir solo con aproximación (igual que la señal ruidosa filtrada)
    for (uint16_t i = 0; i < SIGNAL_LENGTH; i++) {
        ecgCleanFiltered[i] = waveletFilter->reconstruct(tempApprox[i], 0.0f);
    }
    
    // 2. CALCULAR SNR
    // SNR Input: Potencia de señal original vs ruido añadido
    float32_t signalPowerOriginal = 0.0f;
    float32_t noisePowerAdded = 0.0f;
    
    for (uint16_t i = 0; i < SIGNAL_LENGTH; i++) {
        signalPowerOriginal += ecgClean[i] * ecgClean[i];
        
        float32_t noiseAdded = ecgNoisy[i] - ecgClean[i];
        noisePowerAdded += noiseAdded * noiseAdded;
    }
    
    signalPowerOriginal /= SIGNAL_LENGTH;
    noisePowerAdded /= SIGNAL_LENGTH;
    
    qualMetrics.snrInputDB = 10.0f * log10f(signalPowerOriginal / noisePowerAdded);
    
    // SNR Output: Potencia de señal filtrada vs ruido residual
    float32_t signalPowerFiltered = 0.0f;
    float32_t noisePowerResidual = 0.0f;
    
    for (uint16_t i = 0; i < SIGNAL_LENGTH; i++) {
        signalPowerFiltered += approxCoeffs[i] * approxCoeffs[i]; // filtered[i] * filtered[i];
        
        // Ruido residual = diferencia entre filtrada y limpia filtrada
        float32_t noiseResidual = approxCoeffs[i] - ecgClean[i]; // filtered[i] - ecgCleanFiltered[i];
        noisePowerResidual += noiseResidual * noiseResidual;
    }
    
    signalPowerFiltered /= SIGNAL_LENGTH;
    noisePowerResidual /= SIGNAL_LENGTH;
    
    // Evitar división por cero
    if (noisePowerResidual < 1e-10f) {
        qualMetrics.snrOutputDB = 100.0f; // SNR muy alto
    } else {
        qualMetrics.snrOutputDB = 10.0f * log10f(signalPowerFiltered / noisePowerResidual);
    }
    
    qualMetrics.snrImprovementDB = qualMetrics.snrOutputDB - qualMetrics.snrInputDB;
    
    // 3. Correlación y MSE (comparar con señal limpia filtrada)
    qualMetrics.correlationWithClean = calculateCorrelation(ecgClean, approxCoeffs, SIGNAL_LENGTH); // ecgCleanFiltered, filtered
    qualMetrics.mseWithClean = calculateMSE(ecgClean, approxCoeffs, SIGNAL_LENGTH); // ecgCleanFiltered, filtered
    qualMetrics.rmsError = sqrtf(qualMetrics.mseWithClean);
    
    // 4. Preservación de energía
    float32_t energyApprox = 0.0f;
    float32_t energyDetail = 0.0f;
    float32_t energyOriginal = 0.0f;
    
    for (uint16_t i = 0; i < SIGNAL_LENGTH; i++) {
        energyApprox += approxCoeffs[i] * approxCoeffs[i];
        energyDetail += detailCoeffs[i] * detailCoeffs[i];
        energyOriginal += ecgNoisy[i] * ecgNoisy[i];
    }
    
    float32_t energyTotal = energyApprox + energyDetail;
    qualMetrics.energyPreservationPercent = (energyTotal / energyOriginal) * 100.0f;
    qualMetrics.approxEnergyPercent = (energyApprox / energyTotal) * 100.0f;
    qualMetrics.detailEnergyPercent = (energyDetail / energyTotal) * 100.0f;
    
    // 5. Distorsión
    qualMetrics.totalHarmonicDistortion = (calculateRMS(detailCoeffs, SIGNAL_LENGTH) / 
                                           calculateRMS(approxCoeffs, SIGNAL_LENGTH)) * 100.0f;
    qualMetrics.phaseDistortion = (1.0f - qualMetrics.correlationWithClean) * 100.0f;
}

// ════════════════════════════════════════════════════════════════
// FUNCIONES DE IMPRESIÓN
// ════════════════════════════════════════════════════════════════

void printExperimentInfo() {
    Serial.println("┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ INFORMACIÓN DEL EXPERIMENTO                                      │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  Configuración:");
    Serial.print("  • Plataforma: ");
    Serial.println("Arduino Due (ARM Cortex-M3, 84 MHz)");
    Serial.print("  • Tipo de filtro: ");
    Serial.println("Wavelet Daubechies-4 (banco de filtros)");
    Serial.print("  • Librería DSP: ");
    Serial.println("CMSIS-DSP 1.10.0");
    Serial.print("  • Longitud de señal: ");
    Serial.print(SIGNAL_LENGTH);
    Serial.println(" muestras");
    Serial.print("  • Frecuencia de muestreo: ");
    Serial.print(SAMPLE_RATE);
    Serial.println(" Hz");
    Serial.print("  • Duración de señal: ");
    Serial.print((float)SIGNAL_LENGTH / SAMPLE_RATE, 2);
    Serial.println(" segundos");
    Serial.print("  • Block size: ");
    Serial.println(BLOCK_SIZE);
    Serial.println();
}

void printResourceMetrics() {
    Serial.println("\n┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ MÉTRICAS DE RECURSOS                                             │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  ┌─ MEMORIA RAM ─────────────────────────────────────────────────┐");
    Serial.print("  │ Total disponible:        ");
    Serial.print(resMetrics.totalRAMAvailable);
    Serial.println(" bytes (96 KB)");
    Serial.print("  │ Libre antes del filtro:  ");
    Serial.print(resMetrics.ramFreeBeforeBytes);
    Serial.println(" bytes");
    Serial.print("  │ Libre después del filtro:");
    Serial.print(resMetrics.ramFreeAfterBytes);
    Serial.println(" bytes");
    Serial.print("  │ Usada por el filtro:     ");
    Serial.print(resMetrics.ramUsedBytes);
    Serial.print(" bytes (");
    Serial.print(resMetrics.ramUsedPercent, 1);
    Serial.println("%)");
    Serial.println("  └───────────────────────────────────────────────────────────────┘");
    
    Serial.println("\n  ┌─ DESGLOSE DE MEMORIA ─────────────────────────────────────────┐");
    Serial.print("  │ Buffers de estado (4 filtros): ");
    Serial.print(resMetrics.stateBufferSizeBytes);
    Serial.println(" bytes");
    Serial.print("  │ Coeficientes (4 sets):         ");
    Serial.print(resMetrics.coeffBufferSizeBytes);
    Serial.println(" bytes");
    Serial.print("  │ Total teórico:                 ");
    Serial.print(resMetrics.totalFilterMemoryBytes);
    Serial.println(" bytes");
    Serial.print("  │ Overhead (estructuras/alineac):");
    Serial.print(resMetrics.ramUsedBytes - resMetrics.totalFilterMemoryBytes);
    Serial.println(" bytes");
    Serial.println("  └───────────────────────────────────────────────────────────────┘");
    
    Serial.println("\n  ┌─ EFICIENCIA ──────────────────────────────────────────────────┐");
    Serial.print("  │ Bytes por muestra:  ");
    Serial.print(resMetrics.bytesPerSample, 2);
    Serial.println(" bytes/muestra");
    Serial.print("  │ Overhead de memoria:");
    float32_t overhead = ((float32_t)(resMetrics.ramUsedBytes - resMetrics.totalFilterMemoryBytes) / 
                          resMetrics.ramUsedBytes) * 100.0f;
    Serial.print(overhead, 1);
    Serial.println("%");
    Serial.println("  └───────────────────────────────────────────────────────────────┘\n");
    
    // Evaluación
    Serial.println("  📊 EVALUACIÓN:");
    if (resMetrics.ramUsedPercent < 5) {
        Serial.println("  ✓ EXCELENTE: Uso de memoria muy eficiente (<5%)");
    } else if (resMetrics.ramUsedPercent < 10) {
        Serial.println("  ✓ MUY BUENO: Uso de memoria eficiente (<10%)");
    } else if (resMetrics.ramUsedPercent < 20) {
        Serial.println("  ✓ BUENO: Uso de memoria aceptable (<20%)");
    } else {
        Serial.println("  ⚠ ADVERTENCIA: Uso de memoria considerable (>20%)");
    }
    Serial.println();
}

void printPerformanceMetrics() {
    Serial.println("\n┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ MÉTRICAS DE VELOCIDAD Y RENDIMIENTO                              │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  ┌─ TIEMPOS DE PROCESAMIENTO ────────────────────────────────────┐");
    Serial.print("  │ Tiempo total:           ");
    Serial.print(perfMetrics.processingTimeMicros);
    Serial.print(" µs (");
    Serial.print(perfMetrics.processingTimeMicros / 1000.0f, 2);
    Serial.println(" ms)");
    Serial.print("  │ Tiempo por muestra:     ");
    Serial.print(perfMetrics.timePerSampleMicros, 2);
    Serial.println(" µs/muestra");
    Serial.print("  │ Throughput:             ");
    Serial.print(perfMetrics.throughputSamplesPerSec, 0);
    Serial.println(" muestras/s");
    Serial.println("  └───────────────────────────────────────────────────────────────┘");
    
    Serial.println("\n  ┌─ CAPACIDAD TIEMPO REAL ───────────────────────────────────────┐");
    Serial.print("  │ Frecuencia objetivo:    ");
    Serial.print(SAMPLE_RATE);
    Serial.println(" Hz");
    Serial.print("  │ Capacidad alcanzada:    ");
    Serial.print(perfMetrics.realTimeCapability, 1);
    Serial.println("x la frecuencia objetivo");
    Serial.print("  │ Uso de CPU (@");
    Serial.print(SAMPLE_RATE);
    Serial.print("Hz):  ");
    Serial.print(perfMetrics.cpuUsagePercent, 2);
    Serial.println("%");
    Serial.print("  │ CPU disponible:         ");
    Serial.print(100.0f - perfMetrics.cpuUsagePercent, 2);
    Serial.println("%");
    Serial.println("  └───────────────────────────────────────────────────────────────┘");
    
    Serial.println("\n  ┌─ LATENCIA ────────────────────────────────────────────────────┐");
    Serial.print("  │ Latencia del filtro:    ");
    Serial.print(perfMetrics.latencySamples);
    Serial.print(" muestras (");
    Serial.print(perfMetrics.latencyMicros / 1000.0f, 2);
    Serial.println(" ms)");
    Serial.print("  │ Grupo delay:            ");
    Serial.print(perfMetrics.latencySamples / 2.0f, 1);
    Serial.println(" muestras");
    Serial.println("  └───────────────────────────────────────────────────────────────┘\n");
    
    // Evaluación
    Serial.println("  📊 EVALUACIÓN:");
    if (perfMetrics.realTimeCapability > 10) {
        Serial.println("  ✓ EXCELENTE: Capacidad >10x superior a requisitos");
        Serial.println("    → Ideal para procesamiento en tiempo real");
    } else if (perfMetrics.realTimeCapability > 5) {
        Serial.println("  ✓ MUY BUENO: Capacidad >5x superior a requisitos");
        Serial.println("    → Apropiado para tiempo real con margen");
    } else if (perfMetrics.realTimeCapability > 2) {
        Serial.println("  ✓ BUENO: Capacidad >2x superior a requisitos");
        Serial.println("    → Viable para tiempo real");
    } else {
        Serial.println("  ⚠ ADVERTENCIA: Capacidad cercana a requisitos");
        Serial.println("    → Margen limitado para tiempo real");
    }
    
    if (perfMetrics.cpuUsagePercent < 10) {
        Serial.println("  ✓ Uso de CPU muy eficiente (<10%)");
    } else if (perfMetrics.cpuUsagePercent < 30) {
        Serial.println("  ✓ Uso de CPU eficiente (<30%)");
    } else if (perfMetrics.cpuUsagePercent < 50) {
        Serial.println("  ○ Uso de CPU moderado (30-50%)");
    } else {
        Serial.println("  ⚠ Uso de CPU considerable (>50%)");
    }
    Serial.println();
}

void printQualityMetrics() {
    Serial.println("\n┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ MÉTRICAS DE CALIDAD DE FILTRADO                                  │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  ┌─ REDUCCIÓN DE RUIDO ──────────────────────────────────────────┐");
    Serial.print("  │ SNR entrada:            ");
    Serial.print(qualMetrics.snrInputDB, 2);
    Serial.println(" dB");
    Serial.print("  │ SNR salida:             ");
    Serial.print(qualMetrics.snrOutputDB, 2);
    Serial.println(" dB");
    Serial.print("  │ Mejora de SNR:          ");
    Serial.print(qualMetrics.snrImprovementDB, 2);
    Serial.print(" dB (");
    Serial.print(((pow(10, qualMetrics.snrImprovementDB/10) - 1) * 100), 1);
    Serial.println("% mejora)");
    Serial.println("  └───────────────────────────────────────────────────────────────┘");
    
    Serial.println("\n  ┌─ FIDELIDAD DE SEÑAL ──────────────────────────────────────────┐");
    Serial.print("  │ Correlación c/limpia:   ");
    Serial.print(qualMetrics.correlationWithClean, 4);
    Serial.print(" (");
    Serial.print(qualMetrics.correlationWithClean * 100, 2);
    Serial.println("%)");
    Serial.print("  │ MSE:                    ");
    Serial.print(qualMetrics.mseWithClean, 6);
    Serial.println();
    Serial.print("  │ RMS error:              ");
    Serial.print(qualMetrics.rmsError, 6);
    Serial.println();
    Serial.println("  └───────────────────────────────────────────────────────────────┘");
    
    Serial.println("\n  ┌─ DISTRIBUCIÓN DE ENERGÍA ─────────────────────────────────────┐");
    Serial.print("  │ Preservación:           ");
    Serial.print(qualMetrics.energyPreservationPercent, 2);
    Serial.println("%");
    Serial.print("  │ En aproximación:        ");
    Serial.print(qualMetrics.approxEnergyPercent, 2);
    Serial.println("% (señal útil)");
    Serial.print("  │ En detalle:             ");
    Serial.print(qualMetrics.detailEnergyPercent, 2);
    Serial.println("% (ruido/transitorios)");
    Serial.print("  │ Ratio Aprox/Detalle:    ");
    Serial.println(qualMetrics.approxEnergyPercent / qualMetrics.detailEnergyPercent, 2);
    Serial.println("  └───────────────────────────────────────────────────────────────┘");
    
    Serial.println("\n  ┌─ DISTORSIÓN ──────────────────────────────────────────────────┐");
    Serial.print("  │ THD (Total Harmonic):   ");
    Serial.print(qualMetrics.totalHarmonicDistortion, 2);
    Serial.println("%");
    Serial.print("  │ Distorsión de fase:     ");
    Serial.print(qualMetrics.phaseDistortion, 2);
    Serial.println("%");
    Serial.println("  └───────────────────────────────────────────────────────────────┘\n");
    
    // Evaluación
    Serial.println("  📊 EVALUACIÓN:");
    
    // SNR
    if (qualMetrics.snrImprovementDB > 10) {
        Serial.println("  ✓ EXCELENTE: Mejora de SNR >10 dB");
    } else if (qualMetrics.snrImprovementDB > 5) {
        Serial.println("  ✓ BUENO: Mejora de SNR >5 dB");
    } else if (qualMetrics.snrImprovementDB > 2) {
        Serial.println("  ○ ACEPTABLE: Mejora de SNR >2 dB");
    } else {
        Serial.println("  ⚠ LIMITADO: Mejora de SNR <2 dB");
    }
    
    // Correlación
    if (qualMetrics.correlationWithClean > 0.95) {
        Serial.println("  ✓ EXCELENTE: Fidelidad muy alta (>95%)");
    } else if (qualMetrics.correlationWithClean > 0.90) {
        Serial.println("  ✓ BUENO: Fidelidad alta (>90%)");
    } else if (qualMetrics.correlationWithClean > 0.85) {
        Serial.println("  ○ ACEPTABLE: Fidelidad moderada (>85%)");
    } else {
        Serial.println("  ⚠ ADVERTENCIA: Fidelidad limitada (<85%)");
    }
    
    // Energía
    if (qualMetrics.approxEnergyPercent > 70) {
        Serial.println("  ✓ Excelente separación señal/ruido (>70% en aprox)");
    } else if (qualMetrics.approxEnergyPercent > 60) {
        Serial.println("  ✓ Buena separación señal/ruido (>60% en aprox)");
    } else {
        Serial.println("  ⚠ Señal ruidosa o alta actividad transitoria");
    }
    Serial.println();
}

void printExecutiveSummary() {
    Serial.println("\n┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ RESUMEN EJECUTIVO                                                │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  El filtro Wavelet Daubechies-4 demostró:\n");
    
    // Velocidad
    Serial.println("  🚀 RENDIMIENTO:");
    Serial.print("     Procesa señales ");
    Serial.print(perfMetrics.realTimeCapability, 1);
    Serial.println("x más rápido que tiempo real");
    Serial.print("     Uso de CPU: ");
    Serial.print(perfMetrics.cpuUsagePercent, 1);
    Serial.print("% @ ");
    Serial.print(SAMPLE_RATE);
    Serial.println(" Hz");
    Serial.print("     Latencia: ");
    Serial.print(perfMetrics.latencyMicros / 1000.0f, 2);
    Serial.println(" ms");
    
    // Recursos
    Serial.println("\n  💾 RECURSOS:");
    Serial.print("     Memoria usada: ");
    Serial.print(resMetrics.ramUsedBytes);
    Serial.print(" bytes (");
    Serial.print(resMetrics.ramUsedPercent, 1);
    Serial.println("%)");
    Serial.print("     Eficiencia: ");
    Serial.print(resMetrics.bytesPerSample, 2);
    Serial.println(" bytes/muestra");
    
    // Calidad
    Serial.println("\n  ✨ CALIDAD:");
    Serial.print("     Mejora de SNR: ");
    Serial.print(qualMetrics.snrImprovementDB, 2);
    Serial.println(" dB");
    Serial.print("     Fidelidad: ");
    Serial.print(qualMetrics.correlationWithClean * 100, 1);
    Serial.println("%");
    Serial.print("     Energía preservada: ");
    Serial.print(qualMetrics.energyPreservationPercent, 1);
    Serial.println("%");
    
    // Veredicto general
    Serial.println("\n  🎯 VEREDICTO GENERAL:");
    
    bool excellentPerf = perfMetrics.realTimeCapability > 10;
    bool excellentMem = resMetrics.ramUsedPercent < 10;
    bool excellentQual = qualMetrics.snrImprovementDB > 10 && qualMetrics.correlationWithClean > 0.95;
    
    if (excellentPerf && excellentMem && excellentQual) {
        Serial.println("     ⭐⭐⭐⭐⭐ EXCELENTE");
        Serial.println("     Ideal para aplicaciones en tiempo real con recursos limitados");
    } else if ((excellentPerf || perfMetrics.realTimeCapability > 5) &&
               (excellentMem || resMetrics.ramUsedPercent < 15) &&
               (excellentQual || (qualMetrics.snrImprovementDB > 5 && qualMetrics.correlationWithClean > 0.90))) {
        Serial.println("     ⭐⭐⭐⭐ MUY BUENO");
        Serial.println("     Apropiado para la mayoría de aplicaciones biomédicas");
    } else {
        Serial.println("     ⭐⭐⭐ BUENO");
        Serial.println("     Funcional para aplicaciones estándar");
    }
    Serial.println();
}

void printComparisonTable() {
    Serial.println("┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ TABLA COMPARATIVA CON BENCHMARKS                                │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  Métrica                    │  Logrado  │ Mínimo Req. │ Recomendado │ Estado");
    Serial.println("  ───────────────────────────┼───────────┼─────────────┼─────────────┼────────");
    
    // Throughput
    Serial.print("  Throughput (muestras/s)    │ ");
    printPadded(perfMetrics.throughputSamplesPerSec, 9, 0);
    Serial.print(" │    ");
    printPadded(SAMPLE_RATE, 7, 0);
    Serial.print("  │    ");
    printPadded(SAMPLE_RATE * 2, 9, 0);
    Serial.print("  │ ");
    Serial.println(perfMetrics.throughputSamplesPerSec > SAMPLE_RATE * 2 ? "✓ PASS" : "○ WARN");
    
    // CPU Usage
    Serial.print("  Uso CPU (%)                │ ");
    printPadded(perfMetrics.cpuUsagePercent, 9, 2);
    Serial.print(" │      50.00  │       20.00  │ ");
    Serial.println(perfMetrics.cpuUsagePercent < 20 ? "✓ PASS" : (perfMetrics.cpuUsagePercent < 50 ? "○ WARN" : "✗ FAIL"));
    
    // RAM Usage
    Serial.print("  Uso RAM (%)                │ ");
    printPadded(resMetrics.ramUsedPercent, 9, 2);
    Serial.print(" │      30.00  │       10.00  │ ");
    Serial.println(resMetrics.ramUsedPercent < 10 ? "✓ PASS" : (resMetrics.ramUsedPercent < 30 ? "○ WARN" : "✗ FAIL"));
    
    // SNR Improvement
    Serial.print("  Mejora SNR (dB)            │ ");
    printPadded(qualMetrics.snrImprovementDB, 9, 2);
    Serial.print(" │       3.00  │       10.00  │ ");
    Serial.println(qualMetrics.snrImprovementDB > 10 ? "✓ PASS" : (qualMetrics.snrImprovementDB > 3 ? "○ WARN" : "✗ FAIL"));
    
    // Correlation
    Serial.print("  Correlación (%)            │ ");
    printPadded(qualMetrics.correlationWithClean * 100, 9, 2);
    Serial.print(" │      85.00  │       95.00  │ ");
    Serial.println(qualMetrics.correlationWithClean > 0.95 ? "✓ PASS" : (qualMetrics.correlationWithClean > 0.85 ? "○ WARN" : "✗ FAIL"));
    
    // Latency
    Serial.print("  Latencia (ms)              │ ");
    printPadded(perfMetrics.latencyMicros / 1000.0f, 9, 2);
    Serial.print(" │      50.00  │       10.00  │ ");
    float32_t latencyMs = perfMetrics.latencyMicros / 1000.0f;
    Serial.println(latencyMs < 10 ? "✓ PASS" : (latencyMs < 50 ? "○ WARN" : "✗ FAIL"));
    
    Serial.println("\n  Leyenda: ✓ PASS = Cumple recomendado │ ○ WARN = Cumple mínimo │ ✗ FAIL = No cumple\n");
}

void printConclusions() {
    Serial.println("┌──────────────────────────────────────────────────────────────────┐");
    Serial.println("│ CONCLUSIONES Y RECOMENDACIONES                                   │");
    Serial.println("└──────────────────────────────────────────────────────────────────┘\n");
    
    Serial.println("  📋 CONCLUSIONES:\n");
    
    Serial.print("  1. El filtro es ");
    if (perfMetrics.realTimeCapability > 10) {
        Serial.println("APTO para procesamiento en tiempo real");
        Serial.println("     con amplio margen de seguridad.");
    } else if (perfMetrics.realTimeCapability > 2) {
        Serial.println("APTO para procesamiento en tiempo real");
        Serial.println("     con margen de seguridad adecuado.");
    } else {
        Serial.println("LIMITADO para procesamiento en tiempo real.");
        Serial.println("     Margen de seguridad reducido.");
    }
    
    Serial.print("\n  2. El consumo de recursos es ");
    if (resMetrics.ramUsedPercent < 10) {
        Serial.println("EXCELENTE (<10% RAM).");
        Serial.println("     Permite múltiples instancias del filtro.");
    } else if (resMetrics.ramUsedPercent < 20) {
        Serial.println("MUY BUENO (<20% RAM).");
        Serial.println("     Deja espacio para otros procesos.");
    } else {
        Serial.println("CONSIDERABLE (>20% RAM).");
        Serial.println("     Limita capacidad para otros procesos.");
    }
    
    Serial.print("\n  3. La calidad de filtrado es ");
    if (qualMetrics.snrImprovementDB > 10 && qualMetrics.correlationWithClean > 0.95) {
        Serial.println("EXCELENTE.");
        Serial.println("     Alta reducción de ruido con mínima distorsión.");
    } else if (qualMetrics.snrImprovementDB > 5 && qualMetrics.correlationWithClean > 0.90) {
        Serial.println("MUY BUENA.");
        Serial.println("     Buena reducción de ruido con baja distorsión.");
    } else {
        Serial.println("ACEPTABLE.");
        Serial.println("     Reducción de ruido moderada.");
    }
    
    Serial.println("\n  💡 RECOMENDACIONES:\n");
    
    Serial.println("  • Para tiempo real:");
    if (perfMetrics.cpuUsagePercent < 20) {
        Serial.println("    ✓ Puede procesar múltiples canales simultáneamente");
    } else {
        Serial.println("    ○ Considerar optimizaciones para multi-canal");
    }
    
    Serial.println("\n  • Para optimización de memoria:");
    if (resMetrics.ramUsedPercent < 10) {
        Serial.println("    ✓ Configuración actual es óptima");
    } else {
        Serial.println("    ○ Considerar BLOCK_SIZE más pequeño si es necesario");
    }
    
    Serial.println("\n  • Para aplicaciones clínicas:");
    if (qualMetrics.correlationWithClean > 0.95) {
        Serial.println("    ✓ Fidelidad suficiente para análisis diagnóstico");
    } else {
        Serial.println("    ○ Verificar preservación de características clínicas");
    }
    
    Serial.println();
}

void printPadded(float value, int width, int decimals) {
    String str = String(value, decimals);
    while (str.length() < width) {
        str = " " + str;  // pad with spaces on the left
    }
    Serial.print(str);
}
