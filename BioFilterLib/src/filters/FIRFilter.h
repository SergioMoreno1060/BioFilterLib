/**
 * @file FIRFilter.h
 * @brief Filtro FIR optimizado para bioseñales usando CMSIS-DSP
 * @author Sergio
 * @version 1.0.0
 * @date 2024
 * 
 * @details Este archivo contiene la implementación de un wrapper C++ para los filtros FIR
 * de CMSIS-DSP, diseñado específicamente para el procesamiento de bioseñales en Arduino Due.
 * La clase FIRFilter facilita el uso de filtros FIR manteniendo las optimizaciones de hardware
 * que ofrece CMSIS-DSP para procesadores ARM Cortex-M.
 * 
 * @note Requiere CMSIS-DSP y está optimizado para Arduino Due (ARM Cortex-M3)
 * 
 * @example
 * @code
 * // Coeficientes de un filtro pasa-bajas (ejemplo)
 * const float32_t lowpassCoeffs[51] = { ... };
 * 
 * // Crear filtro con 51 coeficientes, procesamiento en bloques de 32 muestras
 * FIRFilter filter(lowpassCoeffs, 51, 32);
 * 
 * // Procesar una muestra individual
 * float32_t filteredSample = filter.processSample(inputSample);
 * 
 * // Procesar un buffer completo
 * filter.processBuffer(inputBuffer, outputBuffer, bufferSize);
 * @endcode
 */

#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <Arduino.h>
#include <arm_math.h>  // CMSIS-DSP

/**
 * @class FIRFilter
 * @brief Wrapper C++ para filtros FIR de CMSIS-DSP optimizado para bioseñales
 * 
 * Esta clase encapsula la funcionalidad de filtros FIR (Finite Impulse Response) de CMSIS-DSP,
 * proporcionando una interfaz amigable para el procesamiento de señales biomédicas como ECG, EMG, EEG, etc.
 * 
 * @details Los filtros FIR son especialmente útiles para:
 * - Eliminación de ruido de alta frecuencia (filtros pasa-bajas)
 * - Eliminación de interferencia de línea eléctrica (filtros notch)
 * - Separación de bandas de frecuencia específicas (filtros pasa-banda)
 * - Preservación de la forma de onda original (respuesta de fase lineal)
 * 
 * La clase maneja automáticamente:
 * - Inicialización del estado interno del filtro
 * - Gestión de memoria para el buffer de estados
 * - Interfaz con las funciones optimizadas de CMSIS-DSP
 * - Procesamiento tanto de muestras individuales como de buffers
 * 
 * @note Los coeficientes del filtro deben ser calculados externamente usando herramientas
 * como Python/scipy, MATLAB o herramientas online de diseño de filtros.
 * 
 * @warning La memoria para los coeficientes debe mantenerse válida durante toda la vida
 * del objeto FIRFilter, ya que se almacena solo la referencia.
 * 
 * @see arm_fir_f32() para más detalles sobre la implementación subyacente de CMSIS-DSP
 */
class FIRFilter {
    public:
        /**
         * @brief Constructor de la clase FIRFilter
         * 
         * Inicializa un filtro FIR con los coeficientes especificados y configura
         * el procesamiento para el tamaño de bloque dado.
         * 
         * @param coeffs Puntero al array de coeficientes del filtro (debe permanecer válido)
         * @param numTaps Número de coeficientes del filtro (orden + 1)
         * @param blockSize Tamaño del bloque para procesamiento optimizado
         * 
         * @details El parámetro blockSize determina cómo se procesan las muestras:
         * - blockSize = 1: Optimizado para procesamiento muestra por muestra (tiempo real)
         * - blockSize > 1: Optimizado para procesamiento por lotes (mayor throughput)
         * 
         * @note Un filtro de orden N tiene N+1 coeficientes (numTaps = N+1)
         * 
         * @warning Los coeficientes deben estar normalizados apropiadamente para evitar
         * saturación en el procesamiento de punto fijo.
         * 
         * @example
         * @code
         * // Filtro pasa-bajas de 50 coeficientes para ECG
         * const float32_t ecgCoeffs[51] = { ... };
         * FIRFilter ecgFilter(ecgCoeffs, 51, 1);  // Tiempo real
         * @endcode
         */
        FIRFilter(const float32_t* coeffs, uint16_t numTaps, uint16_t blockSize);

        /**
         * @brief Destructor de la clase FIRFilter
         * 
         * Libera automáticamente la memoria asignada para el buffer de estados interno.
         * El destructor se encarga de limpiar todos los recursos utilizados por el filtro.
         * 
         * @note No es necesario llamar explícitamente a funciones de limpieza.
         */
        ~FIRFilter();

