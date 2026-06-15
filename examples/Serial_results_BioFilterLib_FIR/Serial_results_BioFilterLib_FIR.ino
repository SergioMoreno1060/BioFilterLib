/**
 * @file Serial_results_BioFilterLib.ino
 * @brief Test simple de filtrado con FIR de BioFilterLib
 * @author Test
 * @version 1.0
 * 
 * Filtra señal ECG con ruido de 60Hz usando la librería FIRFilter
 * de BioFilterLib y muestra resultados
 * 
 * IMPORTANTE: Necesita BioFilterLib.h (incluye FIRFilter y Waveforms)
 */

#include "BioFilterLib.h"

// ============================================================================
// CONFIGURACIÓN
// ============================================================================

#define NUM_SAMPLES 2000              // Número de muestras a procesar
#define SAMPLE_RATE 960               // Frecuencia de muestreo (Hz)
#define FILTERTAPS 51                 // Número de coeficientes del filtro

// Delay de grupo del filtro FIR (siempre N/2 para FIR simétrico)
#define FILTER_GROUP_DELAY (FILTERTAPS/2)

// Coeficientes del filtro paso-bajo (fc=40Hz @ fs=960Hz, 51 taps, window='hann')
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

float32_t noisySignal[NUM_SAMPLES];      // Señal con ruido
float32_t filteredSignal[NUM_SAMPLES];   // Señal filtrada
FIRFilter *firFilter;                    // Puntero al filtro FIR de BioFilterLib

// ============================================================================
// FUNCIONES
// ============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    
    // Serial.println("========================================");
    // Serial.println("  FIR BIOFILTERLIB - Visualización Simple");
    // Serial.println("========================================\n");
    
    // Cargar señal con ruido de 60Hz
    // Serial.print("Cargando señal con ruido... ");
    if (!loadSignal(noisySignal, "ecg_60hz_noised", NUM_SAMPLES)) {
        Serial.println("ERROR!");
        while(1);
    }
    // Serial.println("OK");
    
    // Crear filtro FIR de BioFilterLib con coeficientes personalizados
    // blockSize = 1 para procesamiento muestra por muestra (tiempo real)
    // Serial.print("Configurando filtro FIR... ");
    firFilter = new FIRFilter(coefs, FILTERTAPS, 1);
    // Serial.println("OK");
    
    // Procesar señal muestra por muestra
    Serial.print("Filtrando señal... ");
    for (int i = 0; i < NUM_SAMPLES; i++) {
        filteredSignal[i] = firFilter->processSample(noisySignal[i]);
    }
    Serial.println("OK\n");
    
    // Imprimir encabezados para Serial Plotter
    // Serial.println("Señal_Ruidosa Señal_Filtrada");
    // Serial.println("========================================");
    
    // Imprimir datos para Serial Plotter
    // Compensar el retardo del filtro (FILTERTAPS/2 ≈ 25 muestras)
    for (int i = 0; i < NUM_SAMPLES - FILTER_GROUP_DELAY; i++) {
        // Formato: valor1 valor2 (separados por espacio)
        Serial.print(noisySignal[i], 4);
        Serial.print(" ");
        Serial.println(filteredSignal[i + FILTER_GROUP_DELAY], 4);
        
        delay(5); // Pequeña pausa para visualización suave
    }
    
    // Serial.println("\n========================================");
    // Serial.println("Filtrado completado con BioFilterLib!");
    // Serial.println("Usa el Serial Plotter para visualizar");
    // Serial.println("========================================");
}

void loop() {
    // Nada que hacer
}