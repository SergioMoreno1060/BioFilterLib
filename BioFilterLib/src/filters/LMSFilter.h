/**
 * @file LMSFilter.h
 * @brief Filtro LMS (Least Mean Squares) adaptativo para bioseñales usando CMSIS-DSP
 * @author Sergio
 * @version 1.0.0
 * @date 2025
 * 
 * @details Este archivo contiene la implementación de un wrapper C++ para los filtros LMS
 * adaptativos de CMSIS-DSP, diseñado específicamente para el procesamiento de bioseñales 
 * en Arduino Due. La clase LMSFilter facilita el uso de filtros adaptativos manteniendo 
 * las optimizaciones de hardware que ofrece CMSIS-DSP para procesadores ARM Cortex-M.
 * 
 * @note Requiere CMSIS-DSP y está optimizado para Arduino Due (ARM Cortex-M3)
 * 
 * @par Ejemplo
 * @code
 * // Coeficientes iniciales para un filtro LMS de 32 taps
 * float32_t initialCoeffs[32] = {0.0f}; // Inicializar a cero
 * 
 * // Crear filtro LMS adaptativo con mu = 0.01
 * LMSFilter adaptiveFilter(initialCoeffs, 32, 0.01f, 1);
 * 
 * // Procesar una muestra individual (necesita señal de entrada Y referencia)
 * float32_t output, error;
 * adaptiveFilter.processSample(inputSample, referenceSample, &output, &error);
 * 
 * // Procesar un buffer completo
 * adaptiveFilter.processBuffer(inputBuffer, referenceBuffer, outputBuffer, 
 *                              errorBuffer, bufferSize);
 * @endcode
 */

#ifndef LMS_FILTER_H
#define LMS_FILTER_H

#include <Arduino.h>
#include <arm_math.h> // CMSIS-DSP

/**
 * @class LMSFilter
 * @brief Wrapper C++ para filtros LMS adaptativos de CMSIS-DSP, optimizado para bioseñales
 * 
 * Esta clase encapsula la funcionalidad de filtros LMS (Least Mean Squares) adaptativos
 * de CMSIS-DSP, proporcionando una interfaz amigable para el procesamiento adaptativo
 * de señales biomédicas como ECG, EMG, EEG, etc.
 * 
 * @details Los filtros LMS adaptativos son especialmente útiles para:
 * - Cancelación adaptativa de artefactos (movimiento, parpadeo, respiración)
 * - Eliminación de interferencia de línea eléctrica variable (50/60 Hz)
 * - Supresión adaptativa de ruido con características cambiantes
 * - Identificación de sistemas biológicos
 * - Ecualización adaptativa de canales de transmisión
 * 
 * A diferencia de los filtros FIR/IIR fijos, los filtros LMS ajustan automáticamente
 * sus coeficientes en tiempo real para minimizar el error entre la señal deseada
 * y la salida del filtro. Esto los hace ideales para entornos donde las características
 * del ruido o la señal cambian con el tiempo.
 * 
 * La clase maneja automáticamente:
 * - Inicialización del estado interno del filtro adaptativo
 * - Gestión de memoria para coeficientes y buffer de estados
 * - Actualización adaptativa de coeficientes usando el algoritmo LMS
 * - Interfaz con las funciones optimizadas de CMSIS-DSP
 * - Procesamiento tanto de muestras individuales como de buffers
 * 
 * @note Los coeficientes se actualizan automáticamente durante el procesamiento.
 * Se recomienda inicializar los coeficientes a cero o valores pequeños aleatorios.
 * 
 * @warning La memoria para los coeficientes iniciales debe mantenerse válida durante 
 * toda la vida del objeto LMSFilter, ya que se modifica internamente.
 * 
 * @see arm_lms_f32() para más detalles sobre la implementación subyacente de CMSIS-DSP
 */
