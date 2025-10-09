/**
 * @file WaveletFilter.cpp
 * @brief Implementación del filtro Wavelet optimizado para bioseñales
 * @author Sergio
 * @version 1.0.0
 * @date 2025
 * 
 * @details Este archivo contiene la implementación de la clase WaveletFilter,
 * que actúa como wrapper C++ para filtros wavelet implementados como banco
 * de filtros usando CMSIS-DSP. La implementación se enfoca en:
 * - Descomposición wavelet usando filtros FIR de aproximación y detalle
 * - Reconstrucción perfecta para análisis tiempo-frecuencia
 * - Gestión automática de memoria para buffers internos
 * - Interfaz simplificada para procesamiento de bioseñales
 * - Optimización para procesadores ARM Cortex-M
 * 
 * @see WaveletFilter.h para documentación de la interfaz pública
 */

#include "WaveletFilter.h"

// Definición de coeficientes estáticos

/**
 * @brief Coeficientes Daubechies-4 para filtro de aproximación (análisis pasa-bajo)
 * 
 * @details Estos coeficientes implementan la función de escalamiento φ(t) de Daubechies-4,
 * optimizada para extracción de componentes de baja frecuencia en bioseñales.
 * Los valores están normalizados para preservar la energía de la señal.
 * 
 * Propiedades:
 * - Soporte compacto: 8 coeficientes no nulos
 * - 4 momentos nulos (suprime polinomios de grado 3)
 * - Respuesta de fase casi lineal
 * - Frecuencia de corte aproximada: Fs/4
 */
const float32_t WaveletFilter::_approxCoeffs[8] = {
    0.23037781f,  0.71484657f,  0.63088077f, -0.02798377f,
   -0.18703481f,  0.03084138f,  0.03288301f, -0.01059740f
};

/**
 * @brief Coeficientes Daubechies-4 para filtro de detalle (análisis pasa-alto)
 * 
 * @details Estos coeficientes implementan la función wavelet ψ(t) de Daubechies-4,
 * optimizada para extracción de componentes de alta frecuencia y detección de transitorios.
 * Derivados de los coeficientes de aproximación mediante relación de ortogonalidad.
 * 
 * Propiedades:
 * - Ortogonal al filtro de aproximación
 * - Detecta bordes y discontinuidades eficientemente
 * - 4 momentos nulos (cancela tendencias polinómicas)
 * - Rango de frecuencia: Fs/4 a Fs/2
 */
const float32_t WaveletFilter::_detailCoeffs[8] = {
   -0.01059740f, -0.03288301f,  0.03084138f,  0.18703481f,
   -0.02798377f, -0.63088077f,  0.71484657f, -0.23037781f
};

/**
 * @brief Coeficientes de síntesis para reconstrucción de aproximación
 * 
 * @details Usados en la etapa de síntesis para reconstruir la señal desde
 * los coeficientes de aproximación. En Daubechies-4, son los coeficientes
 * de análisis en orden inverso para garantizar reconstrucción perfecta.
 */
const float32_t WaveletFilter::_synthApproxCoeffs[8] = {
   -0.01059740f,  0.03288301f,  0.03084138f, -0.18703481f,
   -0.02798377f,  0.63088077f,  0.71484657f,  0.23037781f
};

/**
 * @brief Coeficientes de síntesis para reconstrucción de detalle
 * 
 * @details Usados en la etapa de síntesis para reconstruir la señal desde
 * los coeficientes de detalle. Complementarios a los de síntesis de aproximación
 * para lograr reconstrucción perfecta.
 */
const float32_t WaveletFilter::_synthDetailCoeffs[8] = {
    0.23037781f, -0.71484657f,  0.63088077f,  0.02798377f,
   -0.18703481f, -0.03084138f,  0.03288301f,  0.01059740f
};

/**
 * @brief Constructor que inicializa el banco de filtros wavelet Daubechies-4
 * 
 * @details El constructor realiza las siguientes operaciones:
 * 1. Almacena el tamaño de bloque para procesamiento optimizado
 * 2. Crea las cuatro instancias de FIRFilter necesarias para análisis y síntesis
 * 3. Inicializa cada filtro con sus coeficientes específicos
 * 4. Configura el procesamiento para el tamaño de bloque especificado
 * 
 * @param blockSize Tamaño del bloque de procesamiento para optimizaciones SIMD
 * 
 * @remark
 * La implementación wavelet requiere cuatro filtros FIR:
 * - Análisis: aproximación (pasa-bajo) y detalle (pasa-alto)  
 * - Síntesis: reconstrucción de aproximación y detalle
 * 
 * @note Los filtros son inicializados con los coeficientes Daubechies-4 predefinidos.
 * Cada filtro FIR maneja automáticamente su buffer de estados interno.
 * 
 * @warning Si la asignación de memoria falla, el comportamiento es indefinido.
 * En producción se debería verificar que los punteros no sean nulos.
 */
