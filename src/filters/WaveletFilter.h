/**
 * @file WaveletFilter.h
 * @brief Filtro Wavelet de banco de filtros optimizado para bioseñales usando CMSIS-DSP
 * @author Sergio
 * @version 1.0.0
 * @date 2025
 * 
 * @details Este archivo contiene la implementación de un wrapper C++ para filtros wavelet
 * implementados como banco de filtros usando CMSIS-DSP, diseñado específicamente para el 
 * procesamiento de bioseñales en Arduino Due. La clase WaveletFilter facilita el análisis 
 * tiempo-frecuencia manteniendo las optimizaciones de hardware que ofrece CMSIS-DSP para 
 * procesadores ARM Cortex-M.
 * 
 * @note Requiere CMSIS-DSP y está optimizado para Arduino Due (ARM Cortex-M3)
 * 
 * @example
 * @code
 * // Crear filtro wavelet Daubechies-4 para análisis de ECG
 * // Procesamiento en bloques de 32 muestras para tiempo real
 * WaveletFilter ecgWavelet(32);
 * 
 * // Procesar una muestra individual y obtener coeficientes de aproximación y detalle
 * float32_t approxCoeff, detailCoeff;
 * ecgWavelet.processSample(inputSample, &approxCoeff, &detailCoeff);
 * 
 * // Procesar un buffer completo
 * ecgWavelet.processBuffer(inputBuffer, approxBuffer, detailBuffer, bufferSize);
 * 
 * // Reconstruir señal filtrada (solo aproximación para suavizado)
 * float32_t reconstructed = ecgWavelet.reconstruct(approxCoeff, 0.0f);
 * @endcode
 */

#ifndef WAVELET_FILTER_H
#define WAVELET_FILTER_H
#define ARM_MATH_CM3

#include <Arduino.h>
#include <BioFilterLib.h>  // CMSIS-DSP

class FIRFilter;  // Forward declaration

/**
 * @class WaveletFilter
 * @brief Wrapper C++ para filtros wavelet implementados como banco de filtros usando CMSIS-DSP
 * 
 * Esta clase encapsula la funcionalidad de análisis wavelet usando un banco de filtros 
 * de pasa-bajo (aproximación) y pasa-alto (detalle), proporcionando una interfaz amigable 
 * para el análisis tiempo-frecuencia de señales biomédicas como ECG, EMG, EEG, etc.
 * 
 * @details Los filtros wavelet son especialmente útiles para:
 * - Análisis tiempo-frecuencia de señales no estacionarias (variabilidad de ECG)
 * - Detección de eventos transitorios (complejos QRS, artefactos de movimiento)
 * - Supresión selectiva de ruido preservando características importantes
 * - Análisis multiescala de señales biomédicas
 * - Compresión de datos con preservación de características clínicas
 * 
 * La implementación utiliza wavelets Daubechies-4, que han demostrado ser óptimas
 * para el procesamiento de bioseñales debido a su:
 * - Buena localización temporal (detección de transitorios)
 * - Respuesta de fase casi lineal (preservación de morfología)
 * - Soporte compacto (eficiencia computacional)
 * - Ortogonalidad (reconstrucción perfecta)
 * 
 * La clase maneja automáticamente:
 * - Inicialización de los filtros FIR de aproximación y detalle
 * - Gestión de memoria para los buffers de estados internos
 * - Interfaz con las funciones optimizadas de CMSIS-DSP
 * - Procesamiento tanto de muestras individuales como de buffers
 * - Reconstrucción de señales a partir de coeficientes wavelet
 * 
 * @note Los coeficientes wavelet Daubechies-4 están predefinidos e integrados en la clase.
 * Para otros tipos de wavelet, se requerirían modificaciones en los coeficientes.
 * 
 * @warning El filtro debe ser inicializado antes del primer uso. El procesamiento
 * en tiempo real requiere llamadas regulares para mantener la continuidad del estado.
 * 
 * @see FIRFilter para detalles sobre la implementación subyacente de filtros FIR
 */
