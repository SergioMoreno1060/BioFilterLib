/**
 * @file LMS_Example_Demo.ino
 * @brief Ejemplo completo de filtro LMS para cancelación adaptativa de interferencia
 * @author Basado en BioFilterLib
 * @date 2025
 * 
 * @details Este ejemplo demuestra:
 * 1. Cancelación de interferencia de 60Hz en una señal ECG simulada
 * 2. Adaptación automática de coeficientes
 * 3. Visualización en tiempo real con Serial Plotter
 * 
 * CONFIGURACIÓN DEL SERIAL PLOTTER:
 * - Abrir Tools -> Serial Plotter en Arduino IDE
 * - Configurar baudrate a 115200
 * - Verás 4 señales:
 *   * Señal_Contaminada (rojo): ECG + interferencia 60Hz
 *   * Interferencia_60Hz (verde): Señal de referencia
 *   * Señal_Limpia (azul): Salida del filtro LMS
 *   * Error_Adaptacion (amarillo): Qué tan bien se adapta el filtro
 */

#include <BioFilterLib.h>

// ============================================================================
// CONFIGURACIÓN DE LA SIMULACIÓN
// ============================================================================

// Parámetros de señal
const uint32_t SAMPLE_RATE = 1000;        // Frecuencia de muestreo: 1000 Hz
const float SAMPLING_PERIOD = 1.0f / SAMPLE_RATE; // Período: 1 ms

// Parámetros del filtro LMS
const uint16_t NUM_TAPS = 64;             // Número de coeficientes
const float MU = 0.01f;                   // Paso de adaptación (0.05 = convergencia rápida)
const uint16_t BLOCK_SIZE = 1;            // Procesamiento muestra por muestra

// Parámetros de interferencia
const float POWERLINE_FREQ = 60.0f;       // Frecuencia de línea eléctrica: 60 Hz
const float INTERFERENCE_AMP = 0.3f;      // Amplitud de la interferencia

// Parámetros de la señal ECG simulada
const float ECG_HEART_RATE = 75.0f;       // Frecuencia cardíaca: 75 BPM (1.25 Hz)

// Variables globales
float32_t lmsCoeffs[NUM_TAPS];            // Coeficientes del filtro LMS
LMSFilter* adaptiveFilter;                // Puntero al filtro LMS
float timeCounter = 0.0f;                 // Contador de tiempo para simulación

// ============================================================================
// FUNCIONES DE GENERACIÓN DE SEÑALES
// ============================================================================

/**
 * @brief Genera una muestra de señal ECG sintética simplificada
 * @param t Tiempo en segundos
 * @return Amplitud de la señal ECG normalizada
 */
float32_t generateECG(float t) {
    // Frecuencia del latido cardíaco en Hz
    float heartRateHz = ECG_HEART_RATE / 60.0f;
    
    // Fase del ciclo cardíaco (0 a 2π)
    float phase = 2.0f * PI * heartRateHz * t;
    phase = fmod(phase, 2.0f * PI);  // Mantener en rango [0, 2π]
    
    // Generar forma de onda ECG simplificada
    // Usando una combinación de sinusoidales para simular complejo QRS
    float ecg = 0.0f;
    
    // Componente P (onda pequeña antes del QRS)
    if (phase > 0.3f && phase < 0.8f) {
        ecg += 0.15f * sin((phase - 0.3f) * 2.0f * PI / 0.5f);
    }
    
    // Complejo QRS (pico principal del ECG)
    if (phase > 1.0f && phase < 1.6f) {
        // Onda Q (pequeña deflexión negativa)
        if (phase < 1.2f) {
            ecg -= 0.1f * sin((phase - 1.0f) * 2.0f * PI / 0.2f);
        }
        // Onda R (pico grande positivo)
        else if (phase < 1.4f) {
            ecg += 1.0f * sin((phase - 1.2f) * 2.0f * PI / 0.2f);
        }
        // Onda S (deflexión negativa)
        else {
            ecg -= 0.2f * sin((phase - 1.4f) * 2.0f * PI / 0.2f);
        }
    }
    
    // Onda T (repolarización ventricular)
    if (phase > 2.0f && phase < 2.8f) {
        ecg += 0.25f * sin((phase - 2.0f) * 2.0f * PI / 0.8f);
    }
    
    return ecg;
}

/**
 * @brief Genera interferencia de línea eléctrica (60 Hz)
 * @param t Tiempo en segundos
 * @return Amplitud de la interferencia
 */
float32_t generatePowerlineInterference(float t) {
    return INTERFERENCE_AMP * sin(2.0f * PI * POWERLINE_FREQ * t);
}

/**
 * @brief Genera señal de referencia para el LMS (60 Hz puro)
 * @param t Tiempo en segundos
 * @return Amplitud de la referencia
 */
float32_t generateReference(float t) {
    // La referencia debe estar correlacionada con la interferencia
    // pero NO con la señal ECG limpia
    return sin(2.0f * PI * POWERLINE_FREQ * t);
}

// ============================================================================
// SETUP: INICIALIZACIÓN
// ============================================================================

