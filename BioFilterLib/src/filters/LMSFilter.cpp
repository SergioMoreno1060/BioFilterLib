/**
 * @file LMSFilter.cpp
 * @brief Implementación del filtro LMS adaptativo optimizado para bioseñales
 * @author Sergio
 * @version 1.0.0
 * @date 2025
 * 
 * @details Este archivo contiene la implementación de la clase LMSFilter,
 * que actúa como wrapper C++ para las funciones de filtrado LMS (Least Mean Squares)
 * adaptativo de CMSIS-DSP. La implementación se enfoca en:
 * - Adaptación automática de coeficientes para minimizar el error cuadrático medio
 * - Gestión eficiente de memoria para estados y coeficientes adaptativos
 * - Interfaz simplificada para procesamiento adaptativo de bioseñales
 * - Optimización para procesadores ARM Cortex-M con SIMD
 * 
 * El algoritmo LMS implementado realiza:
 * 1. Filtrado FIR con coeficientes actuales: y[n] = Σ(w[k] * x[n-k])
 * 2. Cálculo del error: e[n] = d[n] - y[n]
 * 3. Actualización de coeficientes: w[k] = w[k] + μ * e[n] * x[n-k]
 * 
 * @see LMSFilter.h para documentación de la interfaz pública
 */

#include "LMSFilter.h"

/**
 * @brief Constructor que inicializa el filtro LMS adaptativo con parámetros específicos
 * 
 * @details El constructor realiza las siguientes operaciones críticas:
 * 1. Inicializa las variables miembro con los parámetros proporcionados
 * 2. Calcula y asigna memoria para el buffer de estados interno (numTaps elementos)
 * 3. Inicializa la estructura arm_lms_instance_f32 de CMSIS-DSP
 * 4. Configura el filtro adaptativo para procesamiento optimizado
 * 
 * @param coeffs Puntero al array de coeficientes iniciales del filtro (modificable)
 * @param numTaps Número de coeficientes (orden del filtro + 1)
 * @param mu Paso de adaptación que controla la velocidad de convergencia
 * @param blockSize Tamaño del bloque de procesamiento para optimizaciones SIMD
 * 
 * @remark
 * El buffer de estados para LMS tiene un tamaño de numTaps elementos, diferente
 * al FIR que requiere (numTaps + blockSize - 1). Esto se debe a que CMSIS-DSP
 * maneja internamente el acceso circular para filtros adaptativos.
 * 
 * @note La función arm_lms_init_f32() configura internamente:
 * - Punteros a coeficientes adaptativos y buffer de estados
 * - Parámetro de paso de adaptación (mu)
 * - Configuración para actualización automática de coeficientes
 * - Optimizaciones SIMD cuando están disponibles
 * 
 * @warning Si la asignación de memoria falla, el comportamiento es indefinido.
 * En un sistema de producción se debería verificar el retorno de 'new'.
 * 
 * @par Consideraciones para la selección de μ (mu):
 * - μ muy pequeño (< 0.001): Convergencia lenta, muy estable, poco ruido residual
 * - μ moderado (0.001-0.01): Balance entre velocidad y estabilidad
 * - μ grande (> 0.1): Convergencia rápida pero posible inestabilidad
 * 
 * Para bioseñales típicas:
 * - ECG (cancelación 50/60Hz): μ = 0.01-0.05
 * - EMG (supresión artefactos): μ = 0.001-0.01  
 * - EEG (eliminación parpadeo): μ = 0.0001-0.001
 */
