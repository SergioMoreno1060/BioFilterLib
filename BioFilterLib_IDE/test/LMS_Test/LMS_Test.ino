/**
 * @file LMS_Test.ino
 * @brief Pruebas de rendimiento y calidad para LMSFilter usando TestRunner
 * @author Sergio Moreno
 * @version 2.0.0
 * @date 2025
 *
 * @details
 * Realiza pruebas exhaustivas del filtro LMS adaptativo evaluando:
 * 1. Velocidad de procesamiento y convergencia
 * 2. Calidad de adaptación y cancelación de interferencia
 * 3. Rendimiento con diferentes valores de mu (velocidad de adaptación)
 * 
 * Usa señal ECG con ruido 60Hz (waveformsTable[7]) @ 240Hz con 900 samples
 */

#include <Arduino.h>
#include <BioFilterLib.h>

// ========== CONFIGURACIÓN DE PRUEBAS ==========

#define SIGNAL_TAG      "ecg_60hz_noised_fs240"
#define NUM_TAPS_LMS    32
#define MU_FAST         0.05f   // Adaptación rápida
#define MU_MEDIUM       0.02f   // Adaptación balanceada
#define MU_SLOW         0.005f  // Adaptación lenta
#define BLOCK_SIZE      1
#define SAMPLE_RATE     240.0f
#define POWERLINE_FREQ  60.0f
#define SPEED_TEST_ITERATIONS 50

// ========== BUFFERS DE DATOS ==========

float32_t inputSignal[maxSamplesNum];
float32_t referenceSignal[maxSamplesNum];  // Señal de referencia (60Hz)
float32_t filteredSignal[maxSamplesNum];
float32_t errorSignal[maxSamplesNum];

// Coeficientes LMS
float32_t lmsCoeffsFast[NUM_TAPS_LMS];
float32_t lmsCoeffsMedium[NUM_TAPS_LMS];
float32_t lmsCoeffsSlow[NUM_TAPS_LMS];

// Instancias de filtros LMS
LMSFilter* lmsFilterFast;
LMSFilter* lmsFilterMedium;
LMSFilter* lmsFilterSlow;

// TestRunner
TestRunner testRunner;

// ========== FUNCIONES AUXILIARES ==========

/**
 * @brief Genera señal de referencia sinusoidal (60Hz)
 */
void generateReferenceSignal() {
    Serial.println("Generando señal de referencia (60Hz)...");
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        float32_t t = (float32_t)i / SAMPLE_RATE;
        referenceSignal[i] = sin(2.0f * PI * POWERLINE_FREQ * t);
    }
}

/**
 * @brief Inicializa coeficientes LMS a cero
 */