class LMSFilter {
    public:
        /**
         * @brief Constructor de la clase LMSFilter
         * 
         * Inicializa un filtro LMS adaptativo con los parámetros especificados.
         * Los coeficientes se ajustarán automáticamente durante el procesamiento
         * para minimizar el error cuadrático medio.
         * 
         * @param coeffs Puntero al array de coeficientes iniciales (será modificado durante la adaptación)
         * @param numTaps Número de coeficientes del filtro (orden + 1)
         * @param mu Paso de adaptación (step size) - controla la velocidad de convergencia
         * @param blockSize Tamaño del bloque para procesamiento optimizado
         * 
         * @details El parámetro 'mu' (paso de adaptación) es crítico para el rendimiento:
         * - mu muy pequeño: convergencia lenta pero estable
         * - mu muy grande: convergencia rápida pero puede ser inestable
         * - Valores típicos: 0.001 - 0.1 (ajustar según la aplicación)
         * 
         * Para bioseñales, valores recomendados de mu:
         * - ECG: 0.01 - 0.05 (eliminación de deriva de línea base)
         * - EMG: 0.001 - 0.01 (cancelación de artefactos)
         * - EEG: 0.0001 - 0.001 (supresión de parpadeo)
         * 
         * @note Un filtro de orden N tiene N+1 coeficientes (numTaps = N+1)
         * 
         * @warning Los coeficientes serán modificados durante la operación del filtro.
         * Si necesitas preservar los valores originales, haz una copia antes de pasarlos.
         * 
         * @par Ejemplo
         * @code
         * // Filtro adaptativo de 64 coeficientes para cancelación de artefactos ECG
         * float32_t adaptiveCoeffs[64];
         * for(int i = 0; i < 64; i++) adaptiveCoeffs[i] = 0.0f; // Inicializar a cero
         * 
         * LMSFilter ecgArtifactCanceller(adaptiveCoeffs, 64, 0.02f, 1); // mu = 0.02 para ECG
         * @endcode
         */
        LMSFilter(float32_t* coeffs, uint16_t numTaps, float32_t mu, uint16_t blockSize);

        /**
         * @brief Destructor de la clase LMSFilter
         * 
         * Libera automáticamente la memoria asignada para el buffer de estados interno.
         * Los coeficientes no se liberan ya que son gestionados externamente.
         * 
         * @note No es necesario llamar explícitamente a funciones de limpieza.
         */
        ~LMSFilter();

        /**
         * @brief Procesa una muestra individual en tiempo real con adaptación
         * 
         * Aplica el filtro LMS adaptativo a una muestra de entrada usando la señal
         * de referencia proporcionada. Los coeficientes del filtro se actualizan
         * automáticamente para minimizar el error entre la salida deseada (referencia)
         * y la salida actual del filtro.
         * 
         * @param input Valor de la muestra de entrada (señal primaria a filtrar)
         * @param reference Valor de la muestra de referencia (señal deseada)
         * @param output Puntero donde escribir la muestra filtrada de salida
         * @param error Puntero donde escribir el error de adaptación (reference - output)
         * 
         * @details El procesamiento LMS realiza las siguientes operaciones:
         * 1. Calcula la salida del filtro: y[n] = Σ(w[k] * x[n-k])
         * 2. Calcula el error: e[n] = d[n] - y[n] (donde d[n] es la referencia)
         * 3. Actualiza los coeficientes: w[k] = w[k] + mu * e[n] * x[n-k]
         * 
         * @note La señal de error indica qué tan bien se está adaptando el filtro.
         * Un error pequeño indica buena adaptación.
         * 
         * @par Ejemplo
         * @code
         * // Cancelación adaptativa de interferencia de línea eléctrica
         * float32_t ecgSample = readECG();           // Señal ECG con interferencia
         * float32_t powerlineRef = sin(2*PI*60*t);   // Referencia de 60Hz
         * float32_t cleanECG, adaptiveError;
         * 
         * filter.processSample(ecgSample, powerlineRef, &cleanECG, &adaptiveError);
         * 
         * // cleanECG contiene la señal ECG con interferencia reducida
         * // adaptiveError indica la efectividad de la cancelación
         * @endcode
         */
        void processSample(float32_t input, float32_t reference, 
                          float32_t* output, float32_t* error);

