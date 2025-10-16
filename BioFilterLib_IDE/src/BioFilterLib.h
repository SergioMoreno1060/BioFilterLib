/*
 * BioFilterLib.h - Biblioteca de filtros digitales optimizados para Arduino Due
 */

 #ifndef BIOFILTERLIB_H
 #define BIOFILTERLIB_H
//  #define ARM_MATH_CM3 // Definir el core Cortex-M3 (Arduino Due), para Cortex-M4 o Cortex-M0 usar ARM_MATH_CM4 o ARM_MATH_CM0

 #include <Arduino.h>
 #include <arm_math.h>
 
 #include "filters/FIRFilter.h"
 #include "filters/IIRFilter.h"
 #include "filters/LMSFilter.h"
 #include "filters/WaveletFilter.h"
 #include "utils/utils.h"
 #include "utils/Waveforms.h"
 #include "utils/TestRunner.h"
 
 #endif // BIOFILTERLIB_H
 