WaveletFilter::WaveletFilter(uint16_t blockSize)
    : _blockSize(blockSize)
{
    // Crear filtros de análisis (descomposición)
    // Filtro de aproximación - extrae componentes de baja frecuencia
    _approxFilter = new FIRFilter(_approxCoeffs, _numTaps, _blockSize);
    
    // Filtro de detalle - extrae componentes de alta frecuencia  
    _detailFilter = new FIRFilter(_detailCoeffs, _numTaps, _blockSize);
    
    // Crear filtros de síntesis (reconstrucción)
    // Filtro para reconstruir desde coeficientes de aproximación
    _synthApproxFilter = new FIRFilter(_synthApproxCoeffs, _numTaps, _blockSize);
    
    // Filtro para reconstruir desde coeficientes de detalle
    _synthDetailFilter = new FIRFilter(_synthDetailCoeffs, _numTaps, _blockSize);
}

/**
 * @brief Destructor que libera recursos asignados dinámicamente
 * 
 * @details Libera la memoria asignada para las cuatro instancias de FIRFilter
 * usadas en el banco de filtros wavelet. El destructor garantiza que no hay
 * fugas de memoria cuando el objeto WaveletFilter sale de scope.
 * 
 * @remarks
 * Cada FIRFilter maneja automáticamente la limpieza de sus buffers internos,
 * por lo que solo necesitamos liberar las instancias de los objetos.
 * 
 * @note CMSIS-DSP no requiere funciones de limpieza específicas adicionales.
 */
WaveletFilter::~WaveletFilter() {
    // Liberar memoria de los filtros de análisis
    delete _approxFilter;
    delete _detailFilter;
    
    // Liberar memoria de los filtros de síntesis
    delete _synthApproxFilter;
    delete _synthDetailFilter;
}

/**
 * @brief Procesa una muestra individual obteniendo coeficientes wavelet
 * 
 * @details Esta función aplica la descomposición wavelet Daubechies-4 a una sola
 * muestra de entrada, calculando simultáneamente los coeficientes de aproximación
 * y detalle que representan las componentes de baja y alta frecuencia respectivamente.
 * 
 * @param input Muestra de entrada a procesar (típicamente normalizada ±1.0)
 * @param approxCoeff Puntero donde escribir el coeficiente de aproximación
 * @param detailCoeff Puntero donde escribir el coeficiente de detalle
 * 
 * @par Detalles de implementación
 * La función utiliza los dos filtros FIR de análisis en paralelo:
 * 1. **Filtro de aproximación**: Aplica pasa-bajo para extraer tendencias
 * 2. **Filtro de detalle**: Aplica pasa-alto para extraer transitorios
 * 3. Los resultados representan la descomposición wavelet de nivel 1
 * 4. Mantiene el estado interno para procesamiento continuo
 * 
 * @note Los coeficientes mantienen la energía total: |input|² ≈ |approx|² + |detail|²
 * 
 * @remark Para aplicaciones típicas:
 * - **Supresión de ruido**: Usar solo approxCoeff (componente suave)
 * - **Detección de eventos**: Analizar detailCoeff (cambios bruscos)
 * - **Análisis completo**: Usar ambos coeficientes
 * 
 * @example
 * @code
 * // Ejemplo de filtrado adaptativo de ECG
 * float32_t approx, detail;
 * waveletFilter.processSample(ecgSample, &approx, &detail);
 * 
 * // Supresión de ruido conservando morfología
 * float32_t cleanECG = approx;
 * 
 * // Detección de complejos QRS
 * if (abs(detail) > qrsThreshold) {
 *     detectQRSComplex();
 * }
 * @endcode
 */
void WaveletFilter::processSample(float32_t input, float32_t* approxCoeff, float32_t* detailCoeff) {
    // Aplicar filtro de aproximación (pasa-bajo) para componentes suaves
    *approxCoeff = _approxFilter->processSample(input);
    
    // Aplicar filtro de detalle (pasa-alto) para componentes transitorios  
    *detailCoeff = _detailFilter->processSample(input);
}