        /**
         * @brief Procesa un buffer completo de muestras con adaptación
         * 
         * Aplica el filtro LMS adaptativo a un array completo de muestras, usando
         * las señales de referencia correspondientes. Optimizado para procesamiento
         * por lotes con mejor rendimiento que múltiples llamadas a processSample().
         * 
         * @param inputArray Puntero al array de muestras de entrada
         * @param referenceArray Puntero al array de muestras de referencia
         * @param outputArray Puntero al array donde escribir las muestras filtradas
         * @param errorArray Puntero al array donde escribir las señales de error
         * @param length Número de muestras a procesar
         * 
         * @details Esta función es optimizada para:
         * - Mayor throughput que el procesamiento muestra por muestra
         * - Uso eficiente de las optimizaciones SIMD de ARM
         * - Procesamiento de bloques grandes de datos adaptativos
         * 
         * @warning Todos los arrays deben tener al menos 'length' elementos válidos
         * y no deben solaparse en memoria.
         * 
         * @note Los coeficientes del filtro se actualizan continuamente durante
         * el procesamiento del bloque, por lo que el filtro se adapta incluso
         * dentro de un solo bloque de datos.
         * 
         * @par Ejemplo
         * @code
         * // Procesamiento adaptativo de un buffer de EEG para eliminar artefactos de parpadeo
         * float32_t eegBuffer[256];      // Señal EEG contaminada
         * float32_t blinkRefBuffer[256]; // Referencia de artefacto de parpadeo (EOG)
         * float32_t cleanEEG[256];       // EEG limpio (salida)
         * float32_t adaptError[256];     // Señal de error de adaptación
         * 
         * // Leer datos de los ADC
         * readEEGBuffer(eegBuffer, 256);
         * readEOGReference(blinkRefBuffer, 256);
         * 
         * // Aplicar filtro adaptativo
         * filter.processBuffer(eegBuffer, blinkRefBuffer, cleanEEG, adaptError, 256);
         * 
         * // Analizar la efectividad de la adaptación
         * float32_t meanError = calculateMean(adaptError, 256);
         * Serial.print("Error promedio de adaptación: ");
         * Serial.println(meanError);
         * @endcode
         */
        void processBuffer(const float32_t* inputArray, float32_t* referenceArray,
                          float32_t* outputArray, float32_t* errorArray, uint32_t length);

        /**
         * @brief Obtiene el valor actual del paso de adaptación
         * 
         * @return float32_t Valor actual de mu (paso de adaptación)
         * 
         * @note Útil para monitorear o ajustar dinámicamente la velocidad de adaptación
         */
        float32_t getMu() const { return _mu; }

        /**
         * @brief Modifica el paso de adaptación durante la operación
         * 
         * Permite ajustar dinámicamente la velocidad de convergencia del filtro
         * sin reinicializar toda la estructura.
         * 
         * @param newMu Nuevo valor del paso de adaptación
         * 
         * @note Cambiar mu durante la operación puede ser útil para implementar
         * esquemas de adaptación variables o para optimizar la convergencia
         * 
         * @warning Valores de mu demasiado grandes pueden causar inestabilidad
         */
        void setMu(float32_t newMu);

        /**
         * @brief Reinicia los coeficientes adaptativos a valores iniciales
         * 
         * @param newCoeffs Puntero a los nuevos coeficientes iniciales (opcional, si es NULL usa ceros)
         * 
         * @details Útil para:
         * - Reiniciar la adaptación cuando cambian las condiciones
         * - Implementar esquemas de adaptación por bloques
         * - Recuperarse de una mala convergencia
         * 
         * @note El buffer de estados también se reinicia a cero
         */
        void resetCoefficients(const float32_t* newCoeffs = nullptr);

    private:
        /**
         * @brief Puntero a los coeficientes adaptativos del filtro LMS
         * 
         * Los coeficientes se actualizan automáticamente durante el procesamiento
         * usando el algoritmo LMS para minimizar el error cuadrático medio.
         * 
         * @note Esta es una referencia al array externo que se modifica dinámicamente.
         */
        float32_t* _coeffs;

