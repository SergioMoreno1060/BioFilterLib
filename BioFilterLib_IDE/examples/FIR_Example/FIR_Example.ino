/**
 * @file FIR_Example.ino
 * @brief Ejemplo simple de filtrado FIR con visualización en Serial Plotter
 * @author BioFilterLib
 * @version 1.0.0
 * @date 2025
 * 
 * @details Este sketch demuestra el uso básico de FIRFilter:
 *   - Carga señales de prueba desde Waveforms.h
 *   - Aplica filtro pasa-bajas para eliminar ruido de 60Hz
 *   - Muestra señal original y filtrada en Serial Plotter
 *   - Loop continuo para visualización en tiempo real
 * 
 * Hardware: Arduino Due
 * Frecuencia de muestreo: 960 Hz
 * 
 * CÓMO USAR:
 * 1. Subir el sketch al Arduino Due
 * 2. Abrir Serial Plotter (Herramientas -> Serial Plotter)
 * 3. Configurar baud rate a 115200
 * 4. Ver las señales actualizándose en tiempo real
 * 
 * SEÑALES VISUALIZADAS:
 * - Rojo: Señal ECG con ruido de 60Hz (original)
 * - Azul: Señal ECG filtrada (limpia)
 */

#include <BioFilterLib.h>

// ============================================================================
// CONFIGURACIÓN DEL FILTRO
// ============================================================================

// Coeficientes para filtro FIR pasa-bajas
// fs=960Hz, fc=50Hz, orden=50, ventana=Hamming
// Diseñado para eliminar ruido de 60Hz de señales ECG
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

// ============================================================================
// CONFIGURACIÓN DE SEÑALES
// ============================================================================

// Número de muestras a procesar y mostrar
const uint16_t SIGNAL_LENGTH = 2780;  // ~1.56 segundos @ 960Hz

// Buffers para señales
float32_t ecgNoisy[SIGNAL_LENGTH];      // Señal con ruido de 60Hz
float32_t ecgFiltered[SIGNAL_LENGTH];   // Señal filtrada

// Instancia del filtro FIR
FIRFilter* lowpassFilter = nullptr;

// Control de loop
uint16_t currentSample = 0;              // Índice de muestra actual
const uint16_t SAMPLES_PER_UPDATE = 900;   // Muestras a mostrar por iteración
const uint16_t DELAY_MS = 10;            // Delay entre actualizaciones (ms)

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    while (!Serial) {
        ; // Esperar conexión serial
    }
    
    delay(2000);
    
    // Mensaje de inicio
    Serial.println("========================================");
    Serial.println("  BioFilterLib - FIR Filter Example");
    Serial.println("  Visualizacion en Serial Plotter");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Configuracion:");
    Serial.println("  - Filtro: Pasa-bajas FIR (fc=50Hz)");
    Serial.println("  - Orden: 50 (51 coeficientes)");
    Serial.println("  - Aplicacion: Eliminar ruido 60Hz");
    Serial.println("  - Señal: ECG @ 960Hz");
    Serial.println();
    Serial.println("Cargando señales...");
    
    // Cargar señal ECG con ruido de 60Hz desde Waveforms.h
    // Intentar primero con etiqueta, luego con índice
    if (!loadSignal(ecgNoisy, "ecg_60hz_noised", SIGNAL_LENGTH)) {
        if (!loadSignal(ecgNoisy, "1", SIGNAL_LENGTH)) {
            Serial.println("ERROR: No se pudo cargar señal ECG");
            Serial.println("Verifica que Waveforms.h este disponible");
            while(1);  // Detener ejecución
        }
    }
    
    Serial.println("OK: Señal cargada correctamente");
    Serial.println();
    Serial.println("Creando filtro FIR...");
    
    // Crear instancia del filtro
    // blockSize=1 para máxima compatibilidad
    lowpassFilter = new FIRFilter(lowpassCoeffs, NUM_TAPS, 1);
    
    Serial.println("OK: Filtro creado");
    Serial.println();
    Serial.println("Aplicando filtro a toda la señal...");
    
    // Aplicar filtro a toda la señal de una vez
    lowpassFilter->processBuffer(ecgNoisy, ecgFiltered, SIGNAL_LENGTH);
    
    Serial.println("OK: Filtrado completado");
    Serial.println();
    Serial.println("========================================");
    Serial.println("  Iniciando visualizacion...");
    Serial.println("  Abre Serial Plotter para ver graficas");
    Serial.println("========================================");
    Serial.println();
    
    delay(2000);
    
    // Limpiar buffer serial antes de empezar el loop
    while(Serial.available()) {
        Serial.read();
    }
}

// ============================================================================
// LOOP - Visualización continua
// ============================================================================

void loop() {

    // if (currentSample % 20 == 0) {
    //     // Muestra una sola muestra por iteración (sin retardos grandes)
    //     Serial.print(ecgNoisy[currentSample], 4); 
    //     Serial.print(" ");
    //     Serial.print(ecgFiltered[currentSample], 4);
    //     Serial.print(" ");
    //     Serial.print(1.2f);
    //     Serial.print(" ");
    //     Serial.println(-1.2f);
        
    //     if (currentSample >= SIGNAL_LENGTH) {
    //         currentSample = 0;
    //         delay(10); // Pausa al reiniciar
    //     }
    // }
    // currentSample++;

    
    // Muestra una sola muestra por iteración (sin retardos grandes)
    Serial.print(ecgNoisy[currentSample], 4); 
    Serial.print(" ");
    Serial.print(ecgFiltered[currentSample + 25], 4);
    Serial.print(" ");
    Serial.print(1.2f);
    Serial.print(" ");
    Serial.println(-1.2f);
        
    if (currentSample >= SIGNAL_LENGTH) {
         currentSample = 0;
        delay(1); // Pausa al reiniciar
    }
    currentSample++;

    // Esperar exactamente el tiempo de muestreo: 1/960 s ≈ 1.04 ms
    delayMicroseconds(1040);
}