class WaveletFilter {
    public:
        /**
         * @brief Constructor que inicializa el filtro wavelet Daubechies-4
         * 
         * Inicializa un banco de filtros wavelet usando coeficientes Daubechies-4
         * predefinidos, optimizados para el análisis de bioseñales.
         * 
         * @param blockSize Tamaño del bloque para procesamiento optimizado
         * 
         * @details El constructor:
         * 1. Inicializa los coeficientes Daubechies-4 para los filtros FIR
         * 2. Crea instancias de FIRFilter para aproximación (pasa-bajo) y detalle (pasa-alto)
         * 3. Configura el procesamiento para el tamaño de bloque especificado
         * 4. Inicializa los estados internos para procesamiento continuo
         * 
         * Los coeficientes Daubechies-4 proporcionan:
         * - 8 coeficientes para el filtro de aproximación (pasa-bajo)
         * - 8 coeficientes para el filtro de detalle (pasa-alto)
         * - Reconstrucción perfecta cuando se usan juntos
         * - Óptima respuesta para frecuencias típicas de bioseñales (0.1-100 Hz)
         * 
         * @note Para aplicaciones en tiempo real, usar blockSize = 1.
         * Para procesamiento por lotes, usar potencias de 2 (32, 64, 128).
         * 
         * @example
         * @code
         * // Filtro para análisis en tiempo real de ECG
         * WaveletFilter realTimeECG(1);
         * 
         * // Filtro para procesamiento por lotes de EMG
         * WaveletFilter batchEMG(64);
         * @endcode
         */
        WaveletFilter(uint16_t blockSize);

        /**
         * @brief Destructor que libera los recursos asignados
         * 
         * Libera automáticamente la memoria de los filtros FIR internos y
         * limpia todos los recursos utilizados por el filtro wavelet.
         * 
         * @note El destructor maneja automáticamente la limpieza de los objetos
         * FIRFilter internos, no es necesario llamar funciones de limpieza.
         */
        ~WaveletFilter();

        /**
         * @brief Procesa una muestra individual obteniendo coeficientes wavelet
         * 
         * Aplica la descomposición wavelet a una sola muestra de entrada,
         * calculando los coeficientes de aproximación (pasa-bajo) y detalle (pasa-alto).
         * 
         * @param input Muestra de entrada a procesar
         * @param approxCoeff Puntero donde escribir el coeficiente de aproximación
         * @param detailCoeff Puntero donde escribir el coeficiente de detalle
         * 
         * @details Esta función:
         * 1. Aplica el filtro de aproximación (pasa-bajo) a la muestra
         * 2. Aplica el filtro de detalle (pasa-alto) a la misma muestra
         * 3. Almacena los resultados en las variables apuntadas
         * 4. Mantiene el estado interno para continuidad en el procesamiento
         * 
         * Los coeficientes resultantes representan:
         * - **Aproximación**: Componentes de baja frecuencia (tendencia, forma base)
         * - **Detalle**: Componentes de alta frecuencia (bordes, transitorios, ruido)
         * 
         * @note Para supresión de ruido, típicamente se usa solo la aproximación.
         * Para detección de eventos, se analiza principalmente el detalle.
         * 
         * @example
         * @code
         * float32_t approx, detail;
         * waveletFilter.processSample(ecgSample, &approx, &detail);
         * 
         * // Señal suavizada (sin ruido de alta frecuencia)
         * float32_t denoised = approx;
         * 
         * // Detectar picos QRS analizando el detalle
         * if (abs(detail) > threshold) {
         *     // Posible complejo QRS detectado
         * }
         * @endcode
         */
        void processSample(float32_t input, float32_t* approxCoeff, float32_t* detailCoeff);