LMSFilter::LMSFilter(float32_t* coeffs, uint16_t numTaps, float32_t mu, uint16_t blockSize)
    : _coeffs(coeffs),           // Referencia a coeficientes adaptativos (modificables)
      _numTaps(numTaps),         // Número de coeficientes del filtro
      _mu(mu),                   // Paso de adaptación
      _blockSize(blockSize)      // Tamaño de bloque para optimizaciones
{
    // Calcular tamaño del buffer de estados para filtros LMS
    // Para LMS, el tamaño es exactamente numTaps (diferente a FIR)
    // Esto es porque CMSIS-DSP maneja internamente la gestión circular
    uint32_t stateBufferSize = _numTaps;
    
    // Asignar memoria para buffer de estados e inicializar a cero
    // La inicialización a cero es crítica para evitar transitorios
    // y asegurar una convergencia estable desde el inicio
    _state = new float32_t[stateBufferSize]();
    
    // Inicializar la estructura del filtro LMS adaptativo de CMSIS-DSP
    // Esta función configura todos los parámetros necesarios para
    // el procesamiento adaptativo optimizado
    arm_lms_init_f32(&_lmsInstance,    // Puntero a la instancia del filtro LMS
                     _numTaps,         // Número de coeficientes adaptativos
                     _coeffs,          // Puntero a coeficientes (serán modificados)
                     _state,           // Puntero al buffer de estados
                     _mu,              // Paso de adaptación
                     _blockSize);      // Tamaño de bloque para optimización SIMD
}

/**
 * @brief Destructor que libera recursos asignados dinámicamente
 * 
 * @details Libera la memoria asignada para el buffer de estados interno.
 * El destructor garantiza que no hay fugas de memoria cuando el objeto
 * LMSFilter sale de scope o es eliminado explícitamente.
 * 
 * @remarks
 * Solo necesita liberar _state ya que:
 * - _coeffs es una referencia externa (gestionada por el usuario)
 * - _lmsInstance es una estructura por valor (limpieza automática)
 * - Las variables primitivas se limpian automáticamente
 * 
 * @note CMSIS-DSP no requiere funciones de limpieza específicas para
 * la estructura arm_lms_instance_f32. Los coeficientes adaptativos
 * permanecen en su estado final para posibles consultas posteriores.
 */
LMSFilter::~LMSFilter() {
    // Liberar memoria del buffer de estados
    // Usar delete[] porque se asignó con new[]
    delete[] _state;
    
    // Nota: Los coeficientes (_coeffs) NO se liberan aquí porque:
    // - Son gestionados externamente por el usuario
    // - Pueden contener información valiosa post-adaptación
    // - Su liberación es responsabilidad del código que los creó
}

/**
 * @brief Procesa una muestra individual usando el filtro LMS adaptativo
 * 
 * @details Esta función aplica el filtro LMS a una muestra de entrada usando
 * la señal de referencia correspondiente. Internamente realiza:
 * 1. Filtrado FIR con coeficientes actuales
 * 2. Cálculo del error de adaptación
 * 3. Actualización automática de coeficientes usando el algoritmo LMS
 * 4. Mantenimiento del estado para la próxima muestra
 * 
 * @param input Muestra de entrada (señal primaria a procesar)
 * @param reference Muestra de referencia (señal deseada para adaptación)
 * @param output Puntero donde escribir la muestra filtrada
 * @param error Puntero donde escribir la señal de error (reference - output)
 * 
 * @par Detalles del algoritmo LMS implementado:
 * El algoritmo ejecuta los siguientes pasos en cada muestra:
 * 1. **Filtrado**: y[n] = Σ(w[k] * x[n-k]) donde w[k] son los coeficientes
 * 2. **Error**: e[n] = d[n] - y[n] donde d[n] es la señal de referencia
 * 3. **Adaptación**: w[k] ← w[k] + μ * e[n] * x[n-k] para todos los coeficientes
 * 
 * @note La función arm_lms_f32() maneja automáticamente:
 * - Gestión circular del buffer de estados
 * - Actualización vectorizada de todos los coeficientes
 * - Optimizaciones específicas del procesador ARM Cortex-M
 * - Manejo eficiente de la aritmética de punto flotante
 * 
 * @remark Interpretación de la señal de error:
 * - error > 0: La salida del filtro es menor que la referencia
 * - error < 0: La salida del filtro es mayor que la referencia  
 * - |error| pequeño: Buena adaptación, filtro convergente
 * - |error| oscilante: Posible μ demasiado grande
 * 
 * @par Caso de uso típico - Cancelación de interferencia de 60Hz:
 * @code
 * float32_t ecgSample = readADC();  // ECG contaminado con 60Hz
 * float32_t ref60Hz = sin(2*PI*60*t); // Referencia sinusoidal de 60Hz
 * float32_t cleanECG, adaptError;
 * 
 * filter.processSample(ecgSample, ref60Hz, &cleanECG, &adaptError);
 * 
 * // cleanECG: ECG con interferencia de 60Hz reducida
 * // adaptError: efectividad de la cancelación (idealmente → 0)
 * @endcode
 */
