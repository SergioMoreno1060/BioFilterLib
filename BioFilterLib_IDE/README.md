````markdown
# BioFilterLib para Arduino IDE

**BioFilterLib** es una biblioteca diseñada para procesar y filtrar señales biológicas en plataformas basadas en **ARM Cortex**.  
Esta versión está adaptada específicamente para el uso con **Arduino IDE**.

---

## 🚀 Instalación

### 1. Descargar la biblioteca

Esta versión se encuentra en el branch `arduino_ide_version`.  
Descarga o clona el repositorio desde GitHub y asegúrate de estar en la rama correcta:

```bash
git clone -b arduino_ide_version https://github.com/tu_usuario/BioFilterLib.git
````

Dentro del repositorio encontrarás una carpeta llamada:

```
BioFilterLib_IDE
```

Esa es la carpeta que contiene la versión compatible con Arduino IDE.

---

### 2. Instalar en Arduino IDE

1. Copia la carpeta `BioFilterLib_IDE`.
2. Pégala en el directorio de bibliotecas de Arduino:

   * **Windows:** `Documentos/Arduino/libraries/`
   * **macOS/Linux:** `~/Arduino/libraries/`
3. Reinicia el **Arduino IDE**.
4. La biblioteca debería aparecer en:
   **Sketch > Include Library > BioFilterLib**

---

## ⚙️ Configuración para diferentes microcontroladores ARM

Por defecto, la biblioteca está configurada para el **Arduino Due** (basado en **ARM Cortex-M3**).

Si utilizas un microcontrolador diferente (por ejemplo, Cortex-M4 o Cortex-M0), debes modificar el *macro* en el archivo `BioFilterLib.h`:

```cpp
// Archivo: BioFilterLib.h

// Por defecto:
#define ARM_MATH_CM3

// Cambiar según tu plataforma:
#define ARM_MATH_CM4   // Para ARM Cortex-M4
#define ARM_MATH_CM0   // Para ARM Cortex-M0
```

Guarda los cambios antes de compilar el proyecto.

---

## 🧠 Compatibilidad

| Plataforma       | Núcleo ARM    | Macro a usar   |
| ---------------- | ------------- | -------------- |
| Arduino Due      | ARM Cortex-M3 | `ARM_MATH_CM3` |
| Teensy 3.x / 4.x | ARM Cortex-M4 | `ARM_MATH_CM4` |
| Otros ARM M0     | ARM Cortex-M0 | `ARM_MATH_CM0` |

---

## 📄 Licencia

Esta biblioteca se distribuye bajo la licencia especificada en el repositorio original.

---

## 📬 Contacto

Para reportar errores o contribuir al desarrollo, abre un *issue* o un *pull request* en el repositorio oficial.

```
```