        /**
         * @brief Buffer de estados interno del filtro
         * 
         * Almacena el historial de muestras de entrada necesario para el cálculo
         * de la convolución adaptativa y la actualización de coeficientes.
         * El tamaño del buffer es numTaps elementos.
         * 
         * @details El buffer de estados contiene:
         * - Muestras de entrada previas para la convolución
         * - Estados necesarios para el algoritmo de actualización LMS
         * 
         * @note La memoria se asigna dinámicamente en el constructor y se libera en el destructor.
         */
        float32_t* _state;

        /**
         * @brief Número de coeficientes del filtro adaptativo (orden + 1)
         * 
         * Define el orden del filtro LMS. Un mayor número de coeficientes permite:
         * - Mayor capacidad de adaptación a señales complejas
         * - Mejor identificación de sistemas de orden alto
         * - Mayor carga computacional y memoria requerida
         * 
         * @note Típicamente entre 10-128 coeficientes para aplicaciones de bioseñales.
         */
        uint16_t _numTaps;

        /**
         * @brief Paso de adaptación (step size) del algoritmo LMS
         * 
         * Controla la velocidad de convergencia y estabilidad del filtro adaptativo:
         * - Valores pequeños: convergencia lenta pero estable
         * - Valores grandes: convergencia rápida pero posible inestabilidad
         * 
         * @note Rango típico: 0.0001 - 0.1, dependiendo de la aplicación y nivel de señal.
         */
        float32_t _mu;

        /**
         * @brief Tamaño del bloque para procesamiento optimizado
         * 
         * Determina el tamaño de bloque usado por CMSIS-DSP para optimizaciones internas.
         * Valores típicos: 1 (tiempo real), 16, 32, 64 (procesamiento por lotes).
         * 
         * @note Bloques más grandes pueden ofrecer mejor rendimiento pero mayor latencia.
         */
        uint16_t _blockSize;

        /**
         * @brief Instancia del filtro LMS de CMSIS-DSP
         * 
         * Estructura interna utilizada por las funciones arm_lms_* de CMSIS-DSP.
         * Contiene toda la información de configuración y estado del filtro adaptativo.
         * 
         * @note No debe ser modificada directamente, se gestiona a través de las
         * funciones de CMSIS-DSP.
         */
        arm_lms_instance_f32 _lmsInstance;

}; // class LMSFilter

#endif // LMS_FILTER_H

/**
 * @note Consideraciones adicionales para el uso de filtros LMS en bioseñales:
 * 
 * 1. SELECCIÓN DEL PASO DE ADAPTACIÓN (MU):
 *    - Para ECG: mu = 0.01-0.05 (eliminación de deriva, cancelación de 50/60Hz)
 *    - Para EMG: mu = 0.001-0.01 (supresión de artefactos de movimiento)
 *    - Para EEG: mu = 0.0001-0.001 (cancelación de parpadeo, interferencia)
 * 
 * 2. NÚMERO DE COEFICIENTES:
 *    - Artefactos de baja frecuencia: 32-64 taps
 *    - Interferencia de línea eléctrica: 16-32 taps
 *    - Cancelación de eco/reflexiones: 64-128 taps
 * 
 * 3. SEÑALES DE REFERENCIA:
 *    - Interferencia de 50/60Hz: señal sinusoidal de referencia
 *    - Artefactos de parpadeo (EEG): canal EOG como referencia
 *    - Artefactos respiratorios (ECG): sensor de respiración
 *    - Artefactos de movimiento: acelerómetro o giroscopio
 * 
 * 4. CRITERIOS DE CONVERGENCIA:
 *    - Monitorear la señal de error para evaluar la adaptación
 *    - Error decreciente indica buena convergencia
 *    - Error oscilante puede indicar mu demasiado grande
 * 
 * 5. APLICACIONES TÍPICAS EN BIOSEÑALES:
 *    - Cancelación adaptativa de interferencia electromagnética
 *    - Eliminación de artefactos de movimiento en tiempo real
 *    - Supresión de ruido con características cambiantes
 *    - Identificación de sistemas fisiológicos (función de transferencia cardíaca, etc.)
 */