void LMSFilter::processSample(float32_t input, float32_t reference, 
                             float32_t* output, float32_t* error) {
    // Llamar a la función optimizada de CMSIS-DSP para procesar una muestra
    // Esta implementación maneja automáticamente:
    // - El filtrado FIR con coeficientes actuales
    // - El cálculo del error de adaptación
    // - La actualización de todos los coeficientes según el algoritmo LMS
    // - La actualización del buffer de estados circular
    
    // Parámetros de arm_lms_f32():
    // - &_lmsInstance: instancia configurada del filtro adaptativo
    // - &input: puntero a la muestra de entrada (señal primaria)
    // - &reference: puntero a la muestra de referencia (señal deseada)
    // - output: puntero donde escribir la salida filtrada
    // - error: puntero donde escribir el error de adaptación
    // - 1: procesar exactamente 1 muestra
    arm_lms_f32(&_lmsInstance,    // Instancia del filtro previamente inicializada
                &input,           // Dirección de la muestra de entrada
                &reference,       // Dirección de la muestra de referencia  
                output,           // Dirección donde escribir la salida filtrada
                error,            // Dirección donde escribir el error
                1);               // Número de muestras a procesar (1)
    
    // Tras esta llamada:
    // - *output contiene la salida y[n] del filtro adaptativo
    // - *error contiene e[n] = reference - output
    // - Los coeficientes en _coeffs han sido actualizados automáticamente
    // - El buffer de estados se ha actualizado para la próxima muestra
}

/**
 * @brief Procesa un buffer completo de muestras usando el filtro LMS adaptativo
 * 
 * @details Esta función aplica el filtro LMS a arrays completos de muestras
 * de entrada y referencia, escribiendo los resultados en los arrays de salida
 * y error. Es más eficiente que múltiples llamadas a processSample() debido
 * a las optimizaciones vectoriales (SIMD) de CMSIS-DSP.
 * 
 * @param inputArray Puntero al array de muestras de entrada (señal primaria)
 * @param referenceArray Puntero al array de muestras de referencia
 * @param outputArray Puntero al array donde escribir las muestras filtradas
 * @param errorArray Puntero al array donde escribir las señales de error
 * @param length Número de muestras a procesar
 * 
 * @remark
 * Utiliza arm_lms_f32() para procesar el bloque completo:
 * 1. Procesa 'length' muestras en una sola llamada optimizada
 * 2. Utiliza vectorización SIMD para máximo rendimiento
 * 3. Mantiene continuidad del estado adaptativo entre muestras
 * 4. Actualiza progresivamente los coeficientes a lo largo del bloque
 * 
 * @note Ventajas del procesamiento por bloques en filtros adaptativos:
 * - Mayor throughput debido a optimizaciones SIMD de ARM
 * - Menor overhead de llamadas a función
 * - Mejor uso de la caché del procesador
 * - Aprovechamiento del paralelismo a nivel de instrucción
 * - Adaptación continua durante todo el bloque
 * 
 * @warning Consideraciones importantes:
 * - Todos los arrays deben tener exactamente 'length' elementos válidos
 * - Los arrays NO deben solaparse en memoria (undefined behavior)
 * - El estado adaptativo se preserva entre llamadas consecutivas
 * - Los coeficientes se modifican continuamente durante el procesamiento
 * 
 * @par Comportamiento adaptativo durante el bloque:
 * A diferencia del procesamiento por bloques de filtros fijos (FIR/IIR),
 * el LMS actualiza sus coeficientes para cada muestra dentro del bloque.
 * Esto significa que:
 * - La primera muestra usa los coeficientes iniciales
 * - Las muestras posteriores usan coeficientes progresivamente adaptados
 * - Al final del bloque, los coeficientes han evolucionado 'length' veces
 * 
 * @par Uso típico para cancelación adaptativa de artefactos en EEG:
 * @code
 * // Buffers para procesamiento por bloques (512 muestras @ 250Hz = 2.048s)
 * float32_t eegSignal[512];        // EEG contaminado con parpadeo
 * float32_t blinkReference[512];   // Señal EOG como referencia
 * float32_t cleanEEG[512];         // EEG limpio (salida)
 * float32_t adaptationError[512];  // Error de adaptación
 * 
 * // Adquirir datos de los ADCs
 * readEEGBuffer(eegSignal, 512);
 * readEOGBuffer(blinkReference, 512);
 * 
 * // Aplicar filtro adaptativo para cancelar artefactos de parpadeo
 * lmsFilter.processBuffer(eegSignal, blinkReference, cleanEEG, adaptationError, 512);
 * 
 * // Analizar la efectividad de la adaptación
 * float32_t meanSquareError = 0.0f;
 * for(int i = 0; i < 512; i++) {
 *     meanSquareError += adaptationError[i] * adaptationError[i];
 * }
 * meanSquareError /= 512;
 * 
 * Serial.print("MSE de adaptación: ");
 * Serial.println(meanSquareError, 6);
 * 
 * // Si MSE decrece → buena adaptación
 * // Si MSE oscila → considerar reducir μ
 * @endcode
 */