void initializeCoefficients(float32_t* coeffs, uint32_t numTaps) {
    for (uint32_t i = 0; i < numTaps; i++) {
        coeffs[i] = 0.0f;
    }
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
 * @brief Calcula tiempo de convergencia (cuando error < threshold)
 */
uint32_t calculateConvergenceTime(const float32_t* errorSignal, uint32_t length, 
                                   float32_t threshold) {
    float32_t runningAvg = 0.0f;
    const uint32_t avgWindow = 50;
    
    for (uint32_t i = avgWindow; i < length; i++) {
        runningAvg = 0.0f;
        for (uint32_t j = 0; j < avgWindow; j++) {
            runningAvg += abs(errorSignal[i - j]);
        }
        runningAvg /= avgWindow;
        
        if (runningAvg < threshold) {
            return i;
        }
    }
    return length;
}

// ========== FUNCIONES DE TEST ==========

void testSpeedAndConvergence(LMSFilter* filter, float32_t mu, const char* name) {
    Serial.print("\n=== PRUEBA: ");
    Serial.print(name);
    Serial.print(" (mu=");
    Serial.print(mu, 4);
    Serial.println(") ===");
    
    // Prueba de velocidad
    unsigned long totalTime = 0;
    
    for (uint32_t iter = 0; iter < SPEED_TEST_ITERATIONS; iter++) {
        // Reiniciar coeficientes
        filter->reset();
        
        unsigned long startTime = micros();
        
        for (uint32_t i = 0; i < maxSamplesNum; i++) {
            filteredSignal[i] = filter->processSample(inputSignal[i], referenceSignal[i]);
        }
        
        unsigned long endTime = micros();
        totalTime += (endTime - startTime);
    }
    
    float32_t avgTime = (float32_t)totalTime / SPEED_TEST_ITERATIONS;
    float32_t timePerSample = avgTime / maxSamplesNum;
    
    Serial.print("Tiempo promedio: ");
    Serial.print(avgTime, 2);
    Serial.println(" µs");
    Serial.print("Tiempo/muestra: ");
    Serial.print(timePerSample, 3);
    Serial.println(" µs");
    
    // Análisis de convergencia
    filter->reset();
    for (uint32_t i = 0; i < maxSamplesNum; i++) {
        filteredSignal[i] = filter->processSample(inputSignal[i], referenceSignal[i]);
        errorSignal[i] = inputSignal[i] - filteredSignal[i];
    }
    
    uint32_t convergenceTime = calculateConvergenceTime(errorSignal, maxSamplesNum, 0.1f);
    float32_t convergenceTimeSec = convergenceTime / SAMPLE_RATE;
    
    float32_t rmsError = calculateRMS(errorSignal, maxSamplesNum);
    
    Serial.print("Tiempo convergencia: ");
    Serial.print(convergenceTimeSec, 3);
    Serial.print(" s (");
    Serial.print(convergenceTime);
    Serial.println(" muestras)");
    Serial.print("RMS error final: ");
    Serial.println(rmsError, 6);
}

void testAdaptationComparison() {
    Serial.println("\n=== COMPARACIÓN DE VELOCIDADES DE ADAPTACIÓN ===");
    
    // Probar los tres filtros
    testSpeedAndConvergence(lmsFilterFast, MU_FAST, "Adaptación RÁPIDA");
    delay(500);
    
    testSpeedAndConvergence(lmsFilterMedium, MU_MEDIUM, "Adaptación MEDIA");
    delay(500);
    
    testSpeedAndConvergence(lmsFilterSlow, MU_SLOW, "Adaptación LENTA");
    
    Serial.println("\n--- RECOMENDACIONES ---");
    Serial.println("  • mu=0.05: Convergencia rápida, puede ser inestable");
    Serial.println("  • mu=0.02: Balance entre velocidad y estabilidad");
    Serial.println("  • mu=0.005: Convergencia lenta, mayor estabilidad");
}

// ========== SETUP Y LOOP ==========

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("\n========================================");
    Serial.println("  LMS FILTER - PRUEBAS DE RENDIMIENTO  ");
    Serial.println("========================================");
    Serial.println("\nBioFilterLib Test Suite v2.0");
    Serial.println("Plataforma: Arduino Due");
    Serial.println("Filtro: LMS Adaptativo 32 taps");
    Serial.println("Aplicación: Cancelación de interferencia 60Hz");
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
    
    // Generar señal de referencia
    generateReferenceSignal();
    
    // Inicializar coeficientes
    Serial.println("Inicializando coeficientes LMS...");
    initializeCoefficients(lmsCoeffsFast, NUM_TAPS_LMS);
    initializeCoefficients(lmsCoeffsMedium, NUM_TAPS_LMS);
    initializeCoefficients(lmsCoeffsSlow, NUM_TAPS_LMS);
    
    // Crear instancias de filtros
    Serial.println("Creando filtros LMS...");
    lmsFilterFast = new LMSFilter(lmsCoeffsFast, NUM_TAPS_LMS, MU_FAST, BLOCK_SIZE);
    lmsFilterMedium = new LMSFilter(lmsCoeffsMedium, NUM_TAPS_LMS, MU_MEDIUM, BLOCK_SIZE);
    lmsFilterSlow = new LMSFilter(lmsCoeffsSlow, NUM_TAPS_LMS, MU_SLOW, BLOCK_SIZE);
    
    if (!lmsFilterFast || !lmsFilterMedium || !lmsFilterSlow) {
        Serial.println("ERROR: No se pudieron crear los filtros");
        while(1);
    }
    
    Serial.println("Filtros inicializados correctamente.");
    
    // ===== PRUEBAS DE VELOCIDAD Y CONVERGENCIA =====
    delay(1000);
    testAdaptationComparison();
    
    // ===== PRUEBAS DE CALIDAD CON TESTRUNNER =====
    delay(1000);
    Serial.println("\n========================================");
    Serial.println("  PRUEBAS DE CALIDAD (TestRunner)");
    Serial.println("========================================");
    
    // Probar con adaptación media (mejor balance)
    Serial.println("\n--- Prueba individual (mu=0.02) ---");
    testRunner.testLMS(lmsFilterMedium, SIGNAL_TAG, referenceSignal);
    
    // Probar todas las señales con 60Hz
    delay(500);
    testRunner.testAllLMS(lmsFilterMedium, referenceSignal);
    
    Serial.println("\n========================================");
    Serial.println("    PRUEBAS COMPLETADAS CON ÉXITO     ");
    Serial.println("========================================");
    Serial.println("\nSistema listo.");
}

void loop() {
    delay(1000);
}