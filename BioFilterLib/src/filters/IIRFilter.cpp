/**
 * @file IIRFilter.cpp
 * @brief Implementación del filtro IIR Biquad optimizado para bioseñales
 * @author Tu Nombre
 * @version 1.0.0
 * @date 2024
 * * @details Este archivo contiene la implementación de la clase IIRFilter,
 * que actúa como wrapper C++ para las funciones de filtrado IIR Biquad de CMSIS-DSP.
 * La implementación se enfoca en la estabilidad numérica y eficiencia computacional
 * que ofrece la estructura de cascada de secciones de segundo orden (Biquad).
 * * @see IIRFilter.h para documentación de la interfaz pública.
 */

#include "IIRFilter.h"

/**
 * @brief Constructor que inicializa el filtro IIR Biquad.
 * * @details El constructor realiza las siguientes operaciones:
 * 1. Almacena las referencias y parámetros del filtro.
 * 2. Calcula el tamaño del buffer de estado necesario: cada etapa Biquad requiere
 * 4 valores de estado (x[n-1], x[n-2], y[n-1], y[n-2]).
 * 3. Asigna memoria dinámicamente para el buffer de estados y la inicializa a cero.
 * 4. Llama a arm_biquad_cascade_df1_init_f32() para configurar la instancia
 * del filtro de CMSIS-DSP con los coeficientes y el estado.
 * * @param coeffs Puntero al array de coeficientes del filtro (numStages * 5).
 * @param numStages Número de secciones Biquad.
 * @param blockSize Tamaño del bloque de procesamiento.
 * * @warning Si la asignación de memoria falla, el comportamiento es indefinido.
 * Es recomendable añadir una comprobación de puntero nulo en un sistema de producción.
 */
IIRFilter::IIRFilter(const float32_t* coeffs, uint8_t numStages, uint16_t blockSize)
    : _coeffs(coeffs),
      _numStages(numStages),
      _blockSize(blockSize)
{
    // El buffer de estados para un filtro IIR Biquad en cascada necesita
    // 4 * numStages elementos de 32 bits, según la documentación de CMSIS-DSP.
    // Cada etapa almacena 2 estados de entrada y 2 de salida anteriores.
    uint32_t stateBufferSize = 4 * _numStages;
    
    // Asignar memoria para el buffer de estados e inicializarla a cero.
    // El () al final asegura la inicialización a 0.0f para evitar transitorios.
    _state = new float32_t[stateBufferSize]();

    // Inicializar la estructura del filtro IIR Biquad de CMSIS-DSP.
    // Esta función configura la instancia para que apunte a los coeficientes
    // y al buffer de estado, preparándola para el procesamiento.
    arm_biquad_cascade_df1_init_f32(&_iirInstance,    // Puntero a la instancia del filtro
                                    _numStages,       // Número de secciones Biquad
                                    _coeffs,          // Puntero a los coeficientes
                                    _state);          // Puntero al buffer de estados
}

/**
 * @brief Destructor que libera la memoria del buffer de estados.
 * * @details Libera de forma segura la memoria asignada dinámicamente en el constructor
 * para evitar fugas de memoria.
 */
IIRFilter::~IIRFilter() {
    // Liberar la memoria asignada para el buffer de estados.
    // Se usa delete[] porque se asignó con new[].
    delete[] _state;
}

/**
 * @brief Procesa una muestra individual usando el filtro IIR.
 * * @details Esta función es una envoltura alrededor de la función de procesamiento de
 * CMSIS-DSP, configurada para procesar una única muestra. Es ideal para
 * aplicaciones en tiempo real que operan muestra por muestra.
 * * @param input Muestra de entrada a filtrar.
 * @return float32_t Muestra filtrada de salida.
 * * @par Detalles de implementación
 * Llama a arm_biquad_cascade_df1_f32() con una longitud de bloque de 1.
 * La función de CMSIS-DSP maneja automáticamente la actualización del estado
 * interno del filtro a través de todas las etapas en cascada, asegurando
 * la continuidad entre llamadas.
 */
float32_t IIRFilter::processSample(float32_t input) {
    float32_t output; // Variable para almacenar la salida

    // Llamar a la función optimizada de CMSIS-DSP para procesar una muestra.
    // La función procesará la entrada a través de todas las secciones Biquad
    // en cascada y actualizará el buffer de estados interno.
    arm_biquad_cascade_df1_f32(&_iirInstance, // Instancia del filtro
                               &input,        // Puntero a la muestra de entrada
                               &output,       // Puntero para la muestra de salida
                               1);            // Procesar solo 1 muestra

    return output;
}

/**
 * @brief Procesa un buffer completo de muestras usando el filtro IIR.
 * * @details Aplica el filtro IIR a un array completo de muestras, aprovechando
 * las optimizaciones de procesamiento en bloque de CMSIS-DSP (SIMD) para un
 * mayor rendimiento en comparación con el procesamiento muestra por muestra.
 * * @param inputArray Puntero al buffer de entrada.
 * @param outputArray Puntero al buffer de salida.
 * @param length Número de muestras en los buffers.
 * * @remark
 * El estado del filtro se mantiene y actualiza correctamente entre llamadas,
 * permitiendo procesar un flujo de datos continuo en bloques. La función
 * arm_biquad_cascade_df1_f32 está altamente optimizada para arquitecturas ARM
 * y ofrece un rendimiento significativamente mejor para bloques de datos grandes.
 */
void IIRFilter::processBuffer(const float32_t* inputArray, float32_t* outputArray, uint32_t length) {
    // Procesar el buffer completo en una sola llamada a la función de CMSIS-DSP.
    // Esto es mucho más eficiente que un bucle llamando a processSample,
    // ya que reduce el overhead de llamadas y aprovecha las instrucciones SIMD.
    arm_biquad_cascade_df1_f32(&_iirInstance, // Instancia del filtro
                               inputArray,    // Buffer de entrada
                               outputArray,   // Buffer de salida
                               length);       // Número de muestras a procesar
}