void LMSFilter::processBuffer(const float32_t* inputArray, float32_t* referenceArray,
                             float32_t* outputArray, float32_t* errorArray, uint32_t length) {
    // Procesar el buffer completo usando la función optimizada de CMSIS-DSP
    // Esta implementación aprovecha completamente las optimizaciones vectoriales (SIMD)
    // disponibles en procesadores ARM para máximo rendimiento en filtros adaptativos
    
    // Parámetros de arm_lms_f32() para procesamiento por bloques:
    // - &_lmsInstance: instancia del filtro con configuración y estado adaptativo
    // - inputArray: puntero al primer elemento del buffer de entrada (señal primaria)
    // - referenceArray: puntero al primer elemento del buffer de referencia
    // - outputArray: puntero al primer elemento del buffer de salida filtrada
    // - errorArray: puntero al primer elemento del buffer de error
    // - length: número total de muestras a procesar adaptativamente
    arm_lms_f32(&_lmsInstance,        // Instancia previamente inicializada
                inputArray,           // Buffer de entrada (solo lectura)
                referenceArray,       // Buffer de referencia (solo lectura)
                outputArray,          // Buffer de salida filtrada (escritura)
                errorArray,           // Buffer de error de adaptación (escritura)
                length);              // Número de muestras a procesar
    
    // Nota: arm_lms_f32() maneja internamente y de forma optimizada:
    // - Actualización progresiva del buffer de estados con nuevas muestras
    // - Cálculo vectorizado de la salida del filtro para cada muestra
    // - Cálculo del error de adaptación: e[n] = referencia[n] - salida[n]
    // - Actualización automática de TODOS los coeficientes usando algoritmo LMS
    // - Preservación del estado adaptativo para llamadas futuras
    // - Optimizaciones específicas del hardware ARM (SIMD, cache, pipeline)
}

