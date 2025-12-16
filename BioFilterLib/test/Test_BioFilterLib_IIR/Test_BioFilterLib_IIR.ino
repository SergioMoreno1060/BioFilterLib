/**
 * @file Test_BioFilterLib_IIR.cpp
 * @brief Test de rendimiento y calidad para filtros IIR de BioFilterLib
 * @author Test Suite
 * @date 2025
 * 
 * Prueba filtro IIR Notch 60Hz con señales ECG del archivo Waveforms.h
 * Calcula metricas de rendimiento y calidad de señal
 * 
 * Hardware recomendado: Arduino Due (ARM Cortex-M3 @ 84 MHz)
 * Frecuencia de muestreo: 960 Hz (señales ECG)
 * 
 * Filtro: IIR Notch de segundo orden (Biquad)
 * - Frecuencia central: 60 Hz
 * - Factor de calidad Q: 30 (notch estrecho)
 * - Orden: 2 (1 seccion Biquad)
 * 
 * IMPORTANTE: Usa los mismos coeficientes y numero de muestras que
 * Test_ttapa_IIR.ino para permitir comparacion directa de resultados.
 */
#include <BioFilterLib.h>

// ============================================================================
// CONFIGURACION DEL TEST
// ============================================================================

constexpr float FS = 960.0f;              // Frecuencia de muestreo (Hz)
constexpr float NOTCH_FREQ = 60.0f;       // Frecuencia a filtrar (Hz)
constexpr float Q_FACTOR = 30.0f;         // Factor de calidad del notch
constexpr uint16_t NUM_SAMPLES = 2000;    // Muestras a procesar (igual que Test_ttapa_IIR.ino)

// ============================================================================
// DISEÑO DEL FILTRO IIR NOTCH 60Hz
// ============================================================================
// Coeficientes del filtro Notch (f0=60Hz, Q=30, fs=960Hz)
// Diseñado con scipy.signal.iirnotch
// Los MISMOS coeficientes usados en Test_ttapa_IIR.ino
// Formato CMSIS Biquad: {b0, b1, b2, a1, a2} (a0=1 se omite)

const float32_t notch_coeffs[5] = {
    0.99349748f, -1.83574398f,  0.99349748f,  // b0, b1, b2
    1.83574398f, -0.98699496f                  // -a1, -a2 (NEGADOS para CMSIS-DSP)
};

// Crear instancia del filtro IIR de BioFilterLib
// - 1 stage: filtro de orden 2 (una seccion Biquad)
// - blockSize 1: procesamiento muestra por muestra (tiempo real)
IIRFilter iir_filter((float32_t*)notch_coeffs, 1, 1);

// ============================================================================
// BUFFERS DE DATOS
// ============================================================================

float32_t signal_clean[NUM_SAMPLES];     // Señal ECG limpia (referencia)
float32_t signal_noisy[NUM_SAMPLES];     // Señal ECG con ruido 60Hz
float32_t signal_filtered[NUM_SAMPLES];  // Señal filtrada (salida)

// ============================================================================
// ESTRUCTURAS PARA METRICAS ADICIONALES
// ============================================================================
// Nota: Las estructuras base estan en utils_extended.h
// Aqui extendemos con campos adicionales necesarios para este test

struct TestPerformanceMetrics {
    uint32_t total_time_us;       // Tiempo total de procesamiento (us)
    float time_per_sample_us;     // Tiempo por muestra (us)
    float throughput_sps;         // Muestras procesadas por segundo
    float cpu_usage_percent;      // Uso de CPU al procesar a FS Hz
    uint32_t ram_before;          // RAM libre antes del test
    uint32_t ram_after;           // RAM libre despues del test
    int32_t ram_used;             // RAM utilizada por buffers
};

struct TestQualityMetrics {
    float snr_input_db;           // SNR de entrada (señal con ruido)
    float snr_output_db;          // SNR de salida (señal filtrada)
    float snr_improvement_db;     // Mejora de SNR
    float mse;                    // Error cuadratico medio vs señal limpia
    float correlation;            // Correlacion con señal limpia
    float rms_clean;              // RMS señal limpia
    float rms_noisy;              // RMS señal con ruido
    float rms_filtered;           // RMS señal filtrada
};

