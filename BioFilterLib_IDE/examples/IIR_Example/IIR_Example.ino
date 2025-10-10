/**
 * @file IIR_Example.cpp
 * @brief Ejemplo de uso de la clase IIRFilter para eliminar la deriva de línea base en una señal simulada de ECG.
 * @author Sergio Moreno
 * @version 1.0.0
 * @date 2025
 *
 * @details
 * Este ejemplo demuestra cómo configurar y utilizar un filtro IIR (Infinite Impulse Response)
 * de tipo Biquad para procesamiento de bioseñales en tiempo real.
 *
 * El ejemplo realiza lo siguiente:
 * 1.  Define los coeficientes para un filtro pasa-altas Butterworth de 2º orden con una
 * frecuencia de corte de 0.5 Hz, ideal para eliminar la deriva de la línea base.
 * 2.  Instancia un objeto IIRFilter con estos coeficientes.
 * 3.  Genera una señal de prueba que simula un ECG (onda de 10 Hz) superpuesto
 * con una onda sinusoidal lenta (0.2 Hz) que representa la deriva de la línea base.
 * 4.  En cada iteración del bucle, procesa una muestra de la señal con el método processSample().
 * 5.  Envía la señal original y la señal filtrada al puerto serie, formateadas para
 * su visualización en el Trazador Serie de Arduino.
 *
 * @note
 * Para ver el resultado, abre el monitor serie de Arduino y luego selecciona
 * "Herramientas -> Trazador Serie". Verás dos líneas: la señal original con la
 * deriva (azul) y la señal filtrada centrada en cero (rojo).
 */

#include <Arduino.h>
#include <BioFilterLib.h> // Asegúrate de que IIRFilter.h esté en la carpeta 'src' de tu proyecto de PlatformIO

// --- 1. Definición de los Coeficientes del Filtro ---

/**
 * @brief Coeficientes para un filtro pasa-altas Butterworth de 2º orden.
 * - Frecuencia de corte (Cutoff): 0.5 Hz
 * - Frecuencia de muestreo (Sampling): 1000 Hz
 *
 * Generados usando Python con scipy.signal:
 * b, a = signal.butter(2, 0.5, 'highpass', fs=1000)
 *
 * El formato para la estructura Biquad de CMSIS-DSP es {b0, b1, b2, a1, a2}.
 * El coeficiente a0 se asume como 1.
 */
const float32_t highpassCoeffs[5] = {
    0.99778102f, -1.99556205f, 0.99778102f,  // b0, b1, b2
    -1.99555712f, 0.99556697f               // a1, a2
};

// --- 2. Instanciación del Filtro ---

// Crear una instancia del filtro IIR.
// Parámetros:
// 1. Puntero a los coeficientes: highpassCoeffs
// 2. Número de etapas Biquad: 1 (porque es un filtro de 2º orden)
// 3. Tamaño de bloque: 1 (para procesamiento en tiempo real muestra por muestra)
IIRFilter baselineFilter(highpassCoeffs, 1, 1);

// --- 3. Parámetros para la Generación de la Señal de Prueba ---

const float32_t SAMPLING_FREQUENCY = 1000.0f;
const float32_t SIGNAL_FREQUENCY = 10.0f;      // Simula la señal de ECG
const float32_t DRIFT_FREQUENCY = 0.2f;       // Simula la deriva de línea base
const float32_t SIGNAL_AMPLITUDE = 100.0f;
const float32_t DRIFT_AMPLITUDE = 80.0f;
const float32_t NOISE_AMPLITUDE = 5.0f;

uint32_t sampleIndex = 0; // Índice para generar la señal en el tiempo

/**
 * @brief Función de configuración inicial. Se ejecuta una vez al encender el microcontrolador.
 */
void setup() {
    // Inicializar la comunicación serie a 115200 baudios para la depuración
    Serial.begin(9600);
    delay(1000); // Esperar a que el puerto serie se estabilice

    Serial.println("Ejemplo de Filtro IIR para Bioseñales");
    Serial.println("Enviando datos al Trazador Serie...");
    Serial.println("Original:Filtrada"); // Cabecera para el Trazador Serie
}

/**
 * @brief Bucle principal. Se ejecuta repetidamente.
 */
void loop() {
    // --- 4. Generar una Muestra de la Señal de Prueba ---

    // Calcular el tiempo actual basado en el índice de la muestra
    float32_t t = (float32_t)sampleIndex / SAMPLING_FREQUENCY;

    // Crear la señal de interés (onda sinusoidal rápida)
    float32_t signal = SIGNAL_AMPLITUDE * arm_sin_f32(2 * PI * SIGNAL_FREQUENCY * t);

    // Crear la deriva de la línea base (onda sinusoidal lenta)
    float32_t drift = DRIFT_AMPLITUDE * arm_sin_f32(2 * PI * DRIFT_FREQUENCY * t);
    
    // Generar un poco de ruido aleatorio
    float32_t noise = ((float32_t)rand() / RAND_MAX - 0.5f) * 2.0f * NOISE_AMPLITUDE;

    // Combinar las componentes para crear la señal de entrada final
    float32_t originalSample = signal + drift + noise;

    // --- 5. Procesar la Muestra con el Filtro IIR ---

    // Aplicar el filtro pasa-altas para eliminar la componente de deriva (drift)
    float32_t filteredSample = baselineFilter.processSample(originalSample);

    // --- 6. Enviar Datos al Trazador Serie ---

    // Imprimir la señal original y la filtrada, separadas por dos puntos.
    // El Trazador Serie de Arduino usará esto para dibujar dos gráficos.
    Serial.print(originalSample);
    Serial.print(":");
    Serial.println(filteredSample);

    // Incrementar el índice de la muestra para la siguiente iteración
    sampleIndex++;

    // Esperar el tiempo correspondiente a la frecuencia de muestreo
    delay(1000.0f / SAMPLING_FREQUENCY);
}