/**
 * @brief Modifica el paso de adaptación durante la operación del filtro
 * 
 * @details Permite ajustar dinámicamente la velocidad de convergencia del filtro
 * adaptativo sin necesidad de reinicializar toda la estructura. Esto es útil
 * para implementar esquemas de adaptación variables o para optimizar la
 * convergencia según las condiciones cambiantes de la señal.
 * 
 * @param newMu Nuevo valor del paso de adaptación (μ)
 * 
 * @remark Casos de uso para el cambio dinámico de μ:
 * - **Inicio rápido**: μ grande inicialmente para convergencia rápida
 * - **Refinamiento**: μ pequeño después para estabilidad y bajo ruido residual  
 * - **Adaptación a SNR**: μ mayor cuando la señal es fuerte, menor con ruido alto
 * - **Control basado en error**: μ adaptativo según la magnitud del error
 * 
 * @warning Consideraciones de estabilidad:
 * - μ > 2/λmax puede causar inestabilidad (λmax = mayor eigenvalor de la matriz de autocorrelación)
 * - Cambios abruptos de μ pueden causar transitorios temporales
 * - μ muy grande puede provocar oscilaciones en los coeficientes
 * 
 * @note La modificación toma efecto inmediatamente en la siguiente muestra procesada.
 * No se requiere reinicialización del filtro ni pérdida del estado adaptativo.
 * 
 * @par Implementación de μ adaptativo basado en el error:
 * @code
 * float32_t output, error;
 * filter.processSample(input, reference, &output, &error);
 * 
 * // Adaptar μ basado en la magnitud del error
 * float32_t absError = fabs(error);
 * if (absError > 0.1) {
 *     filter.setMu(0.05f);  // Error grande → convergencia rápida
 * } else if (absError < 0.01) {
 *     filter.setMu(0.001f); // Error pequeño → estabilidad fina
 * }
 * @endcode
 */
void LMSFilter::setMu(float32_t newMu) {
    // Actualizar el valor interno del paso de adaptación
    _mu = newMu;
    
    // Actualizar directamente el campo μ en la instancia de CMSIS-DSP
    // Esto evita tener que reinicializar completamente el filtro
    // preservando el estado adaptativo actual (coeficientes y buffer de estados)
    _lmsInstance.mu = newMu;
    
    // Nota: El cambio toma efecto inmediatamente en la siguiente llamada
    // a processSample() o processBuffer(). No se pierde información del
    // estado adaptativo ni se requieren operaciones adicionales.
}

/**
 * @brief Reinicia los coeficientes adaptativos a valores específicos o cero
 * 
 * @details Esta función permite reinicializar los coeficientes del filtro adaptativo
 * sin recrear toda la instancia. Es útil para implementar esquemas de adaptación
 * por bloques, recuperarse de convergencias pobres, o reiniciar cuando cambian
 * drásticamente las condiciones de la señal.
 * 
 * @param newCoeffs Puntero a los nuevos coeficientes iniciales. Si es nullptr,
 *                  se inicializan todos los coeficientes a cero.
 * 
 * @details Operaciones realizadas:
 * 1. **Coeficientes**: Se copian los nuevos valores o se ponen a cero
 * 2. **Buffer de estados**: Se reinicia completamente a cero
 * 3. **Parámetros adaptativos**: Se mantienen (μ, numTaps, etc.)
 * 4. **No se modifica**: La instancia CMSIS-DSP preserva su configuración
 * 
 * @remark Casos de uso típicos:
 * - **Cambio de contexto**: Nueva tarea de filtrado con características diferentes
 * - **Recuperación**: El filtro no converge o converge a un mínimo local pobre
 * - **Adaptación por bloques**: Reinicio periódico para evitar deriva de coeficientes
 * - **Detección de cambios**: Reinicio cuando se detecta cambio en las estadísticas de la señal
 * 
 * @note Efectos del reinicio:
 * - Se pierde toda la "memoria" adaptativa previa
 * - El filtro debe reconverger desde el nuevo punto inicial  
 * - Puede haber un transitorio temporal hasta que se readapte
 * - La velocidad de reconvergencia depende del valor de μ
 * 
 * @warning El transitorio de reconvergencia puede durar:
 * - Filtros rápidos (μ grande): Decenas de muestras
 * - Filtros lentos (μ pequeño): Cientos o miles de muestras
 * 
 * @par Reinicio automático basado en detección de cambio:
 * @code
 * static float32_t previousError = 0.0f;
 * static int poorConvergenceCounter = 0;
 * 
 * float32_t output, error;
 * filter.processSample(input, reference, &output, &error);
 * 
 * // Detectar convergencia pobre (error no decrece)
 * if (fabs(error) > fabs(previousError) * 1.1f) {
 *     poorConvergenceCounter++;
 * } else {
 *     poorConvergenceCounter = 0;
 * }
 * 
 * // Si no converge durante 100 muestras consecutivas, reiniciar
 * if (poorConvergenceCounter > 100) {
 *     Serial.println("Reiniciando filtro por convergencia pobre...");
 *     filter.resetCoefficients(); // Reiniciar a cero
 *     poorConvergenceCounter = 0;
 * }
 * 
 * previousError = error;
 * @endcode
 */