TestPerformanceMetrics perf_metrics;
TestQualityMetrics qual_metrics;

// ============================================================================
// FUNCION PRINCIPAL DE TEST
// ============================================================================

void runFilterTest() {
    Serial.print("\n");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    Serial.println("  INICIANDO TEST DE FILTRO IIR NOTCH 60Hz - BioFilterLib");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    
    // ------------------------------------------------------------------------
    // 1. CARGAR SEÑALES
    // ------------------------------------------------------------------------
    Serial.println("1. Cargando señales desde Waveforms.h...");
    
    if (!loadSignal(signal_clean, "ecg_clean", NUM_SAMPLES)) {
        Serial.println("ERROR: No se pudo cargar señal limpia");
        return;
    }
    
    if (!loadSignal(signal_noisy, "ecg_60hz_noised", NUM_SAMPLES)) {
        Serial.println("ERROR: No se pudo cargar señal con ruido");
        return;
    }
    
    Serial.println("   >> Señal limpia cargada (" + String(NUM_SAMPLES) + " muestras)");
    Serial.println("   >> Señal con ruido 60Hz cargada");
    Serial.println();
    
    // ------------------------------------------------------------------------
    // 2. MEDICION DE MEMORIA ANTES DEL TEST
    // ------------------------------------------------------------------------
    perf_metrics.ram_before = getFreeRAM();
    
    // ------------------------------------------------------------------------
    // 3. APLICAR FILTRO Y MEDIR RENDIMIENTO
    // ------------------------------------------------------------------------
    Serial.println("2. Aplicando filtro IIR Notch 60Hz...");
    Serial.println("   Configuracion del filtro:");
    Serial.println("   - Tipo: IIR Biquad Notch (CMSIS-DSP)");
    Serial.println("   - Orden: 2 (1 seccion Biquad)");
    Serial.println("   - Frecuencia central: " + String(NOTCH_FREQ, 1) + " Hz");
    Serial.println("   - Factor de calidad Q: " + String(Q_FACTOR, 1));
    Serial.println("   - Frecuencia de muestreo: " + String(FS, 1) + " Hz");
    Serial.println("   - Libreria: BioFilterLib (CMSIS-DSP optimizado)");
    Serial.println();
    
    // Procesar señal muestra por muestra y medir tiempo
    uint32_t start_time = micros();
    
    for (uint16_t i = 0; i < NUM_SAMPLES; i++) {
        signal_filtered[i] = iir_filter.processSample(signal_noisy[i]);
    }
    
    uint32_t end_time = micros();
    
    // Calcular metricas de rendimiento
    perf_metrics.total_time_us = end_time - start_time;
    perf_metrics.time_per_sample_us = (float)perf_metrics.total_time_us / NUM_SAMPLES;
    perf_metrics.throughput_sps = 1000000.0f / perf_metrics.time_per_sample_us;
    perf_metrics.cpu_usage_percent = (perf_metrics.time_per_sample_us * FS) / 10000.0f;
    
    Serial.println("   >> Filtrado completado");
    Serial.println();
    
    // ------------------------------------------------------------------------
    // 4. MEDICION DE MEMORIA DESPUES DEL TEST
    // ------------------------------------------------------------------------
    perf_metrics.ram_after = getFreeRAM();
    perf_metrics.ram_used = perf_metrics.ram_before - perf_metrics.ram_after;
    
    // ------------------------------------------------------------------------
    // 5. CALCULAR METRICAS DE CALIDAD
    // ------------------------------------------------------------------------
    Serial.println("3. Calculando metricas de calidad...");
    
    // Calcular RMS usando las funciones de utils.cpp
    qual_metrics.rms_clean = calculateRMS(signal_clean, NUM_SAMPLES);
    qual_metrics.rms_noisy = calculateRMS(signal_noisy, NUM_SAMPLES);
    qual_metrics.rms_filtered = calculateRMS(signal_filtered, NUM_SAMPLES);
    
    // Calcular SNR usando las funciones de utils.cpp
    // Los filtros IIR tienen delay de grupo variable, pero para un Biquad Notch
    // tipicamente es muy pequeño (0-1 muestras)
    const uint16_t delay_samples = 0;
    
    qual_metrics.snr_input_db = calculateSNR(signal_clean, signal_noisy, NUM_SAMPLES);
    qual_metrics.snr_output_db = calculateSNR(signal_clean, signal_filtered + delay_samples, 
                                               NUM_SAMPLES - delay_samples);
    qual_metrics.snr_improvement_db = qual_metrics.snr_output_db - qual_metrics.snr_input_db;
    
    // Calcular MSE y correlacion usando las funciones de utils.cpp
    qual_metrics.mse = calculateMSE(signal_clean, signal_filtered + delay_samples, 
                                     NUM_SAMPLES - delay_samples);
    qual_metrics.correlation = calculateCorrelation(signal_clean, signal_filtered + delay_samples, 
                                                     NUM_SAMPLES - delay_samples);
    
    Serial.println("   >> Metricas calculadas");
    Serial.println();
}

