/**
 * @file LMS_Example.cpp
 * @brief Ejemplo de cancelación adaptativa de interferencia de línea eléctrica usando LMSFilter
 * @author Sergio
 * @version 1.0.0
 * @date 2025
 *
 * @details
 * Este ejemplo demuestra cómo configurar y utilizar un filtro LMS (Least Mean Squares)
 * adaptativo para eliminar interferencia de línea eléctrica (60 Hz) de una señal de ECG
 * en tiempo real.
 *
 * El ejemplo realiza lo siguiente:
 * 1. Carga una señal ECG limpia desde la tabla de ondas predefinida
 * 2. Genera interferencia sinusoidal de 60 Hz con amplitud variable
 * 3. Contamina la señal ECG con esta interferencia simulando condiciones reales
 * 4. Utiliza un filtro LMS adaptativo para cancelar la interferencia automáticamente
 * 5. Muestra en el puerto serie: ECG original, ECG contaminado, ECG limpio y error de adaptación
 * 6. Demuestra la adaptación en tiempo real variando la amplitud de la interferencia
 *
 * @note
 * Para visualizar los resultados:
 * 1. Abre el Monitor Serie de Arduino a 115200 baudios
 * 2. Selecciona "Herramientas -> Trazador Serie" para ver las gráficas en tiempo real
 * 3. Observa cómo el filtro se adapta automáticamente cuando cambia la interferencia
 * 
 * Verás cuatro líneas:
 * - Azul: ECG original (limpio)
 * - Rojo: ECG contaminado con interferencia de 60Hz
 * - Verde: ECG filtrado por el LMS (debería parecerse al original)
 * - Amarillo: Error de adaptación (idealmente debería tender a cero)
 */

#include <Arduino.h>
#include <BioFilterLib.h> // Incluir nuestra clase LMSFilter
#include "Waveforms.h"  // Para acceder a la señal ECG predefinida

// --- 1. Configuración del Filtro LMS ---

/**
 * @brief Configuración de parámetros del filtro adaptativo
 */
#define NUM_TAPS_LMS    32          // Número de coeficientes del filtro (orden + 1)
#define MU_ADAPTATION   0.02f       // Paso de adaptación (balance velocidad/estabilidad)
#define BLOCK_SIZE      1           // Procesamiento muestra por muestra (tiempo real)

/**
 * @brief Configuración de la simulación de señales
 */
#define SAMPLING_FREQ   1000.0f      // Frecuencia de muestreo típica para ECG 250(Hz)
#define POWERLINE_FREQ  60.0f       // Frecuencia de interferencia de línea eléctrica (Hz)
#define ECG_AMPLITUDE   1.0f        // Amplitud de la señal ECG normalizada
#define INTERFERENCE_BASE_AMP 0.3f  // Amplitud base de interferencia
#define INTERFERENCE_VAR_AMP  0.2f  // Variación de amplitud para demostrar adaptación

/**
 * @brief Arrays para almacenar coeficientes y datos
 */
float32_t lmsCoefficients[NUM_TAPS_LMS];  // Coeficientes adaptativos (serán modificados)
float32_t ecgCleanSignal[maxSamplesNum];  // Señal ECG original limpia
float32_t ecgContaminated[maxSamplesNum]; // Señal ECG contaminada con interferencia
float32_t ecgFiltered[maxSamplesNum];     // Señal ECG filtrada por LMS
float32_t adaptationError[maxSamplesNum]; // Error de adaptación del LMS

// --- 2. Instanciación del Filtro LMS ---

/**
 * @brief Crear instancia del filtro LMS adaptativo
 * Parámetros optimizados para cancelación de interferencia de línea eléctrica:
 * - 32 coeficientes: suficientes para modelar la interferencia sinusoidal
 * - mu = 0.02: balance entre velocidad de adaptación y estabilidad
 * - blockSize = 1: procesamiento en tiempo real muestra por muestra
 */
LMSFilter* powerlineFilter = nullptr;

// --- 3. Variables para Generación de Señales ---

uint32_t sampleCounter = 0;              // Contador de muestras procesadas
uint32_t ecgSampleIndex = 0;             // Índice para la señal ECG cíclica
float32_t timeSeconds = 0.0f;            // Tiempo transcurrido en segundos

/**
 * @brief Función de configuración inicial
 */