void LMSFilter::resetCoefficients(const float32_t* newCoeffs) {
    // Reinicializar los coeficientes del filtro adaptativo
    if (newCoeffs != nullptr) {
        // Copiar los nuevos coeficientes proporcionados
        for (uint16_t i = 0; i < _numTaps; i++) {
            _coeffs[i] = newCoeffs[i];
        }
    } else {
        // Inicializar todos los coeficientes a cero (arranque desde cero)
        for (uint16_t i = 0; i < _numTaps; i++) {
            _coeffs[i] = 0.0f;
        }
    }
    
    // Limpiar completamente el buffer de estados
    // Esto elimina toda la "memoria" de muestras previas
    for (uint16_t i = 0; i < _numTaps; i++) {
        _state[i] = 0.0f;
    }
    
    // Nota: No es necesario llamar a arm_lms_init_f32() nuevamente
    // porque la instancia CMSIS-DSP solo mantiene punteros a estos buffers.
    // Los cambios en _coeffs y _state son inmediatamente efectivos.
    
    // El filtro está ahora listo para una nueva fase de adaptación
    // con las mismas configuraciones (μ, numTaps, blockSize) pero
    // sin memoria de adaptaciones previas.
}

/**
 * @note Consideraciones adicionales de implementación:
 * 
 * 1. GESTIÓN DE MEMORIA:
 *    - El buffer de estados LMS es más simple que FIR (solo numTaps elementos)
 *    - Los coeficientes son modificados directamente durante la adaptación
 *    - La memoria se gestiona automáticamente pero los coeficientes persisten
 * 
 * 2. INTEGRACIÓN CON CMSIS-DSP:
 *    - Se utiliza arm_lms_f32() que implementa el algoritmo LMS completo
 *    - Las optimizaciones SIMD se aplican tanto al filtrado como a la adaptación
 *    - La actualización de coeficientes es vectorizada cuando es posible
 * 
 * 3. OPTIMIZACIONES DE RENDIMIENTO:
 *    - Los coeficientes se almacenan por referencia (modificación in-situ)
 *    - El buffer de estados usa acceso circular optimizado
 *    - Se minimiza el overhead entre C++ y CMSIS-DSP
 *    - Procesamiento por bloques maximiza el uso de SIMD
 * 
 * 4. APLICACIONES EN BIOSEÑALES:
 *    - Cancelación adaptativa: interferencia electromagnética, artefactos
 *    - Identificación: modelado de sistemas cardiovasculares, neuromusculares
 *    - Predicción: anticipación de eventos fisiológicos
 *    - Ecualización: compensación de distorsión en canales de transmisión
 * 
 * 5. CONSIDERACIONES DE DISEÑO PARA BIOSEÑALES:
 *    - Inicialización de coeficientes: típicamente a cero para convergencia estable
 *    - Selección de μ: balance entre velocidad y estabilidad según el tipo de señal
 *    - Número de coeficientes: suficientes para modelar la respuesta del sistema
 *    - Monitoreo del error: indicador clave de la calidad de adaptación
 * 
 * 6. DEBUGGING Y OPTIMIZACIÓN:
 *    - Graficar la evolución de los coeficientes para verificar convergencia
 *    - Monitorear la señal de error para detectar problemas de estabilidad
 *    - Usar μ adaptativo para optimizar el rendimiento dinámicamente
 *    - Implementar criterios de reinicio automático para robustez
 */