// ============================================================================
// FUNCION PARA MOSTRAR RESULTADOS
// ============================================================================

void printResults() {
    Serial.print("\n");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    Serial.println("  RESULTADOS DE RENDIMIENTO");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    
    Serial.println("--- Tiempos de Procesamiento ---");
    Serial.println("  Tiempo total: " + String(perf_metrics.total_time_us) + " us");
    Serial.println("  Tiempo por muestra: " + String(perf_metrics.time_per_sample_us, 2) + " us/muestra");
    Serial.println();
    
    Serial.println("--- Throughput ---");
    Serial.println("  Tasa alcanzada: " + String((int)perf_metrics.throughput_sps) + " muestras/s");
    Serial.println("  Tasa objetivo: " + String((int)FS) + " Hz");
    Serial.println("  Factor de velocidad: " + String(perf_metrics.throughput_sps / FS, 1) + "x");
    Serial.println();
    
    Serial.println("--- Uso de CPU ---");
    Serial.println("  CPU (@ " + String((int)FS) + " Hz): " + String(perf_metrics.cpu_usage_percent, 2) + " %");
    
    if (perf_metrics.cpu_usage_percent < 10) {
        Serial.println("  >> Uso de CPU muy eficiente");
    } else if (perf_metrics.cpu_usage_percent < 50) {
        Serial.println("  >> Uso de CPU aceptable");
    } else if (perf_metrics.cpu_usage_percent < 100) {
        Serial.println("  >> ADVERTENCIA: Alto uso de CPU");
    } else {
        Serial.println("  >> CRITICO: CPU sobrecargada, no apto para tiempo real");
    }
    Serial.println();
    
    Serial.println("--- Memoria RAM ---");
    Serial.println("  RAM libre antes: " + String(perf_metrics.ram_before) + " bytes");
    Serial.println("  RAM libre despues: " + String(perf_metrics.ram_after) + " bytes");
    Serial.println("  RAM utilizada: " + String(perf_metrics.ram_used) + " bytes");
    
    // Calcular memoria teorica del filtro IIR CMSIS
    // Para IIR Biquad con 1 stage: 4 * numStages floats para estados
    uint32_t theoretical_memory = 
        5 * sizeof(float32_t) +                    // Coeficientes (b0,b1,b2,a1,a2)
        4 * 1 * sizeof(float32_t) +                // Estados (4 por stage)
        sizeof(arm_biquad_casd_df1_inst_f32);      // Estructura CMSIS
    
    Serial.println("  Memoria teorica del filtro: ~" + String(theoretical_memory) + " bytes");
    Serial.println("    - Coeficientes: " + String(5 * sizeof(float32_t)) + " bytes");
    Serial.println("    - Buffer de estados: " + String(4 * sizeof(float32_t)) + " bytes");
    Serial.println("    - Estructura CMSIS: " + String(sizeof(arm_biquad_casd_df1_inst_f32)) + " bytes");
    Serial.println();
    
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    Serial.println("  RESULTADOS DE CALIDAD");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    
    Serial.println("--- SNR (Relacion Señal/Ruido) ---");
    Serial.println("  SNR entrada (con ruido): " + String(qual_metrics.snr_input_db, 2) + " dB");
    Serial.println("  SNR salida (filtrada): " + String(qual_metrics.snr_output_db, 2) + " dB");
    Serial.println("  Mejora de SNR: " + String(qual_metrics.snr_improvement_db, 2) + " dB");
    Serial.println();
    
    Serial.println("--- Error y Correlacion ---");
    Serial.println("  MSE (vs señal limpia): " + String(qual_metrics.mse, 6));
    Serial.println("  Correlacion (vs señal limpia): " + String(qual_metrics.correlation, 4));
    Serial.println();
    
    Serial.println("--- Valores RMS ---");
    Serial.println("  RMS señal limpia: " + String(qual_metrics.rms_clean, 4));
    Serial.println("  RMS señal con ruido: " + String(qual_metrics.rms_noisy, 4));
    Serial.println("  RMS señal filtrada: " + String(qual_metrics.rms_filtered, 4));
    Serial.println();
    
    // ------------------------------------------------------------------------
    // EVALUACION DE RESULTADOS
    // ------------------------------------------------------------------------
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    Serial.println("  EVALUACION");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    
    // Evaluacion de rendimiento
    Serial.println("--- RENDIMIENTO ---");
    if (perf_metrics.throughput_sps >= FS * 10) {
        Serial.println("  >> EXCELENTE: Throughput muy superior al requerido");
        Serial.println("    (" + String((int)(perf_metrics.throughput_sps / FS)) + "x la frecuencia de muestreo)");
    } else if (perf_metrics.throughput_sps >= FS * 5) {
        Serial.println("  >> BUENO: Throughput adecuado con margen de seguridad");
        Serial.println("    (" + String((int)(perf_metrics.throughput_sps / FS)) + "x la frecuencia de muestreo)");
    } else if (perf_metrics.throughput_sps >= FS) {
        Serial.println("  >> ACEPTABLE: Throughput suficiente pero justo");
        Serial.println("    (" + String(perf_metrics.throughput_sps / FS, 1) + "x la frecuencia de muestreo)");
    } else {
        Serial.println("  >> INSUFICIENTE: Throughput inadecuado para tiempo real");
        Serial.println("    El filtro NO puede procesar en tiempo real a " + String((int)FS) + " Hz");
    }
    Serial.println();
    
    // Evaluacion de calidad
    Serial.println("--- CALIDAD DE FILTRADO ---");
    if (qual_metrics.snr_improvement_db > 10.0f) {
        Serial.println("  >> EXCELENTE: Mejora significativa de SNR");
        Serial.println("    (+" + String(qual_metrics.snr_improvement_db, 1) + " dB)");
    } else if (qual_metrics.snr_improvement_db > 5.0f) {
        Serial.println("  >> BUENO: Mejora notable de SNR");
        Serial.println("    (+" + String(qual_metrics.snr_improvement_db, 1) + " dB)");
    } else if (qual_metrics.snr_improvement_db > 0.0f) {
        Serial.println("  >> ACEPTABLE: Mejora moderada de SNR");
        Serial.println("    (+" + String(qual_metrics.snr_improvement_db, 1) + " dB)");
    } else {
        Serial.println("  >> DEFICIENTE: No hay mejora de SNR o empeora");
        Serial.println("    (" + String(qual_metrics.snr_improvement_db, 1) + " dB)");
    }
    Serial.println();
    
    Serial.println("--- FIDELIDAD DE LA SEÑAL ---");
    if (qual_metrics.correlation > 0.95f) {
        Serial.println("  >> EXCELENTE: Alta correlacion con señal original");
        Serial.println("    (r = " + String(qual_metrics.correlation, 4) + ")");
    } else if (qual_metrics.correlation > 0.85f) {
        Serial.println("  >> BUENA: Buena correlacion con señal original");
        Serial.println("    (r = " + String(qual_metrics.correlation, 4) + ")");
    } else if (qual_metrics.correlation > 0.70f) {
        Serial.println("  >> MODERADA: Correlacion aceptable");
        Serial.println("    (r = " + String(qual_metrics.correlation, 4) + ")");
    } else {
        Serial.println("  >> BAJA: Señal significativamente alterada");
        Serial.println("    (r = " + String(qual_metrics.correlation, 4) + ")");
    }
    Serial.println();
    
    // Evaluacion de memoria
    Serial.println("--- USO DE MEMORIA ---");
    if (perf_metrics.ram_used < 1000) {
        Serial.println("  >> EXCELENTE: Uso de memoria muy eficiente");
    } else if (perf_metrics.ram_used < 5000) {
        Serial.println("  >> BUENO: Uso de memoria aceptable");
    } else {
        Serial.println("  >> ALTO: Considerar optimizacion de memoria");
    }
    Serial.println();
    
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
}

