# Termómetro Digital con Análisis Estadístico

Sistema de medición de temperatura y humedad desarrollado con **PIC16F887**, **DHT11**, reloj en tiempo real (**RTC**), pantalla LCD 16x2 y almacenamiento mediante microSD. El firmware incluye menú local, estadísticas y un pronóstico de seis pasos. Python complementa el sistema con análisis del histórico, gráficas y pronóstico ARIMA/SARIMA.

## Objetivo

Desarrollar un instrumento autónomo que mida temperatura y humedad, muestre los valores en una LCD 16x2, use un RTC para fechar mediciones, guarde el histórico en una microSD y permita consultar estadísticas y un pronóstico de seis pasos. Un notebook de Python sirve como punto de referencia estadístico y de pronóstico.

## Funcionalidades

### Firmware embebido

- Lectura del sensor DHT11.
- Lectura de fecha y hora mediante RTC por I²C.
- LCD 16x2 en modo de cuatro bits.
- Menú con botones `UP`, `DOWN`, `OK` y `BACK`.
- Pantalla de datos actuales.
- Resumen estadístico con `Count`, `Mean`, `Std`, `Min`, `25%`, `50%`, `75%` y `Max`.
- Pronóstico de seis pasos basado en una extrapolación de tendencia de las dos últimas muestras.
- Inicialización, lectura y escritura de la tarjeta SD.
- La revisión de firmware de trabajo incluye una pantalla de configuración manual de fecha y hora; debe comprobarse en el HEX final que la edición, el guardado y la actualización de pantalla funcionen correctamente.

### Análisis con Python

- Lectura de la imagen `sdcard.mmc` usada por Proteus.
- Decodificación de fecha/hora, temperatura y humedad.
- Tabla histórica con Pandas.
- Resumen estadístico mediante `DataFrame.describe()`.
- Gráficas con Matplotlib.
- Pronóstico experimental mediante `pmdarima.auto_arima()`.
- Exportación del histórico y el pronóstico a archivos CSV.
- Comparación opcional entre el método sencillo del PIC y el modelo Python.

## Arquitectura

```text
                 ┌─────────────────────┐
                 │ DHT11               │
                 │ Temperatura/humedad │
                 └──────────┬──────────┘
                            │
┌────────────────┐  I²C     ▼       ┌─────────────┐
│ RTC            ├──────► PIC16F887 ├──► LCD 16x2 │
│ DS3232 Proteus │              │    └─────────────┘
│ DS3231 físico  │              ├────── Botones
└────────────────┘              │
                                ▼
                          ┌────────────┐
                          │ microSD    │
                          │ histórico  │
                          └─────┬──────┘
                                ▼
                        ┌──────────────────┐
                        │ Python / Jupyter │
                        │ Pandas           │
                        │ Matplotlib       │
                        │ pmdarima         │
                        └────────┬─────────┘
                                 ▼
                         CSV + gráficas +
                         pronóstico 6 pasos
```

En Proteus se utiliza el modelo DS3232. Para el montaje físico se prevé un módulo DS3231 con batería de respaldo compatible.

## Componentes

| Componente | Función |
|---|---|
| PIC16F887 | Control principal |
| DHT11 | Medición de temperatura y humedad |
| LCD 16x2 (LM016L) | Visualización de datos y menú |
| DS3232 en Proteus | RTC simulado |
| Módulo DS3231 | RTC previsto para montaje físico |
| Módulo MicroSD SPI | Almacenamiento histórico |
| Cuatro pulsadores | `UP`, `DOWN`, `OK`, `BACK` |
| Pulsador RESET | Reinicio del PIC |
| Potenciómetro de 10 kΩ | Contraste de la LCD |
| Resistencias de 4.7 kΩ | Pull-ups según el bus/sensor |
| Resistencia de 10 kΩ | Pull-up de MCLR |
| LM2596 | Regulación prevista desde batería de 9 V a 5 V |
| Adaptación de nivel para SD | Pendiente verificar según el módulo real |
| PCB y carcasa | Diseño y validación física pendientes |

## Mapa de pines

La siguiente asignación corresponde al firmware y a la simulación usada como referencia. Comprueba el pinout real de los módulos y del encapsulado antes de fabricar la PCB.

| Función | Puerto PIC | Pin DIP-40 | Conexión |
|---|---|---:|---|
| MCLR / RESET | RE3/MCLR | 1 | Pull-up de 10 kΩ a +5 V; pulsador a GND |
| DHT11 DATA | RB0 | 33 | Datos del sensor |
| Botón UP | RB1 | 34 | Pulsador hacia GND |
| Botón DOWN | RB2 | 35 | Pulsador hacia GND |
| Botón OK | RB3 | 36 | Pulsador hacia GND |
| Botón BACK | RB4 | 37 | Pulsador hacia GND |
| SD CS | RC0 | 15 | CS del módulo SD |
| SD CLK | RA0 | 2 | CLK/SCK del módulo SD |
| SD MOSI/DI | RA1 | 3 | Datos PIC hacia SD |
| SD MISO/DO | RA2 | 4 | Datos SD hacia PIC |
| LCD RS | RC2 | 17 | RS de la LCD |
| RTC SCL | RC3 | 18 | SCL del RTC |
| RTC SDA | RC4 | 23 | SDA del RTC |
| LCD E | RD3 | 22 | Enable de la LCD |
| LCD D4 | RD4 | 27 | D4 de la LCD |
| LCD D5 | RD5 | 28 | D5 de la LCD |
| LCD D6 | RD6 | 29 | D6 de la LCD |
| LCD D7 | RD7 | 30 | D7 de la LCD |
| VDD | Alimentación | 11 y 32 | +5 V regulados |
| VSS | Tierra | 12 y 31 | GND común |