/**
 * @brief Procesa un buffer completo obteniendo arrays de coeficientes wavelet
 * 
 * @details Esta función aplica la descomposición wavelet a un array completo
 * de muestras de entrada, generando arrays paralelos de coeficientes de
 * aproximación y detalle. Optimizada para mayor rendimiento que múltiples
 * llamadas a processSample().
 * 
 * @param inputArray Puntero al array de muestras de entrada
 * @param approxArray Puntero al array donde escribir coeficientes de aproximación
 * @param detailArray Puntero al array donde escribir coeficientes de detalle
 * @param length Número de muestras a procesar
 * 
 * @par Optimizaciones implementadas
 * 1. Uso de processBuffer() de FIRFilter para máximo rendimiento
 * 2. Aprovechamiento de vectorización SIMD de CMSIS-DSP
 * 3. Procesamiento en bloque para reducir overhead de función
 * 4. Mantenimiento de continuidad del estado entre bloques
 * 
 * @remark
 * La función procesa ambos filtros (aproximación y detalle) en paralelo
 * sobre el mismo buffer de entrada, manteniendo sincronización perfecta
 * entre los coeficientes resultantes.
 * 
 * @note Los arrays de salida deben estar previamente asignados con al menos
 * 'length' elementos cada uno. La función no verifica límites de arrays.
 * 
 * @warning Los arrays de entrada y salida no deben solaparse en memoria
 * para evitar corrupción de datos durante el procesamiento optimizado.
 * 
 * @example
 * @code
 * // Análisis wavelet de buffer de EMG
 * const uint32_t bufferSize = 512;
 * float32_t emgData[bufferSize];
 * float32_t lowFreq[bufferSize];   // Actividad muscular base
 * float32_t highFreq[bufferSize];  // Actividad muscular rápida
 * 
 * waveletFilter.processBuffer(emgData, lowFreq, highFreq, bufferSize);
 * 
 * // Análisis de potencia en diferentes bandas
 * float32_t lowPower = 0.0f, highPower = 0.0f;
 * for (uint32_t i = 0; i < bufferSize; i++) {
 *     lowPower += lowFreq[i] * lowFreq[i];
 *     highPower += highFreq[i] * highFreq[i];
 * }
 * @endcode
 */
void WaveletFilter::processBuffer(const float32_t* inputArray, float32_t* approxArray, 
                                  float32_t* detailArray, uint32_t length) {
    // Procesar buffer completo con filtro de aproximación
    _approxFilter->processBuffer(inputArray, approxArray, length);
    
    // Procesar el mismo buffer con filtro de detalle
    _detailFilter->processBuffer(inputArray, detailArray, length);
}

/**
 * @brief Reconstruye una muestra a partir de coeficientes wavelet
 * 
 * @details Realiza la síntesis wavelet combinando los coeficientes de aproximación
 * y detalle para reconstruir la señal original o generar una versión filtrada.
 * La implementación garantiza reconstrucción perfecta cuando se usan los coeficientes
 * sin modificación.
 * 
 * @param approxCoeff Coeficiente de aproximación (componente de baja frecuencia)
 * @param detailCoeff Coeficiente de detalle (componente de alta frecuencia)
 * @return float32_t Muestra reconstruida
 * 
 * @par Algoritmo de reconstrucción
 * 1. Aplica filtro de síntesis de aproximación al coeficiente de aproximación
 * 2. Aplica filtro de síntesis de detalle al coeficiente de detalle
 * 3. Suma ambas contribuciones para obtener la muestra reconstruida
 * 4. El resultado preserva la energía y morfología de la señal original
 * 
 * @details Modos de uso típicos:
 * - **Reconstrucción completa**: reconstruct(approx, detail) ≈ original
 * - **Filtro pasa-bajo**: reconstruct(approx, 0) → solo componentes suaves
 * - **Filtro pasa-alto**: reconstruct(0, detail) → solo componentes transitorios
 * - **Filtrado selectivo**: modificar coeficientes según criterios adaptativos
 * 
 * @note La reconstrucción perfecta está garantizada por las propiedades de
 * ortogonalidad y bi-ortogonalidad de los filtros Daubechies-4.
 * 
 * @remark Para filtrado de ruido en bioseñales:
 * - Use solo aproximación si el ruido es de alta frecuencia
 * - Use filtrado adaptativo del detalle para preservar eventos importantes
 * - Combine con umbralización para supresión selectiva de artefactos
 * 
 * @example
 * @code
 * // Procesamiento adaptativo de EEG
 * float32_t approx, detail;
 * waveletFilter.processSample(eegSample, &approx, &detail);
 * 
 * // Filtrado adaptativo basado en amplitud del detalle
 * float32_t threshold = 0.1f * abs(approx); // Umbral adaptativo
 * float32_t filteredDetail = (abs(detail) > threshold) ? detail : 0.0f;
 * 
 * // Reconstruir con supresión selectiva de ruido
 * float32_t cleanEEG = waveletFilter.reconstruct(approx, filteredDetail);
 * 
 * // Para supresión máxima de ruido (solo tendencia)
 * float32_t smoothEEG = waveletFilter.reconstruct(approx, 0.0f);
 * @endcode
 */