// ============================================================================
// FUNCION PARA EXPORTAR DATOS (OPCIONAL)
// ============================================================================

void exportData() {
    Serial.println("Desea exportar las señales para analisis externo? (s/n)");
    
    // Esperar respuesta del usuario con timeout
    unsigned long timeout = millis() + 5000;
    while (!Serial.available() && millis() < timeout) {
        delay(10);
    }
    
    if (Serial.available()) {
        char response = Serial.read();
        while (Serial.available()) Serial.read(); // Limpiar buffer
        
        if (response == 's' || response == 'S') {
            Serial.println("\n--- DATOS EXPORTADOS (formato CSV) ---");
            Serial.println("sample,clean,noisy,filtered");
            
            // Exportar primeras 100 muestras para no saturar el Serial
            for (uint16_t i = 0; i < min(NUM_SAMPLES, (uint16_t)100); i++) {
                Serial.print(i);
                Serial.print(",");
                Serial.print(signal_clean[i], 6);
                Serial.print(",");
                Serial.print(signal_noisy[i], 6);
                Serial.print(",");
                Serial.println(signal_filtered[i], 6);
            }
            
            Serial.println("--- FIN DE DATOS (primeras 100 muestras) ---");
            Serial.println();
        }
    } else {
        Serial.println("Timeout - no se exportaron datos");
        Serial.println();
    }
}