        /**
         * @brief Procesa un buffer completo obteniendo coeficientes wavelet
         * 
         * Aplica la descomposición wavelet a un array completo de muestras,
         * generando arrays de coeficientes de aproximación y detalle.
         * 
         * @param inputArray Puntero al array de muestras de entrada
         * @param approxArray Puntero al array donde escribir coeficientes de aproximación
         * @param detailArray Puntero al array donde escribir coeficientes de detalle
         * @param length Número de muestras a procesar
         * 
         * @details Optimizada para procesamiento por lotes con:
         * 1. Mejor rendimiento que múltiples llamadas a processSample()
         * 2. Uso eficiente de optimizaciones SIMD de CMSIS-DSP
         * 3. Mantenimiento de continuidad del estado entre bloques
         * 4. Procesamiento vectorizado cuando es posible
         * 
         * @note Los arrays de salida deben tener al menos 'length' elementos
         * asignados antes de la llamada a esta función.
         * 
         * @warning Los arrays de entrada y salida no deben solaparse en memoria
         * para evitar corrupción de datos durante el procesamiento.
         * 
         * @example
         * @code
         * const uint32_t bufferSize = 256;
         * float32_t ecgBuffer[bufferSize];
         * float32_t approxBuffer[bufferSize];
         * float32_t detailBuffer[bufferSize];
         * 
         * // Procesar bloque completo
         * waveletFilter.processBuffer(ecgBuffer, approxBuffer, detailBuffer, bufferSize);
         * 
         * // Analizar resultados
         * for (uint32_t i = 0; i < bufferSize; i++) {
         *     // approxBuffer[i] contiene componentes de baja frecuencia
         *     // detailBuffer[i] contiene componentes de alta frecuencia
         * }
         * @endcode
         */
        void processBuffer(float32_t* inputArray, float32_t* approxArray, 
                          float32_t* detailArray, uint32_t length);

        /**
         * @brief Reconstruye una muestra a partir de coeficientes wavelet
         * 
         * Realiza la síntesis wavelet combinando los coeficientes de aproximación
         * y detalle para reconstruir la señal original o una versión filtrada.
         * 
         * @param approxCoeff Coeficiente de aproximación (componente de baja frecuencia)
         * @param detailCoeff Coeficiente de detalle (componente de alta frecuencia)
         * @return float32_t Muestra reconstruida
         * 
         * @details Esta función permite:
         * - **Reconstrucción completa**: Usar ambos coeficientes (señal original)
         * - **Suavizado**: Usar solo aproximación, detalle = 0 (supresión de ruido)
         * - **Detección de bordes**: Usar solo detalle, aproximación = 0
         * - **Filtrado selectivo**: Modificar coeficientes antes de reconstruir
         * 
         * La reconstrucción se basa en la propiedad de reconstrucción perfecta
         * de las wavelets ortogonales Daubechies-4, garantizando que:
         * reconstruct(approx, detail) ≈ señal_original (dentro de precisión numérica)
         * 
         * @note Para reconstrucción perfecta, se deben usar todos los coeficientes
         * sin modificación. Para filtrado, modificar selectivamente los coeficientes.
         * 
         * @example
         * @code
         * float32_t approx, detail;
         * waveletFilter.processSample(noisyECG, &approx, &detail);
         * 
         * // Reconstrucción completa (señal original)
         * float32_t original = waveletFilter.reconstruct(approx, detail);
         * 
         * // Solo aproximación (señal suavizada)
         * float32_t denoised = waveletFilter.reconstruct(approx, 0.0f);
         * 
         * // Filtrado adaptativo del detalle
         * float32_t filteredDetail = (abs(detail) > threshold) ? detail : 0.0f;
         * float32_t adaptiveFiltered = waveletFilter.reconstruct(approx, filteredDetail);
         * @endcode
         */
        float32_t reconstruct(float32_t approxCoeff, float32_t detailCoeff);

        /**
         * @brief Reinicia el estado interno del filtro wavelet
         * 
         * Limpia todos los estados internos de los filtros de aproximación y detalle,
         * reseteando el filtro wavelet a su estado inicial. Útil para procesar
         * nuevas secuencias de datos independientes.
         * 
         * @details Esta función:
         * 1. Reinicia los buffers de estados de ambos filtros FIR internos
         * 2. Limpia cualquier información de muestras previas
         * 3. Prepara el filtro para procesar una nueva secuencia de datos
         * 4. No afecta los coeficientes wavelet (solo el estado)
         * 
         * @note Especialmente útil cuando se cambia de un paciente a otro
         * o cuando se inicia el procesamiento de una nueva sesión de datos.
         * 
         * @example
         * @code
         * // Procesar datos del primer paciente
         * for (int i = 0; i < patient1DataLength; i++) {
         *     waveletFilter.processSample(patient1Data[i], &approx, &detail);
         * }
         * 
         * // Cambiar a segundo paciente - reiniciar estado
         * waveletFilter.reset();
         * 
         * // Procesar datos del segundo paciente con estado limpio
         * for (int i = 0; i < patient2DataLength; i++) {
         *     waveletFilter.processSample(patient2Data[i], &approx, &detail);
         * }
         * @endcode
         */
        void reset();