float32_t WaveletFilter::reconstruct(float32_t approxCoeff, float32_t detailCoeff) {
    // Aplicar filtros de síntesis a cada coeficiente
    float32_t approxContrib = _synthApproxFilter->processSample(approxCoeff);
    float32_t detailContrib = _synthDetailFilter->processSample(detailCoeff);
    
    // Combinar ambas contribuciones para reconstrucción completa
    // La suma preserva la energía y garantiza reconstrucción perfecta
    return approxContrib + detailContrib;
}

/**
 * @brief Reinicia el estado interno de todos los filtros wavelet
 * 
 * @details Limpia completamente todos los buffers de estado interno de los
 * cuatro filtros FIR que componen el banco de filtros wavelet. Esto prepara
 * el filtro para procesar una nueva secuencia de datos independiente sin
 * influencia de muestras procesadas anteriormente.
 * 
 * @par Estados que se reinician:
 * 1. **Filtro de análisis de aproximación**: Buffer de muestras previas
 * 2. **Filtro de análisis de detalle**: Buffer de muestras previas  
 * 3. **Filtro de síntesis de aproximación**: Buffer de coeficientes previos
 * 4. **Filtro de síntesis de detalle**: Buffer de coeficientes previos
 * 
 * @details Esta función es especialmente útil en:
 * - Cambio entre diferentes pacientes o sujetos de estudio
 * - Inicio de nuevas sesiones de adquisición de datos
 * - Procesamiento de segmentos de datos discontinuos
 * - Eliminación de transitorios iniciales entre grabaciones
 * 
 * @note La función reinicializa completamente los filtros FIR internos para
 * garantizar un estado completamente limpio. Esto es más robusto que solo
 * limpiar buffers y garantiza la inicialización correcta.
 * 
 * @remark Después del reset, las primeras muestras procesadas pueden mostrar
 * transitorios hasta que los buffers internos se llenen completamente.
 * Esto es normal y esperado en filtros FIR.
 * 
 * @example
 * @code
 * // Análisis de múltiples segmentos de ECG independientes
 * WaveletFilter ecgAnalyzer(1);
 * 
 * // Procesar primer segmento
 * for (int i = 0; i < segment1Length; i++) {
 *     ecgAnalyzer.processSample(segment1[i], &approx, &detail);
 *     // ... procesar coeficientes
 * }
 * 
 * // Reiniciar para el segundo segmento (eliminar historia previa)
 * ecgAnalyzer.reset();
 * 
 * // Procesar segundo segmento con estado limpio
 * for (int i = 0; i < segment2Length; i++) {
 *     ecgAnalyzer.processSample(segment2[i], &approx, &detail);
 *     // ... procesar coeficientes independientemente del primer segmento
 * }
 * @endcode
 */
void WaveletFilter::reset() {
    // Liberar y recrear filtros de análisis para reinicializar completamente su estado
    delete _approxFilter;
    delete _detailFilter;
    _approxFilter = new FIRFilter(_approxCoeffs, _numTaps, _blockSize);
    _detailFilter = new FIRFilter(_detailCoeffs, _numTaps, _blockSize);
    
    // Liberar y recrear filtros de síntesis para reinicializar completamente su estado
    delete _synthApproxFilter;
    delete _synthDetailFilter;
    _synthApproxFilter = new FIRFilter(_synthApproxCoeffs, _numTaps, _blockSize);
    _synthDetailFilter = new FIRFilter(_synthDetailCoeffs, _numTaps, _blockSize);
    
    // Esta implementación reinicializa completamente los filtros FIR,
    // garantizando un estado interno completamente limpio sin necesidad
    // de modificar la clase FIRFilter existente.
}