// ============================================================================
// TEST ADICIONAL: PROCESAMIENTO POR BUFFER
// ============================================================================

void testBufferProcessing() {
    Serial.print("\n");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    Serial.println("  TEST ADICIONAL: PROCESAMIENTO POR BUFFER");
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
    Serial.println("Comparando procesamiento muestra-por-muestra vs. buffer completo\n");
    
    // Reiniciar filtro para test limpio
    IIRFilter iir_filter_buffer((float32_t*)notch_coeffs, 1, 1);
    
    float32_t buffer_output[NUM_SAMPLES];
    
    // ------------------------------------------------------------------------
    // MEDIR TIEMPO DE PROCESAMIENTO POR BUFFER
    // ------------------------------------------------------------------------
    uint32_t start_buffer = micros();
    iir_filter_buffer.processBuffer(signal_noisy, buffer_output, NUM_SAMPLES);
    uint32_t end_buffer = micros();
    
    uint32_t buffer_time = end_buffer - start_buffer;
    float buffer_time_per_sample = (float)buffer_time / NUM_SAMPLES;
    float buffer_throughput = 1000000.0f / buffer_time_per_sample;
    float buffer_cpu_usage = (buffer_time_per_sample * FS) / 10000.0f;
    
    // ------------------------------------------------------------------------
    // CALCULAR METRICAS DE CALIDAD PARA PROCESAMIENTO POR BUFFER
    // ------------------------------------------------------------------------
    TestQualityMetrics buffer_qual_metrics;
    
    const uint16_t delay_samples = 0;
    
    // RMS
    buffer_qual_metrics.rms_clean = qual_metrics.rms_clean;  // Misma señal limpia
    buffer_qual_metrics.rms_noisy = qual_metrics.rms_noisy;  // Misma señal con ruido
    buffer_qual_metrics.rms_filtered = calculateRMS(buffer_output, NUM_SAMPLES);
    
    // SNR
    buffer_qual_metrics.snr_input_db = qual_metrics.snr_input_db;  // Misma entrada
    buffer_qual_metrics.snr_output_db = calculateSNR(signal_clean, buffer_output + delay_samples, 
                                                       NUM_SAMPLES - delay_samples);
    buffer_qual_metrics.snr_improvement_db = buffer_qual_metrics.snr_output_db - buffer_qual_metrics.snr_input_db;
    
    // MSE y Correlacion
    buffer_qual_metrics.mse = calculateMSE(signal_clean, buffer_output + delay_samples, 
                                            NUM_SAMPLES - delay_samples);
    buffer_qual_metrics.correlation = calculateCorrelation(signal_clean, buffer_output + delay_samples, 
                                                            NUM_SAMPLES - delay_samples);
    
    // ------------------------------------------------------------------------
    // MOSTRAR COMPARACION DE RENDIMIENTO
    // ------------------------------------------------------------------------
    Serial.println("--- COMPARACION DE RENDIMIENTO ---");
    Serial.println();
    Serial.println("Sample-by-Sample:");
    Serial.println("  Tiempo total: " + String(perf_metrics.total_time_us) + " us");
    Serial.println("  Tiempo por muestra: " + String(perf_metrics.time_per_sample_us, 2) + " us/muestra");
    Serial.println("  Throughput: " + String((int)perf_metrics.throughput_sps) + " muestras/s");
    Serial.println("  CPU (@ " + String((int)FS) + " Hz): " + String(perf_metrics.cpu_usage_percent, 2) + " %");
    Serial.println();
    
    Serial.println("Procesamiento por Buffer:");
    Serial.println("  Tiempo total: " + String(buffer_time) + " us");
    Serial.println("  Tiempo por muestra: " + String(buffer_time_per_sample, 2) + " us/muestra");
    Serial.println("  Throughput: " + String((int)buffer_throughput) + " muestras/s");
    Serial.println("  CPU (@ " + String((int)FS) + " Hz): " + String(buffer_cpu_usage, 2) + " %");
    Serial.println();
    
    float speedup = perf_metrics.time_per_sample_us / buffer_time_per_sample;
    Serial.println("Mejora de velocidad: " + String(speedup, 2) + "x");
    
    if (speedup > 1.5) {
        Serial.println("  >> Procesamiento por buffer significativamente mas rapido");
    } else if (speedup > 1.1) {
        Serial.println("  >> Procesamiento por buffer ligeramente mas rapido");
    } else if (speedup > 0.9) {
        Serial.println("  >> Ambos metodos tienen rendimiento similar");
    } else {
        Serial.println("  >> Sample-by-sample es mas rapido (inusual)");
    }
    Serial.println();
    
    // ------------------------------------------------------------------------
    // MOSTRAR COMPARACION DE CALIDAD
    // ------------------------------------------------------------------------
    Serial.println("--- COMPARACION DE CALIDAD ---");
    Serial.println();
    
    Serial.println("SNR (Relacion Señal/Ruido):");
    Serial.println("                          Sample-by-Sample    Buffer");
    Serial.println("  SNR salida:             " + String(qual_metrics.snr_output_db, 2) + " dB          " + 
                   String(buffer_qual_metrics.snr_output_db, 2) + " dB");
    Serial.println("  Mejora SNR:             " + String(qual_metrics.snr_improvement_db, 2) + " dB          " + 
                   String(buffer_qual_metrics.snr_improvement_db, 2) + " dB");
    
    float snr_diff = abs(qual_metrics.snr_output_db - buffer_qual_metrics.snr_output_db);
    Serial.println("  Diferencia SNR:         " + String(snr_diff, 4) + " dB");
    Serial.println();
    
    Serial.println("Error y Correlacion:");
    Serial.println("                          Sample-by-Sample    Buffer");
    Serial.println("  MSE:                    " + String(qual_metrics.mse, 6) + "       " + 
                   String(buffer_qual_metrics.mse, 6));
    Serial.println("  Correlacion:            " + String(qual_metrics.correlation, 6) + "       " + 
                   String(buffer_qual_metrics.correlation, 6));
    
    float mse_diff = abs(qual_metrics.mse - buffer_qual_metrics.mse);
    float corr_diff = abs(qual_metrics.correlation - buffer_qual_metrics.correlation);
    Serial.println("  Diferencia MSE:         " + String(mse_diff, 8));
    Serial.println("  Diferencia Correlacion: " + String(corr_diff, 8));
    Serial.println();
    
    Serial.println("Valores RMS:");
    Serial.println("                          Sample-by-Sample    Buffer");
    Serial.println("  RMS filtrada:           " + String(qual_metrics.rms_filtered, 4) + "           " + 
                   String(buffer_qual_metrics.rms_filtered, 4));
    
    float rms_diff = abs(qual_metrics.rms_filtered - buffer_qual_metrics.rms_filtered);
    Serial.println("  Diferencia RMS:         " + String(rms_diff, 6));
    Serial.println();
    
    // ------------------------------------------------------------------------
    // EVALUACION DE EQUIVALENCIA
    // ------------------------------------------------------------------------
    Serial.println("--- EVALUACION DE EQUIVALENCIA ---");
    
    // Tolerancias para considerar resultados equivalentes
    const float SNR_TOLERANCE = 0.1;      // 0.1 dB
    const float MSE_TOLERANCE = 1e-6;      // Muy pequeño
    const float CORR_TOLERANCE = 1e-4;     // 4 decimales
    const float RMS_TOLERANCE = 1e-4;      // 4 decimales
    
    bool snr_equivalent = (snr_diff < SNR_TOLERANCE);
    bool mse_equivalent = (mse_diff < MSE_TOLERANCE);
    bool corr_equivalent = (corr_diff < CORR_TOLERANCE);
    bool rms_equivalent = (rms_diff < RMS_TOLERANCE);
    
    Serial.println("Equivalencia de resultados:");
    Serial.println("  SNR:          " + String(snr_equivalent ? ">> EQUIVALENTE" : ">> DIFERENTE") + 
                   " (diff < " + String(SNR_TOLERANCE, 2) + " dB)");
    Serial.println("  MSE:          " + String(mse_equivalent ? ">> EQUIVALENTE" : ">> DIFERENTE") + 
                   " (diff < " + String(MSE_TOLERANCE, 8) + ")");
    Serial.println("  Correlacion:  " + String(corr_equivalent ? ">> EQUIVALENTE" : ">> DIFERENTE") + 
                   " (diff < " + String(CORR_TOLERANCE, 6) + ")");
    Serial.println("  RMS:          " + String(rms_equivalent ? ">> EQUIVALENTE" : ">> DIFERENTE") + 
                   " (diff < " + String(RMS_TOLERANCE, 6) + ")");
    Serial.println();
    
    if (snr_equivalent && mse_equivalent && corr_equivalent && rms_equivalent) {
        Serial.println("  >> EXCELENTE: Ambos metodos producen resultados identicos");
        Serial.println("     El procesamiento por buffer es matematicamente equivalente");
    } else if (snr_diff < 1.0 && mse_diff < 1e-4 && corr_diff < 1e-3) {
        Serial.println("  >> BUENO: Ambos metodos producen resultados muy similares");
        Serial.println("    Las pequeñas diferencias son aceptables (errores numericos)");
    } else {
        Serial.println("  >> ADVERTENCIA: Diferencias significativas entre metodos");
        Serial.println("    Revisar implementacion del filtro");
    }
    Serial.println();
    
    // ------------------------------------------------------------------------
    // RECOMENDACION
    // ------------------------------------------------------------------------
    Serial.println("--- RECOMENDACION ---");
    
    if (speedup > 1.2 && snr_equivalent && mse_equivalent) {
        Serial.println("  ** RECOMENDADO: Usar procesamiento por BUFFER");
        Serial.println("     - " + String(speedup, 1) + "x mas rapido");
        Serial.println("     - Resultados equivalentes");
        Serial.println("     - Mejor para aplicaciones de alto throughput");
    } else if (speedup > 1.1) {
        Serial.println("  >> Procesamiento por buffer es preferible");
        Serial.println("    - Ligeramente mas rapido");
        Serial.println("    - Resultados similares");
    } else {
        Serial.println("  >> Ambos metodos son viables");
        Serial.println("    - Usar sample-by-sample para: minima latencia, debugging");
        Serial.println("    - Usar buffer para: maximo throughput, batch processing");
    }
    Serial.println();
    
    for (int i = 0; i < 72; i++) Serial.print("=");
    Serial.println();
}