    private:
        /**
         * @brief Coeficientes del filtro de aproximación Daubechies-4 (pasa-bajo)
         * 
         * Coeficientes normalizados para el filtro FIR de aproximación que extrae
         * las componentes de baja frecuencia de la señal (hasta ~Fs/4).
         * 
         * @details Estos coeficientes implementan:
         * - Respuesta de pasa-bajo con frecuencia de corte aproximada en Fs/4
         * - 8 coeficientes (orden 7) para Daubechies-4
         * - Respuesta optimizada para señales biomédicas
         * - Reconstrucción perfecta cuando se combina con filtro de detalle
         */
        static float32_t _approxCoeffs[8];

        /**
         * @brief Coeficientes del filtro de detalle Daubechies-4 (pasa-alto)
         * 
         * Coeficientes normalizados para el filtro FIR de detalle que extrae
         * las componentes de alta frecuencia de la señal (Fs/4 a Fs/2).
         * 
         * @details Estos coeficientes implementan:
         * - Respuesta de pasa-alto complementaria al filtro de aproximación
         * - 8 coeficientes (orden 7) derivados de Daubechies-4
         * - Ortogonalidad con el filtro de aproximación
         * - Detección óptima de transitorios y bordes
         */
        static float32_t _detailCoeffs[8];

        /**
         * @brief Coeficientes para reconstrucción de aproximación
         * 
         * Coeficientes del filtro FIR usado en la síntesis para reconstruir
         * la componente de aproximación durante la reconstrucción wavelet.
         */
        static float32_t _synthApproxCoeffs[8];

        /**
         * @brief Coeficientes para reconstrucción de detalle
         * 
         * Coeficientes del filtro FIR usado en la síntesis para reconstruir
         * la componente de detalle durante la reconstrucción wavelet.
         */
        static float32_t _synthDetailCoeffs[8];

        /**
         * @brief Instancia del filtro FIR de aproximación
         * 
         * Filtro FIR de pasa-bajo que implementa la extracción de coeficientes
         * de aproximación. Utiliza los coeficientes Daubechies-4 de pasa-bajo.
         */
        FIRFilter* _approxFilter;

        /**
         * @brief Instancia del filtro FIR de detalle
         * 
         * Filtro FIR de pasa-alto que implementa la extracción de coeficientes
         * de detalle. Utiliza los coeficientes Daubechies-4 de pasa-alto.
         */
        FIRFilter* _detailFilter;

        /**
         * @brief Instancia del filtro FIR para síntesis de aproximación
         * 
         * Filtro FIR usado durante la reconstrucción para procesar los
         * coeficientes de aproximación en la síntesis wavelet.
         */
        FIRFilter* _synthApproxFilter;

        /**
         * @brief Instancia del filtro FIR para síntesis de detalle
         * 
         * Filtro FIR usado durante la reconstrucción para procesar los
         * coeficientes de detalle en la síntesis wavelet.
         */
        FIRFilter* _synthDetailFilter;

        /**
         * @brief Tamaño del bloque para procesamiento optimizado
         * 
         * Define el tamaño de bloque usado internamente por los filtros FIR
         * para optimizaciones de CMSIS-DSP. Debe coincidir con el usado
         * en la inicialización de los filtros.
         */
        uint16_t _blockSize;

        /**
         * @brief Número de coeficientes por filtro (siempre 8 para Daubechies-4)
         * 
         * Define el orden de los filtros wavelet. Daubechies-4 usa 8 coeficientes
         * para cada filtro (aproximación y detalle).
         */
        static const uint16_t _numTaps = 8;
        
}; // class WaveletFilter

#endif // WAVELET_FILTER_H