void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    while (!Serial) { delay(10); }  // Esperar a que se abra el puerto serial
    
    delay(2000);  // Esperar 2 segundos antes de comenzar
    
    Serial.println("========================================");
    Serial.println("  DEMO: Filtro LMS Adaptativo");
    Serial.println("  Cancelación de Interferencia 60Hz");
    Serial.println("========================================");
    Serial.println();
    
    // Inicializar coeficientes a cero (punto de partida para adaptación)
    for (uint16_t i = 0; i < NUM_TAPS; i++) {
        lmsCoeffs[i] = 0.0f;
    }
    
    // Crear instancia del filtro LMS
    adaptiveFilter = new LMSFilter(lmsCoeffs, NUM_TAPS, MU, BLOCK_SIZE);
    
    Serial.println("Configuración del filtro:");
    Serial.print("  - Número de coeficientes: ");
    Serial.println(NUM_TAPS);
    Serial.print("  - Paso de adaptación (mu): ");
    Serial.println(MU, 4);
    Serial.print("  - Frecuencia de muestreo: ");
    Serial.print(SAMPLE_RATE);
    Serial.println(" Hz");
    Serial.println();
    
    Serial.println("Señales generadas:");
    Serial.println("  - ECG simulado: 75 BPM");
    Serial.println("  - Interferencia: 60 Hz");
    Serial.println("  - Referencia: 60 Hz puro");
    Serial.println();
    
    Serial.println("INICIANDO SIMULACIÓN...");
    Serial.println("Abre el Serial Plotter para visualizar!");
    Serial.println();
    
    delay(1000);
    
    // Imprimir encabezados para Serial Plotter
    Serial.println("Señal_Contaminada,Interferencia_60Hz,Señal_Limpia,Error_Adaptacion");
}

// ============================================================================
// LOOP: PROCESAMIENTO EN TIEMPO REAL
// ============================================================================

void loop() {
    // Generar señal ECG limpia
    float32_t cleanECG = generateECG(timeCounter);
    
    // Generar interferencia de 60Hz
    float32_t interference = generatePowerlineInterference(timeCounter);
    
    // Señal contaminada = ECG limpio + Interferencia
    float32_t contaminatedSignal = cleanECG + interference;
    
    // Generar señal de referencia (correlacionada con la interferencia)
    float32_t referenceSignal = generateReference(timeCounter);
    
    // Variables para salida del filtro
    float32_t filteredOutput = 0.0f;
    float32_t adaptationError = 0.0f;
    
    // ========================================================================
    // PROCESAMIENTO CON FILTRO LMS ADAPTATIVO
    // ========================================================================
    
    // El filtro LMS intenta:
    // 1. Hacer que 'filteredOutput' coincida con 'referenceSignal'
    // 2. La entrada 'contaminatedSignal' se filtra para producir 'filteredOutput'
    // 3. Los coeficientes se ajustan para minimizar 'adaptationError'
    //
    // En cancelación de interferencia:
    // - input: señal contaminada (ECG + 60Hz)
    // - reference: queremos extraer el ECG limpio
    // - output: estimación de la interferencia
    // - Señal limpia = input - output
    
    adaptiveFilter->processSample(referenceSignal,
                                   contaminatedSignal, 
                                   &filteredOutput, 
                                   &adaptationError);
    
    // La salida del LMS es una estimación de la interferencia
    // Restamos esta estimación de la señal DE REFERENCIA DE RUIDO para obtener ECG limpio
    float32_t cleanedSignal = contaminatedSignal - filteredOutput;
    
    // ========================================================================
    // VISUALIZACIÓN EN SERIAL PLOTTER
    // ========================================================================
    
    // Formato: valor1,valor2,valor3,valor4
    // Serial Plotter mostrará cada valor como una línea separada
    
    Serial.print(contaminatedSignal, 4);   // Señal contaminada (ECG + 60Hz)
    Serial.print(",");
    Serial.print(interference, 4);         // Interferencia real de 60Hz
    Serial.print(",");
    Serial.print(cleanedSignal, 4);        // Señal limpia (salida del LMS)
    Serial.print(",");
    Serial.println(adaptationError, 4);    // Error de adaptación
    
    // ========================================================================
    // ACTUALIZAR TIEMPO Y CONTROL DE VELOCIDAD
    // ========================================================================
    
    // Incrementar tiempo
    timeCounter += SAMPLING_PERIOD;
    
    // Resetear tiempo cada 10 segundos para evitar overflow
    if (timeCounter > 20.0f) {
        timeCounter = 0.0f;
    }
    
    // Control de velocidad de simulación
    // Para 1000 Hz real necesitamos delay de 1ms
    // Ajusta este valor según tu visualización:
    // - delay(1): Tiempo real (rápido, difícil de ver en plotter)
    // - delay(10): 10x más lento (mejor visualización)
    // - delay(50): 50x más lento (muy fácil de ver)
    
    delay(10);  // Ralentizar para mejor visualización en Serial Plotter
}

// ============================================================================
// FUNCIONES ADICIONALES (OPCIONAL - PARA ANÁLISIS DETALLADO)
// ============================================================================

/**
 * @brief Imprime estadísticas del filtro cada N muestras
 * @note Descomenta la llamada en loop() si quieres ver estadísticas
 */
void printFilterStatistics() {
    static uint32_t sampleCount = 0;
    static const uint32_t STATS_INTERVAL = 1000;  // Cada 1000 muestras
    
    sampleCount++;
    
    if (sampleCount >= STATS_INTERVAL) {
        Serial.println();
        Serial.println("=== ESTADÍSTICAS DEL FILTRO ===");
        Serial.print("Mu actual: ");
        Serial.println(adaptiveFilter->getMu(), 4);
        
        // Mostrar algunos coeficientes adaptativos
        Serial.println("Primeros 5 coeficientes:");
        for (int i = 0; i < 5; i++) {
            Serial.print("  w[");
            Serial.print(i);
            Serial.print("] = ");
            Serial.println(lmsCoeffs[i], 6);
        }
        Serial.println();
        
        sampleCount = 0;
    }
}
