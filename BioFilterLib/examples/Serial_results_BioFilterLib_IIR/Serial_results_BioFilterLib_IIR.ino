/**
 * @file Serial_results_IIR_BioFilterLib.ino
 * @brief Test de filtrado con IIR Notch de BioFilterLib (CMSIS-DSP)
 * @author Test
 * @version 1.0
 * 
 * Elimina el ruido de 60Hz de una señal ECG usando un filtro IIR Notch
 * de BioFilterLib (wrapper CMSIS-DSP) y muestra resultados
 * 
 * Parámetros del filtro:
 * - Frecuencia central: 60 Hz
 * - Factor de calidad Q: 30 (notch estrecho)
 * - Frecuencia de muestreo: 960 Hz
 * 
 * IMPORTANTE: Usa los MISMOS coeficientes que Serial_results_IIR_Ttapa.ino
 * para permitir comparación directa de resultados
 */

#include <BioFilterLib.h>

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

#define NUM_SAMPLES 2000              // Número de muestras a procesar
#define SAMPLE_RATE 960               // Frecuencia de muestreo (Hz)

// Coeficientes del filtro Notch (f0=60Hz, Q=30, fs=960Hz)
// Diseñado con scipy.signal.iirnotch
// Formato CMSIS Biquad: {b0, b1, b2, a1, a2} (a0=1 se omite)
// Los MISMOS coeficientes usados en Serial_results_IIR_Ttapa.ino
const float32_t notch_coeffs[5] = {
    0.99349748f, -1.83574398f,  0.99349748f,  // b0, b1, b2
    1.83574398f,  -0.98699496f                 // -a1, -a2
};

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

float32_t noisySignal[NUM_SAMPLES];        // Señal con ruido de 60Hz
float32_t filteredSignal[NUM_SAMPLES];     // Señal filtrada (sin 60Hz)

// Crear instancia del filtro IIR de BioFilterLib
// - 1 stage: filtro de orden 2 (una sección Biquad)
// - blockSize 1: procesamiento muestra por muestra (tiempo real)
IIRFilter notchFilter((float32_t*)notch_coeffs, 1, 1);

// ============================================================================
// FUNCIONES
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    
    // Serial.println("========================================");
    // Serial.println("  IIR NOTCH - Eliminación de 60Hz");
    // Serial.println("  BioFilterLib (CMSIS-DSP)");
    // Serial.println("========================================\n");
    
    // Cargar señal con ruido de 60Hz
    // Serial.print("Cargando señal con ruido de 60Hz... ");
    if (!loadSignal(noisySignal, "ecg_60hz_noised", NUM_SAMPLES)) {
        Serial.println("ERROR!");
        while(1);
    }
    // Serial.println("OK");
    
    // Procesar señal muestra por muestra
    Serial.print("Filtrando señal... ");
    for (int i = 0; i < NUM_SAMPLES; i++) {
        filteredSignal[i] = notchFilter.processSample(noisySignal[i]);
    }
    Serial.println("OK\n");
    
    // Imprimir datos para Serial Plotter
    // Los filtros notch tienen delay de grupo muy pequeño (~0-1 muestras)
    int delay_samples = 0;  // Notch tiene delay mínimo
    
    for (int i = 0; i < NUM_SAMPLES - delay_samples; i++) {
        // Formato: valor1 valor2 (separados por espacio)
        Serial.print(noisySignal[i], 4);
        Serial.print(" ");
        Serial.println(filteredSignal[i + delay_samples], 4);
        
        delay(5); // Pequeña pausa para visualización suave
    }
    
    // Serial.println("\n========================================");
    // Serial.println("Filtrado completado con IIR Notch!");
    // Serial.println("60Hz eliminado exitosamente");
    // Serial.println("Usa el Serial Plotter para visualizar");
    // Serial.println("========================================");
}

void loop() {
    // Nada que hacer
}
