/**
 * @file IIRFilter.h
 * @brief Filtro IIR de tipo Biquad en cascada, optimizado para bioseñales usando CMSIS-DSP
 * @author Sergio
 * @version 1.0.0
 * @date 2025
 * * @details Este archivo contiene la implementación de un wrapper C++ para los filtros IIR
 * de CMSIS-DSP, diseñado para el procesamiento eficiente de bioseñales en Arduino Due.
 * La clase IIRFilter facilita el uso de filtros IIR (Infinite Impulse Response)
 * manteniendo las optimizaciones de hardware de CMSIS-DSP.
 * * @note Requiere CMSIS-DSP y está optimizado para Arduino Due (ARM Cortex-M3)
 * * @example
 * @code
 * // Coeficientes para un filtro pasa-altas Butterworth de 2do orden a 1 Hz (Fs=1000Hz)
 * // Generados con Python/scipy: b, a = signal.butter(2, 1, 'highpass', fs=1000)
 * // Formato para CMSIS Biquad: {b0, b1, b2, a1, a2} (a0=1 se omite)
 * const float32_t highpassCoeffs[5] = {
 * 0.99376249f, -1.98752499f,  0.99376249f, // b0, b1, b2
 * -1.98745615f,  0.98759382f                 // a1, a2
 * };
 * * // Crear filtro IIR con 1 sección Biquad, para procesamiento en tiempo real
 * IIRFilter filter(highpassCoeffs, 1, 1);
 * * // Procesar una muestra individual
 * float32_t filteredSample = filter.processSample(inputSample);
 * * // Procesar un buffer completo
 * filter.processBuffer(inputBuffer, outputBuffer, bufferSize);
 * @endcode
 */

#ifndef IIR_FILTER_H
#define IIR_FILTER_H

#include <Arduino.h>
#include <arm_math.h> // CMSIS-DSP

/**
 * @class IIRFilter
 * @brief Wrapper C++ para filtros IIR Biquad de CMSIS-DSP, optimizado para bioseñales
 * * Esta clase encapsula la funcionalidad de filtros IIR (Infinite Impulse Response)
 * de CMSIS-DSP, usando una estructura de cascada de secciones de segundo orden (Biquad).
 * Esta implementación es ideal para filtros con respuestas de frecuencia muy selectivas
 * que serían computacionalmente costosos si se implementaran como FIR.
 * * @details Los filtros IIR son especialmente útiles para:
 * - Filtros de alta selectividad (p. ej. notch muy agudo) con bajo orden.
 * - Emulación de filtros analógicos clásicos (Butterworth, Chebyshev).
 * - Menor carga computacional y memoria que un filtro FIR de rendimiento similar.
 * * A diferencia de los filtros FIR, los IIR tienen una respuesta de fase no lineal,
 * lo que puede distorsionar la forma de onda. Esto debe considerarse en aplicaciones
 * donde la morfología de la señal es crítica (p. ej. análisis del complejo QRS en ECG).
 * * La clase maneja automáticamente:
 * - Inicialización del estado interno del filtro.
 * - Gestión de memoria para el buffer de estados.
 * - Interfaz con las funciones Biquad optimizadas de CMSIS-DSP.
 * * @note Los coeficientes deben ser calculados externamente (ej. Python/scipy, MATLAB)
 * y formateados para la estructura Biquad de CMSIS-DSP. Cada sección Biquad
 * requiere 5 coeficientes: {b0, b1, b2, a1, a2}. El coeficiente a0 se asume como 1.
 * * @warning La memoria para los coeficientes debe permanecer válida durante toda la vida
 * del objeto IIRFilter.
 * * @see arm_biquad_cascade_df1_f32() para detalles de la implementación subyacente.
 */
class IIRFilter {
    public:
        /**
         * @brief Constructor de la clase IIRFilter
         * * Inicializa un filtro IIR en cascada de Biquads.
         * * @param coeffs Puntero al array de coeficientes del filtro. El array debe contener
         * numStages * 5 coeficientes en el orden {b0, b1, b2, a1, a2} para
         * cada sección.
         * @param numStages Número de secciones Biquad de segundo orden en cascada.
         * Un filtro de orden N requiere N/2 etapas.
         * @param blockSize Tamaño del bloque para procesamiento optimizado (usualmente 1 para tiempo real).
         * * @example
         * @code
         * // Filtro Notch de 4to orden (2 etapas Biquad) para eliminar 60 Hz
         * const float32_t notchCoeffs[10] = {
         * // Coeficientes Etapa 1
         * b0_1, b1_1, b2_1, a1_1, a2_1,
         * // Coeficientes Etapa 2
         * b0_2, b1_2, b2_2, a1_2, a2_2
         * };
         * IIRFilter notchFilter(notchCoeffs, 2, 1); // 2 etapas, tiempo real
         * @endcode
         */
        IIRFilter(const float32_t* coeffs, uint8_t numStages, uint16_t blockSize);

        /**
         * @brief Destructor de la clase IIRFilter
         * * Libera la memoria asignada para el buffer de estados interno.
         */
        ~IIRFilter();

        /**
         * @brief Procesa una muestra individual en tiempo real
         * * Aplica el filtro IIR a una sola muestra de entrada. Ideal para aplicaciones
         * de baja latencia.
         * * @param input Valor de la muestra de entrada.
         * @return float32_t Muestra filtrada de salida.
         */
        float32_t processSample(float32_t input);

        /**
         * @brief Procesa un buffer completo de muestras
         * * Aplica el filtro IIR a un array de muestras. Optimizado para
         * mayor throughput en procesamiento por lotes.
         * * @param inputArray Puntero al array de muestras de entrada.
         * @param outputArray Puntero al array donde se escribirán las muestras filtradas.
         * @param length Número de muestras a procesar.
         * * @warning Los arrays de entrada y salida no deben solaparse en memoria.
         */
        void processBuffer(const float32_t* inputArray, float32_t* outputArray, uint32_t length);

    private:
        /**
         * @brief Puntero a los coeficientes del filtro IIR.
         * * Referencia constante al array de coeficientes, formateado para CMSIS-DSP Biquad.
         * La memoria es gestionada externamente.
         */
        const float32_t* _coeffs;

        /**
         * @brief Buffer de estados interno del filtro.
         * * Almacena el historial de muestras de entrada y salida (x[n-1], x[n-2], y[n-1], y[n-2])
         * para cada sección Biquad. El tamaño es 4 * numStages.
         * La memoria se asigna dinámicamente.
         */
        float32_t* _state;

        /**
         * @brief Número de secciones Biquad en cascada.
         */
        uint8_t _numStages;

        /**
         * @brief Tamaño del bloque para procesamiento optimizado.
         */
        uint16_t _blockSize;
        
        /**
         * @brief Instancia del filtro IIR Biquad de CMSIS-DSP.
         * * Estructura interna utilizada por las funciones arm_biquad_cascade_* de CMSIS-DSP
         * para gestionar la configuración y el estado del filtro.
         */
        arm_biquad_casd_df1_inst_f32 _iirInstance;

}; // class IIRFilter

#endif // IIR_FILTER_H