/**
 * @file Test_Wavelet_SerialPlotter.ino
 * @brief Visualización de componentes Wavelet en Serial Plotter
 * @author Sergio
 * 
 * @details Este sketch procesa una señal ECG con ruido usando filtro Wavelet
 * Daubechies-4 y muestra en el Serial Plotter:
 * - Señal original (con ruido de 60Hz)
 * - Aproximación (componentes de baja frecuencia)
 * - Detalle (componentes de alta frecuencia)
 * 
 * @note Configurar Serial Plotter a 115200 baud
 */

#include <BioFilterLib.h>

// ════════════════════════════════════════════════════════════════
// CONFIGURACIÓN
// ════════════════════════════════════════════════════════════════
const uint16_t SIGNAL_LENGTH = 2000;      // Número de muestras
const uint32_t SAMPLE_RATE = 960;         // Hz
const uint16_t BLOCK_SIZE = SIGNAL_LENGTH;

// Buffers
static float32_t ecgNoisy[SIGNAL_LENGTH];      // Señal con ruido
static float32_t approxCoeffs[SIGNAL_LENGTH];  // Coeficientes aproximación
static float32_t detailCoeffs[SIGNAL_LENGTH];  // Coeficientes detalle
static float32_t approxSignal[SIGNAL_LENGTH];  // Señal reconstruida (aprox)
static float32_t detailSignal[SIGNAL_LENGTH];  // Señal reconstruida (detalle)

WaveletFilter* waveletFilter;

// Variables para envío al plotter
uint16_t currentSample = 0;
bool processingComplete = false;

// ════════════════════════════════════════════════════════════════
// SETUP
// ════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    // Pequeño retardo para que el Serial Plotter se inicialice
    delay(2000);
    
    Serial.println("Iniciando procesamiento...");
    delay(500);
    
    // 1. Cargar señal ECG con ruido
    if (!loadSignal(ecgNoisy, "ecg_320hz_noised", SIGNAL_LENGTH)) {
        Serial.println("ERROR: No se pudo cargar la señal");
        while(1);
    }
    
    // 2. Crear y configurar filtro Wavelet
    waveletFilter = new WaveletFilter(BLOCK_SIZE);
    
    // 3. Procesar señal completa
    waveletFilter->processBuffer(ecgNoisy, approxCoeffs, detailCoeffs, SIGNAL_LENGTH);
    
    // 4. Reconstruir señales
    for (uint16_t i = 0; i < SIGNAL_LENGTH; i++) {
        // Reconstruir solo aproximación (señal suavizada)
        approxSignal[i] = waveletFilter->reconstruct(approxCoeffs[i], 0.0f);

        // Reconstruir solo detalle (componentes de alta frecuencia)
        detailSignal[i] = waveletFilter->reconstruct(0.0f, detailCoeffs[i]);
    }
    
    Serial.println("Procesamiento completo. Mostrando datos...");
    delay(500);
    
    processingComplete = true;

    if (!processingComplete) {
        return;
    }
    
    // Enviar muestra actual en formato Serial Plotter
    // Formato: Label1:value1 Label2:value2 Label3:value3
    for (uint16_t i = 0; i < SIGNAL_LENGTH; i++) {
        // Serial.print("Original:");
        Serial.print(ecgNoisy[i], 4);
        Serial.print(" ");
        
        // Serial.print("Aproximacion:");
        Serial.print(approxSignal[i], 4);
        Serial.print(" ");
        
        // Serial.print("Coeficientes approx:");
        Serial.print(approxCoeffs[i], 4);
        Serial.print(" ");

        // Serial.print("Coeficientes detail:");
        Serial.println(detailCoeffs[i], 4);
        
    }
}

// ════════════════════════════════════════════════════════════════
// LOOP - Enviar datos al Serial Plotter
// ════════════════════════════════════════════════════════════════
void loop() {
    // if (!processingComplete) {
    //     return;
    // }
    
    // // Enviar muestra actual en formato Serial Plotter
    // // Formato: Label1:value1 Label2:value2 Label3:value3
    
    // Serial.print("Original:");
    // Serial.print(ecgNoisy[currentSample], 4);
    // Serial.print(",");
    
    // Serial.print("Aproximacion:");
    // Serial.print(approxSignal[currentSample], 4);
    // Serial.print(",");
    
    // Serial.print("Detalle:");
    // Serial.println(detailSignal[currentSample], 4);
    
    // // Avanzar al siguiente sample
    // currentSample++;
    
    // // Reiniciar cuando termine
    // if (currentSample >= SIGNAL_LENGTH) {
    //     currentSample = 0;
    //     delay(1000);  // Pausa de 1 segundo entre ciclos
    // }
    
    // // Control de velocidad de envío
    // // Ajustar este delay para controlar la velocidad de visualización
    // delay(5);  // 5ms entre muestras = ~200 muestras/seg en el plotter
}