void setup() {
    Serial.begin(9600);
    delay(1000);
    
    Serial.println("=== Ejemplo de Filtro LMS Adaptativo ===");
    Serial.println("Cancelación de Interferencia de Línea Eléctrica en ECG");
    Serial.println("Configuración:");
    Serial.print("- Coeficientes LMS: "); Serial.println(NUM_TAPS_LMS);
    Serial.print("- Paso de adaptación (mu): "); Serial.println(MU_ADAPTATION, 4);
    Serial.print("- Frecuencia de muestreo: "); Serial.print(SAMPLING_FREQ); Serial.println(" Hz");
    Serial.print("- Interferencia simulada: "); Serial.print(POWERLINE_FREQ); Serial.println(" Hz");
    Serial.println();
    
    // --- 4. Inicializar Coeficientes del Filtro LMS ---
    
    Serial.println("Inicializando coeficientes del filtro LMS...");
    
    // Inicializar todos los coeficientes a cero (estrategia típica para LMS)
    // El filtro aprenderá automáticamente los coeficientes óptimos durante la adaptación
    for (int i = 0; i < NUM_TAPS_LMS; i++) {
        lmsCoefficients[i] = 0.0f;
    }
    
    // Alternativamente, se podrían inicializar con valores aleatorios pequeños:
    // for (int i = 0; i < NUM_TAPS_LMS; i++) {
    //     lmsCoefficients[i] = ((float32_t)random(-1000, 1000)) / 100000.0f; // ±0.01 máx
    // }
    
    // --- 5. Preparar Señal ECG Limpia ---
    
    Serial.println("Convirtiendo señal ECG a formato flotante...");
    
    // Convertir la señal ECG de uint16_t a float32_t y normalizar
    // La señal original está en rango 0-4095 (12-bit ADC simulado)
    // La convertimos a rango ±1.0 para procesamiento con punto flotante
    for (int i = 0; i < maxSamplesNum; i++) {
        // Normalizar de 12-bit (0-4095) a rango ±1.0
        ecgCleanSignal[i] = ((float32_t)waveformsTable[4][i] - 2048.0f) / 2048.0f * ECG_AMPLITUDE;
    }
    
    // --- 6. Crear Instancia del Filtro LMS ---
    
    Serial.println("Creando filtro LMS adaptativo...");
    
    powerlineFilter = new LMSFilter(lmsCoefficients,    // Coeficientes adaptativos
                                   NUM_TAPS_LMS,        // Número de coeficientes
                                   MU_ADAPTATION,       // Paso de adaptación
                                   BLOCK_SIZE);         // Tamaño de bloque
    
    Serial.println("Filtro LMS inicializado correctamente.");
    Serial.println();
    Serial.println("Iniciando procesamiento adaptativo en tiempo real...");
    Serial.println("Formato: ECG_Original:ECG_Contaminado:ECG_Filtrado:Error_Adaptacion");
    Serial.println();
    
    // Pequeña pausa antes de comenzar el procesamiento
    delay(1000);
}

/**
 * @brief Bucle principal - Procesamiento en tiempo real
 */