// ============================================================================
// SETUP Y LOOP
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000); // Esperar hasta 5 segundos por Serial
    delay(1000);
    
    Serial.println("\n\n");
    Serial.println("+----------------------------------------------------------------------+");
    Serial.println("|                                                                      |");
    Serial.println("|         TEST DE FILTROS IIR - BioFilterLib (CMSIS-DSP)              |");
    Serial.println("|                                                                      |");
    Serial.println("|  Libreria: BioFilterLib (wrapper CMSIS-DSP)                         |");
    Serial.println("|  Filtro: IIR Notch 60Hz (Biquad, orden 2)                           |");
    Serial.println("|  Señales: ECG @ 960 Hz (Waveforms.h)                                |");
    Serial.println("|  Muestras: 2000 (compatible con Test_ttapa_IIR.ino)                 |");
    Serial.println("|                                                                      |");
    Serial.println("|  COMPARACION DIRECTA con Arduino-Filters (Ttapa)                    |");
    Serial.println("|  - Mismos coeficientes (f0=60Hz, Q=30)                              |");
    Serial.println("|  - Mismo numero de muestras (2000)                                  |");
    Serial.println("|  - Mismas señales de prueba                                         |");
    Serial.println("|                                                                      |");
    Serial.println("+----------------------------------------------------------------------+");
    Serial.println();
    
    // Informacion del sistema
    Serial.println("Informacion del Sistema:");
    #ifdef ARDUINO_TEENSY40
        Serial.println("  Placa: Teensy 4.0");
        Serial.println("  MCU: ARM Cortex-M7 @ 600 MHz");
    #elif defined(ARDUINO_TEENSY41)
        Serial.println("  Placa: Teensy 4.1");
        Serial.println("  MCU: ARM Cortex-M7 @ 600 MHz");
    #elif defined(ARDUINO_SAM_DUE)
        Serial.println("  Placa: Arduino Due");
        Serial.println("  MCU: ARM Cortex-M3 @ 84 MHz");
    #elif defined(ESP32)
        Serial.println("  Placa: ESP32");
        Serial.print("  MCU: Xtensa LX6 @ ");
        Serial.print(ESP.getCpuFreqMHz());
        Serial.println(" MHz");
    #else
        Serial.println("  Placa: Desconocida");
    #endif
    
    Serial.println("  Frecuencia de muestreo: " + String((int)FS) + " Hz");
    Serial.println("  Muestras a procesar: " + String(NUM_SAMPLES));
    Serial.println("  CMSIS-DSP: Optimizaciones ARM habilitadas");
    Serial.println();
    
    delay(2000);
    
    // Ejecutar test principal
    runFilterTest();
    
    // Mostrar resultados
    printResults();
    
    // Test adicional de procesamiento por buffer
    testBufferProcessing();
    
    // Opcion de exportar datos
    exportData();
    
    Serial.println("\n+----------------------------------------------------------------------+");
    Serial.println("|                                                                      |");
    Serial.println("|                      TEST COMPLETADO                                 |");
    Serial.println("|                                                                      |");
    Serial.println("|  Presione RESET para ejecutar nuevamente                            |");
    Serial.println("|                                                                      |");
    Serial.println("|  NOTA: Compare estos resultados con Test_ttapa_IIR.ino para         |");
    Serial.println("|  evaluar diferencias de rendimiento entre librerias                 |");
    Serial.println("|                                                                      |");
    Serial.println("+----------------------------------------------------------------------+");
    Serial.println();
}

void loop() {
    // Test ejecutado una vez en setup()
    delay(1000);
}