        /**
         * @brief Procesa una muestra individual en tiempo real
         * 
         * Aplica el filtro FIR a una sola muestra de entrada, manteniendo el estado
         * interno para procesamiento continuo. Ideal para aplicaciones en tiempo real.
         * 
         * @param input Valor de la muestra de entrada (típicamente normalizada entre -1.0 y 1.0)
         * @return float32_t Muestra filtrada de salida
         * 
         * @details Esta función es optimizada para:
         * - Procesamiento en tiempo real muestra por muestra
         * - Latencia mínima (una muestra de retraso por el filtro)
         * - Mantenimiento automático del estado interno
         * 
         * @note La función mantiene internamente el historial de muestras necesario
         * para el cálculo de la convolución FIR.
         * 
         * @example
         * @code
         * // Procesar señal ECG muestra por muestra
         * while (adcAvailable()) {
         *     float32_t ecgSample = readADC() / 2048.0f;  // Normalizar
         *     float32_t filtered = filter.processSample(ecgSample);
         *     outputDAC(filtered * 2048.0f);  // Escalar para salida
         * }
         * @endcode
         */
        float32_t processSample(float32_t input);

        /**
         * @brief Procesa un buffer completo de muestras
         * 
         * Aplica el filtro FIR a un array de muestras de entrada, escribiendo
         * el resultado en el array de salida. Optimizado para procesamiento por lotes.
         * 
         * @param inputArray Puntero al array de muestras de entrada
         * @param outputArray Puntero al array donde escribir las muestras filtradas
         * @param length Número de muestras a procesar
         * 
         * @details Esta función es optimizada para:
         * - Mayor throughput que el procesamiento muestra por muestra
         * - Uso eficiente de las optimizaciones SIMD de ARM
         * - Procesamiento de bloques grandes de datos
         * 
         * @warning Los arrays de entrada y salida deben tener al menos 'length' elementos
         * y no deben solaparse en memoria.
         * 
         * @note El estado interno del filtro se mantiene entre llamadas consecutivas,
         * permitiendo procesar streams de datos en múltiples bloques.
         * 
         * @example
         * @code
         * // Procesar buffer de 512 muestras de EMG
         * float32_t inputBuffer[512];
         * float32_t outputBuffer[512];
         * 
         * // Llenar inputBuffer con datos del ADC
         * readADCBuffer(inputBuffer, 512);
         * 
         * // Aplicar filtro
         * filter.processBuffer(inputBuffer, outputBuffer, 512);
         * 
         * // Usar outputBuffer filtrado
         * processEMGData(outputBuffer, 512);
         * @endcode
         */
        void processBuffer(const float32_t* inputArray, float32_t* outputArray, uint32_t length);

    private:
        /**
         * @brief Puntero a los coeficientes del filtro FIR
         * 
         * Referencia constante al array de coeficientes proporcionado en el constructor.
         * Los coeficientes definen la respuesta de frecuencia del filtro.
         * 
         * @note Esta es solo una referencia, la memoria debe ser gestionada externamente.
         */
        const float32_t* _coeffs;

        /**
         * @brief Buffer de estados interno del filtro
         * 
         * Almacena el historial de muestras necesario para el cálculo de la convolución FIR.
         * El tamaño del buffer es (numTaps + blockSize - 1) elementos.
         * 
         * @details El buffer de estados contiene:
         * - Muestras de entrada previas necesarias para la convolución
         * - Estados intermedios para procesamiento optimizado por CMSIS-DSP
         * 
         * @note La memoria se asigna dinámicamente en el constructor y se libera en el destructor.
         */
        float32_t* _state;

        /**
         * @brief Número de coeficientes del filtro (orden + 1)
         * 
         * Define el orden del filtro FIR. Un mayor número de coeficientes permite:
         * - Mayor selectividad en frecuencia
         * - Mejor atenuación en la banda de rechazo
         * - Mayor carga computacional
         * 
         * @note Típicamente entre 10-100 coeficientes para filtros de bioseñales.
         */
        uint16_t _numTaps;

        /**
         * @brief Tamaño del bloque para procesamiento optimizado
         * 
         * Determina el tamaño de bloque usado por CMSIS-DSP para optimizaciones internas.
         * Valores típicos: 1 (tiempo real), 32, 64, 128 (procesamiento por lotes).
         * 
         * @note Bloques más grandes pueden ofrecer mejor rendimiento pero mayor latencia.
         */
        uint16_t _blockSize;

        /**
         * @brief Instancia del filtro FIR de CMSIS-DSP
         * 
         * Estructura interna utilizada por las funciones arm_fir_* de CMSIS-DSP.
         * Contiene toda la información de configuración y estado del filtro.
         * 
         * @note No debe ser modificada directamente, se gestiona a través de las
         * funciones de CMSIS-DSP.
         */
        arm_fir_instance_f32 _firInstance;

        /**
         * @brief Índice de muestra para procesamiento en tiempo real
         * 
         * Mantiene el seguimiento de la posición actual en el procesamiento
         * secuencial de muestras individuales.
         * 
         * @note Usado internamente para optimizaciones de acceso al buffer de estados.
         */
        uint32_t _sampleIndex;
        
}; // class FIRFilter

#endif // FIR_FILTER_H