void loop() {
    // --- 7. Generar Señales para Esta Muestra ---
    
    // Calcular tiempo actual
    timeSeconds = (float32_t)sampleCounter / SAMPLING_FREQ;
    
    // Obtener muestra de ECG limpio (cíclica)
    float32_t cleanECG = ecgCleanSignal[ecgSampleIndex];
    
    // Generar interferencia de línea eléctrica (60 Hz) con amplitud variable
    // La amplitud varía lentamente para demostrar la capacidad adaptativa del filtro
    float32_t interferenceAmplitude = INTERFERENCE_BASE_AMP + 
                                     INTERFERENCE_VAR_AMP * arm_sin_f32(2 * PI * 0.1f * timeSeconds);
    float32_t powerlineInterference = interferenceAmplitude * arm_sin_f32(2 * PI * POWERLINE_FREQ * timeSeconds);
    
    // Crear señal de referencia (conocimiento a priori de la interferencia)
    // En aplicaciones reales, esta podría venir de un sensor de referencia
    // o ser generada internamente si se conoce la frecuencia de interferencia
    float32_t interferenceReference = arm_sin_f32(2 * PI * POWERLINE_FREQ * timeSeconds);
    
    // Contaminar la señal ECG con interferencia
    float32_t contaminatedECG = cleanECG + powerlineInterference;
    
    // --- 8. Procesar con Filtro LMS Adaptativo ---
    
    float32_t filteredECG = 0.0f;
    float32_t adaptiveError = 0.0f;
    
    // El filtro LMS intenta minimizar la diferencia entre:
    // - Entrada: ECG contaminado
    // - Referencia: señal de interferencia conocida  
    // - Salida: estimación de la interferencia
    // - Error: ECG limpio (contaminado - interferencia estimada)
    
    powerlineFilter->processSample(contaminatedECG,      // Señal de entrada (ECG + interferencia)
                                  interferenceReference, // Referencia de interferencia
                                  &filteredECG,          // Salida del filtro (interferencia estimada)
                                  &adaptiveError);       // Error = entrada - interferencia_estimada
    
    // En cancelación de interferencia, el "error" del LMS es en realidad
    // la señal deseada (ECG limpio). El filtro aprende a estimar la interferencia
    // de tal manera que al restarla de la entrada, quede solo el ECG.
    float32_t ecgCleanEstimate = adaptiveError;  // El error es nuestra señal limpia estimada
    
    // --- 9. Enviar Datos para Visualización ---
    
    // Formato para el Trazador Serie de Arduino (separado por dos puntos)
    Serial.print(cleanECG, 4);                    // ECG original (referencia)
    Serial.print(":");
    Serial.print(contaminatedECG, 4);             // ECG contaminado
    Serial.print(":");  
    Serial.print(ecgCleanEstimate, 4);            // ECG filtrado por LMS
    Serial.print(":");
    Serial.println(filteredECG, 4);               // Interferencia estimada por LMS
    
    // --- 10. Actualizar Contadores y Estados ---
    
    sampleCounter++;
    ecgSampleIndex = (ecgSampleIndex + 1) % maxSamplesNum;  // Hacer la señal ECG cíclica
    
    // --- 11. Demostración de Cambio Dinámico de μ (Opcional) ---
    
    // Cada 5 segundos, demostrar el ajuste dinámico del paso de adaptación
    if (sampleCounter % (5 * (int)SAMPLING_FREQ) == 0 && sampleCounter > 0) {
        float32_t currentMu = powerlineFilter->getMu();
        float32_t newMu = (currentMu > 0.01f) ? 0.005f : 0.03f;  // Alternar entre rápido y lento
        
        powerlineFilter->setMu(newMu);
        
        // Comentar las siguientes líneas si no se desea debugging en el trazador
        /*
        Serial.print("# Cambiando mu de ");
        Serial.print(currentMu, 4);
        Serial.print(" a ");
        Serial.println(newMu, 4);
        */
    }
    
    // --- 12. Información de Estado Periódica (Opcional) ---
    
    // Cada 10 segundos, mostrar información sobre el rendimiento del filtro
    if (sampleCounter % (10 * (int)SAMPLING_FREQ) == 0 && sampleCounter > 0) {
        // Comentar si no se desea debugging en el trazador
        /*
        Serial.print("# Muestras procesadas: ");
        Serial.print(sampleCounter);
        Serial.print(", Tiempo: ");
        Serial.print(timeSeconds, 1);
        Serial.print("s, Mu actual: ");
        Serial.println(powerlineFilter->getMu(), 4);
        */
    }
    
    // --- 13. Simular Frecuencia de Muestreo ---
    
    // Esperar para simular la frecuencia de muestreo de 250 Hz
    // En una aplicación real, esto sería controlado por un timer o interrupción
    delay(1000.0f / SAMPLING_FREQ);  // 4ms para 250Hz
}

/**
 * @note Interpretación de los Resultados:
 * 
 * Al observar el Trazador Serie, deberías ver:
 * 
 * 1. **Línea Azul (ECG Original)**: La señal ECG limpia de referencia
 * 2. **Línea Roja (ECG Contaminado)**: ECG con interferencia de 60Hz superpuesta
 * 3. **Línea Verde (ECG Filtrado)**: Resultado del filtro LMS, debería parecerse al original
 * 4. **Línea Amarilla (Interferencia Estimada)**: Lo que el filtro "aprendió" sobre la interferencia
 * 
 * **Indicadores de Buena Adaptación:**
 * - La línea verde (filtrada) converge hacia la azul (original)
 * - La línea amarilla se estabiliza en una forma sinusoidal pura
 * - La diferencia entre líneas verdes y azul es mínima después de unos segundos
 * 
 * **Efectos Observables:**
 * - **Convergencia Inicial**: Los primeros 1-3 segundos el filtro "aprende"
 * - **Adaptación Dinámica**: Cuando cambia la amplitud de interferencia, el filtro se readapta
 * - **Cambios de μ**: Cada 5 segundos, observa cómo cambia la velocidad de adaptación
 * 
 * **Aplicaciones Prácticas Similares:**
 * - Cancelación de interferencia electromagnética en hospitales
 * - Eliminación de artefactos de movimiento en monitoreo ambulatorio
 * - Supresión de ruido de línea eléctrica en equipos portátiles
 * - Cancelación de interferencia de dispositivos móviles
 */

/**
 * @example Configuraciones Alternativas para Diferentes Escenarios:
 * 
 * // Para interferencia muy fuerte (alta amplitud):
 * #define MU_ADAPTATION 0.05f  // Adaptación más rápida
 * #define NUM_TAPS_LMS 48      // Más coeficientes para mejor modelado
 * 
 * // Para señales con mucho ruido:
 * #define MU_ADAPTATION 0.001f // Adaptación muy lenta y estable
 * #define NUM_TAPS_LMS 16      // Menos coeficientes para evitar sobreajuste
 * 
 * // Para interferencia de 50Hz (Europa):
 * #define POWERLINE_FREQ 50.0f
 * 
 * // Para múltiples armónicos de interferencia:
 * #define NUM_TAPS_LMS 64      // Más coeficientes para capturar armónicos
 */