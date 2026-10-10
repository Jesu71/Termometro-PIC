#include <xc.h>
#include <stdint.h>

// ============================================================
// CONFIGURACION PIC16F887
// ============================================================

#pragma config FOSC = INTRC_NOCLKOUT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config MCLRE = ON
#pragma config CP = OFF
#pragma config CPD = OFF
#pragma config BOREN = ON
#pragma config IESO = OFF
#pragma config FCMEN = OFF
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000UL


// ============================================================
// CONFIGURACION DEL PROYECTO
// ============================================================

// UNA MEDICION CADA HORA
#define LOG_INTERVAL 3600UL

// 1 = pone hora inicial al arrancar Proteus
// 0 = conserva la hora del RTC
#define RTC_SET_START 0


// ============================================================
// LCD
// ============================================================

#define LCD_RS RC2
#define LCD_EN RD3

#define LCD_D4 RD4
#define LCD_D5 RD5
#define LCD_D6 RD6
#define LCD_D7 RD7


// ============================================================
// DHT11
// ============================================================

#define DHT_DATA RB0


// ============================================================
// BOTONES
// ============================================================

#define BTN_UP   RB1
#define BTN_DOWN RB2
#define BTN_OK   RB3
#define BTN_BACK RB4


// ============================================================
// SD - SPI POR SOFTWARE
// ============================================================

#define SD_CS   RC0
#define SD_CLK  RA0
#define SD_MOSI RA1
#define SD_MISO RA2


// ============================================================
// BLOQUES SD
// ============================================================

#define META_BLOCK 99UL
#define DATA_START 100UL


// ============================================================
// PANTALLAS
// ============================================================

#define MENU_SCREEN 0
#define DATA_SCREEN 1
#define DATE_SCREEN 2
#define STATS_SCREEN 3
#define FORECAST_SCREEN 4


// ============================================================
// CANTIDAD MAXIMA PARA ESTADISTICAS
// ============================================================
//
// No usamos histogram ni float.
//
// Solo cargamos las ultimas 24 mediciones en RAM.
// ============================================================

#define STAT_N 24


// ============================================================
// RTC
// ============================================================

typedef struct
{
    uint8_t sec;
    uint8_t min;
    uint8_t hour;

    uint8_t dow;
    uint8_t day;
    uint8_t month;
    uint8_t year;

} RTC_Time;


// ============================================================
// REGISTRO DE MEDICION
// ============================================================

typedef struct
{
    uint16_t number;

    uint8_t year;
    uint8_t month;
    uint8_t day;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    uint8_t temp;
    uint8_t hum;

} SensorRecord;


// ============================================================
// VARIABLES GLOBALES
// ============================================================

RTC_Time rtc;

SensorRecord record;


// ============================================================
// DATOS ACTUALES
// ============================================================

uint8_t temperature = 0;
uint8_t humidity = 0;

uint8_t dht_ok = 0;
uint8_t rtc_ok = 0;
uint8_t sd_ok = 0;


// ============================================================
// CONTADOR DE REGISTROS
// ============================================================

uint16_t record_count = 0;


// ============================================================
// CONTROL SD
// ============================================================
//
// 0 = direccion por BYTE
// 1 = direccion por BLOQUE
// ============================================================

uint8_t sd_block_mode = 0;


// ============================================================
// ARRAYS PARA ESTADISTICAS
// ============================================================

uint8_t stat_temp[STAT_N];
uint8_t stat_hum[STAT_N];

uint8_t stat_n = 0;


// ============================================================
// RESULTADOS ESTADISTICOS
// ============================================================

uint16_t stat_count = 0;

uint8_t stat_mean_t = 0;
uint8_t stat_mean_h = 0;

uint8_t stat_std_t = 0;
uint8_t stat_std_h = 0;

uint8_t stat_min_t = 0;
uint8_t stat_min_h = 0;

uint8_t stat_q1_t = 0;
uint8_t stat_q1_h = 0;

uint8_t stat_med_t = 0;
uint8_t stat_med_h = 0;

uint8_t stat_q3_t = 0;
uint8_t stat_q3_h = 0;

uint8_t stat_max_t = 0;
uint8_t stat_max_h = 0;


// ============================================================
// PRONOSTICO
// ============================================================

int16_t forecast_t[6];
int16_t forecast_h[6];

uint8_t forecast_ready = 0;


// ============================================================
// PROTOTIPOS LCD
// ============================================================

void LCD_Enable(void);
void LCD_Send4(uint8_t d);
void LCD_Command(uint8_t c);
void LCD_Char(uint8_t c);
void LCD_Print(const char *s);
void LCD_Clear(void);
void LCD_Cursor(uint8_t row, uint8_t col);
void LCD_Init(void);
void LCD_Num2(uint8_t n);
void LCD_Num3(uint16_t n);
void LCD_Line(const char *s);


// ============================================================
// PROTOTIPOS DHT11
// ============================================================

void Timer1_Init(void);

uint8_t DHT_Wait(
    uint8_t state,
    uint16_t limit
);

uint8_t DHT_ReadByte(
    uint8_t *value
);

uint8_t DHT_Read(
    uint8_t *hum,
    uint8_t *temp
);


// ============================================================
// PROTOTIPOS I2C / RTC
// ============================================================

void I2C_Init(void);
void I2C_Wait(void);
void I2C_Start(void);
void I2C_Restart(void);
void I2C_Stop(void);

uint8_t I2C_Write(
    uint8_t d
);

uint8_t I2C_Read(
    uint8_t ack
);

uint8_t BCD_Dec(
    uint8_t d
);

uint8_t Dec_BCD(
    uint8_t d
);

uint8_t RTC_Set(
    uint8_t h,
    uint8_t m,
    uint8_t s,
    uint8_t dow,
    uint8_t day,
    uint8_t month,
    uint8_t year
);

uint8_t RTC_Read(
    RTC_Time *r
);

uint32_t RTC_Seconds(
    RTC_Time *r
);


// ============================================================
// PROTOTIPOS SD
// ============================================================

void SD_SPI_Init(void);

uint8_t SD_SPI(
    uint8_t d
);

uint8_t SD_Response(void);

uint8_t SD_Command(
    uint8_t cmd,
    uint32_t arg,
    uint8_t crc
);

uint8_t SD_Init(void);

uint32_t SD_Address(
    uint32_t block
);

uint8_t SD_SetBlock(void);

uint8_t SD_NotBusy(void);

uint8_t SD_WriteBlock(
    uint32_t block,
    const uint8_t *data,
    uint16_t len
);

uint8_t SD_ReadBlock(
    uint32_t block,
    uint8_t *data,
    uint16_t len
);

uint8_t SD_WriteMeta(
    uint16_t count
);

uint8_t SD_ReadMeta(
    uint16_t *count
);

uint8_t SD_WriteRecord(
    uint32_t block,
    SensorRecord *r
);

uint8_t SD_ReadRecord(
    uint32_t block,
    SensorRecord *r
);


// ============================================================
// ESTADISTICAS
// ============================================================

void SortArray(
    uint8_t *a,
    uint8_t n
);

uint8_t IntSqrt(
    uint32_t x
);

uint8_t StatPercentile(
    uint8_t *a,
    uint8_t n,
    uint8_t p
);

uint8_t BuildStats(void);


// ============================================================
// PRONOSTICO
// ============================================================

uint8_t BuildForecast(void);


// ============================================================
// BOTONES
// ============================================================

uint8_t Button(void);


// ============================================================
// PANTALLAS
// ============================================================

void ShowMenu(
    uint8_t option
);

void ShowData(void);

void ShowDate(void);

void ShowStats(
    uint8_t page
);

void ShowForecast(
    uint8_t page
);

void ConfigureRTC(void);


// ============================================================
// GUARDAR MEDICION
// ============================================================

uint8_t SaveMeasurement(void);


// ============================================================
// LCD
// ============================================================

void LCD_Enable(void)
{
    LCD_EN = 1;

    __delay_us(5);

    LCD_EN = 0;

    __delay_us(100);
}


// ============================================================

void LCD_Send4(uint8_t d)
{
    LCD_D4 = (d >> 0) & 1;
    LCD_D5 = (d >> 1) & 1;
    LCD_D6 = (d >> 2) & 1;
    LCD_D7 = (d >> 3) & 1;

    LCD_Enable();
}


// ============================================================

void LCD_Command(uint8_t c)
{
    LCD_RS = 0;

    LCD_Send4(
        c >> 4
    );

    LCD_Send4(
        c & 0x0F
    );

    if(
        c == 0x01 ||
        c == 0x02
    )
    {
        __delay_ms(2);
    }
}


// ============================================================

void LCD_Char(uint8_t c)
{
    LCD_RS = 1;

    LCD_Send4(
        c >> 4
    );

    LCD_Send4(
        c & 0x0F
    );
}


// ============================================================

void LCD_Print(const char *s)
{
    while(*s)
    {
        LCD_Char(
            (uint8_t)*s
        );

        s++;
    }
}


// ============================================================

void LCD_Clear(void)
{
    LCD_Command(0x01);

    __delay_ms(2);
}


// ============================================================

void LCD_Cursor(
    uint8_t row,
    uint8_t col
)
{
    if(row == 1)
    {
        LCD_Command(
            0x80 + col
        );
    }
    else
    {
        LCD_Command(
            0xC0 + col
        );
    }
}


// ============================================================

void LCD_Num2(
    uint8_t n
)
{
    LCD_Char(
        (uint8_t)(
            (n / 10) + '0'
        )
    );

    LCD_Char(
        (uint8_t)(
            (n % 10) + '0'
        )
    );
}


// ============================================================

void LCD_Num3(
    uint16_t n
)
{
    if(n > 999)
    {
        n %= 1000;
    }

    if(n >= 100)
    {
        LCD_Char(
            (uint8_t)(
                (n / 100) + '0'
            )
        );

        LCD_Char(
            (uint8_t)(
                ((n / 10) % 10) + '0'
            )
        );

        LCD_Char(
            (uint8_t)(
                (n % 10) + '0'
            )
        );
    }
    else if(n >= 10)
    {
        LCD_Char(
            (uint8_t)(
                (n / 10) + '0'
            )
        );

        LCD_Char(
            (uint8_t)(
                (n % 10) + '0'
            )
        );
    }
    else
    {
        LCD_Char(
            (uint8_t)(
                n + '0'
            )
        );
    }
}


// ============================================================

void LCD_Line(
    const char *s
)
{
    uint8_t n = 0;

    while(
        *s &&
        n < 16
    )
    {
        LCD_Char(
            (uint8_t)*s
        );

        s++;

        n++;
    }

    while(n < 16)
    {
        LCD_Char(' ');

        n++;
    }
}


// ============================================================

void LCD_Init(void)
{
    LCD_RS = 0;
    LCD_EN = 0;

    __delay_ms(30);

    LCD_Send4(0x03);
    __delay_ms(5);

    LCD_Send4(0x03);
    __delay_us(150);

    LCD_Send4(0x03);
    __delay_us(150);

    LCD_Send4(0x02);

    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);

    __delay_ms(5);
}


// ============================================================
// TIMER1
// ============================================================

void Timer1_Init(void)
{
    T1CON = 0x00;

    TMR1H = 0;
    TMR1L = 0;
}


// ============================================================
// DHT11
// ============================================================

uint8_t DHT_Wait(
    uint8_t state,
    uint16_t limit
)
{
    uint16_t count = 0;

    while(
        DHT_DATA != state
    )
    {
        __delay_us(1);

        count++;

        if(
            count >= limit
        )
        {
            return 0;
        }
    }

    return 1;
}


// ============================================================

uint8_t DHT_ReadByte(
    uint8_t *value
)
{
    uint8_t i;

    uint8_t data = 0;

    uint16_t time;


    for(
        i = 0;
        i < 8;
        i++
    )
    {
        if(
            !DHT_Wait(
                1,
                100
            )
        )
        {
            return 0;
        }


        TMR1H = 0;
        TMR1L = 0;


        T1CONbits.TMR1ON = 1;


        while(DHT_DATA)
        {
            if(
                TMR1H > 1
            )
            {
                T1CONbits.TMR1ON = 0;

                return 0;
            }
        }


        T1CONbits.TMR1ON = 0;


        time =
            ((uint16_t)TMR1H << 8)
            |
            TMR1L;


        data <<= 1;


        if(
            time > 90
        )
        {
            data |= 1;
        }
    }


    *value = data;

    return 1;
}


// ============================================================

uint8_t DHT_Read(
    uint8_t *hum,
    uint8_t *temp
)
{
    uint8_t data[5];

    uint8_t i;


    TRISB0 = 0;

    DHT_DATA = 0;

    __delay_ms(20);

    DHT_DATA = 1;

    __delay_us(30);

    TRISB0 = 1;


    if(
        !DHT_Wait(
            0,
            120
        )
    )
    {
        return 0;
    }


    if(
        !DHT_Wait(
            1,
            120
        )
    )
    {
        return 0;
    }


    if(
        !DHT_Wait(
            0,
            120
        )
    )
    {
        return 0;
    }


    for(
        i = 0;
        i < 5;
        i++
    )
    {
        if(
            !DHT_ReadByte(
                &data[i]
            )
        )
        {
            return 0;
        }
    }


    if(
        (uint8_t)(
            data[0] +
            data[1] +
            data[2] +
            data[3]
        )
        !=
        data[4]
    )
    {
        return 0;
    }


    *hum = data[0];

    *temp = data[2];


    return 1;
}


// ============================================================
// I2C
// ============================================================

void I2C_Init(void)
{
    TRISC3 = 1;
    TRISC4 = 1;

    SSPSTAT = 0x80;

    SSPADD = 19;

    SSPCON = 0x28;

    SSPCON2 = 0x00;
}


// ============================================================

void I2C_Wait(void)
{
    while(
        (SSPCON2 & 0x1F)
        ||
        SSPSTATbits.R_W
    )
    {
        ;
    }
}


// ============================================================

void I2C_Start(void)
{
    I2C_Wait();

    SSPCON2bits.SEN = 1;

    while(
        SSPCON2bits.SEN
    )
    {
        ;
    }
}


// ============================================================

void I2C_Restart(void)
{
    I2C_Wait();

    SSPCON2bits.RSEN = 1;

    while(
        SSPCON2bits.RSEN
    )
    {
        ;
    }
}


// ============================================================

void I2C_Stop(void)
{
    I2C_Wait();

    SSPCON2bits.PEN = 1;

    while(
        SSPCON2bits.PEN
    )
    {
        ;
    }
}


// ============================================================

uint8_t I2C_Write(
    uint8_t d
)
{
    I2C_Wait();

    PIR1bits.SSPIF = 0;

    SSPBUF = d;

    while(
        !PIR1bits.SSPIF
    )
    {
        ;
    }

    PIR1bits.SSPIF = 0;

    return(
        SSPCON2bits.ACKSTAT == 0
    );
}


// ============================================================

uint8_t I2C_Read(
    uint8_t ack
)
{
    uint8_t d;


    I2C_Wait();

    PIR1bits.SSPIF = 0;

    SSPCON2bits.RCEN = 1;


    while(
        !PIR1bits.SSPIF
    )
    {
        ;
    }


    d = SSPBUF;

    PIR1bits.SSPIF = 0;


    SSPCON2bits.ACKDT =
        ack ? 0 : 1;


    SSPCON2bits.ACKEN = 1;


    while(
        SSPCON2bits.ACKEN
    )
    {
        ;
    }


    return d;
}


// ============================================================
// BCD
// ============================================================

uint8_t BCD_Dec(
    uint8_t d
)
{
    return(
        (uint8_t)(
            ((d >> 4) * 10)
            +
            (d & 0x0F)
        )
    );
}


// ============================================================

uint8_t Dec_BCD(
    uint8_t d
)
{
    return(
        (uint8_t)(
            ((d / 10) << 4)
            |
            (d % 10)
        )
    );
}


// ============================================================
// RTC ESCRIBIR
// ============================================================

uint8_t RTC_Set(
    uint8_t h,
    uint8_t m,
    uint8_t s,
    uint8_t dow,
    uint8_t day,
    uint8_t month,
    uint8_t year
)
{
    uint8_t ok = 1;


    I2C_Start();


    if(
        !I2C_Write(0xD0)
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(0x00)
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(
            Dec_BCD(s)
        )
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(
            Dec_BCD(m)
        )
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(
            Dec_BCD(h)
        )
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(
            dow
        )
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(
            Dec_BCD(day)
        )
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(
            Dec_BCD(month)
        )
    )
    {
        ok = 0;
    }


    if(
        ok &&
        !I2C_Write(
            Dec_BCD(year)
        )
    )
    {
        ok = 0;
    }


    I2C_Stop();


    return ok;
}


// ============================================================
// RTC LEER
// ============================================================

uint8_t RTC_Read(
    RTC_Time *r
)
{
    uint8_t d;


    I2C_Start();


    if(
        !I2C_Write(0xD0)
        ||
        !I2C_Write(0x00)
    )
    {
        I2C_Stop();

        return 0;
    }


    I2C_Restart();


    if(
        !I2C_Write(0xD1)
    )
    {
        I2C_Stop();

        return 0;
    }


    d = I2C_Read(1);

    r->sec =
        BCD_Dec(
            d & 0x7F
        );


    d = I2C_Read(1);

    r->min =
        BCD_Dec(
            d & 0x7F
        );


    d = I2C_Read(1);

    r->hour =
        BCD_Dec(
            d & 0x3F
        );


    d = I2C_Read(1);

    r->dow =
        d & 0x07;


    d = I2C_Read(1);

    r->day =
        BCD_Dec(
            d & 0x3F
        );


    d = I2C_Read(1);

    r->month =
        BCD_Dec(
            d & 0x1F
        );


    d = I2C_Read(0);

    r->year =
        BCD_Dec(d);


    I2C_Stop();


    return 1;
}


// ============================================================

uint32_t RTC_Seconds(
    RTC_Time *r
)
{
    return(
        ((uint32_t)r->hour * 3600UL)
        +
        ((uint32_t)r->min * 60UL)
        +
        r->sec
    );
}


// ============================================================
// SD SPI
// ============================================================

void SD_SPI_Init(void)
{
    TRISC0 = 0;

    TRISA0 = 0;

    TRISA1 = 0;

    TRISA2 = 1;


    SD_CS = 1;

    SD_CLK = 0;

    SD_MOSI = 1;
}


// ============================================================

uint8_t SD_SPI(
    uint8_t d
)
{
    uint8_t i;

    uint8_t r = 0;


    for(
        i = 0;
        i < 8;
        i++
    )
    {
        if(
            d & 0x80
        )
        {
            SD_MOSI = 1;
        }
        else
        {
            SD_MOSI = 0;
        }


        d <<= 1;


        SD_CLK = 1;

        __delay_us(2);


        r <<= 1;


        if(SD_MISO)
        {
            r |= 1;
        }


        SD_CLK = 0;

        __delay_us(2);
    }


    return r;
}


// ============================================================

uint8_t SD_Response(void)
{
    uint16_t count = 0;

    uint8_t r;


    while(
        count < 10000
    )
    {
        r =
            SD_SPI(0xFF);


        if(
            r != 0xFF
        )
        {
            return r;
        }


        count++;
    }


    return 0xFF;
}


// ============================================================

uint8_t SD_Command(
    uint8_t cmd,
    uint32_t arg,
    uint8_t crc
)
{
    SD_SPI(
        0x40 | cmd
    );


    SD_SPI(
        (uint8_t)(
            arg >> 24
        )
    );


    SD_SPI(
        (uint8_t)(
            arg >> 16
        )
    );


    SD_SPI(
        (uint8_t)(
            arg >> 8
        )
    );


    SD_SPI(
        (uint8_t)arg
    );


    SD_SPI(crc);


    return SD_Response();
}


// ============================================================
// SD INIT
// ============================================================

uint8_t SD_Init(void)
{
    uint8_t r;

    uint8_t i;

    uint8_t r7[4];

    uint8_t ocr[4];

    uint16_t tries;


    sd_block_mode = 0;


    __delay_ms(100);


    SD_CS = 1;


    for(
        i = 0;
        i < 10;
        i++
    )
    {
        SD_SPI(0xFF);
    }


    SD_CS = 0;


    // --------------------------------------------------------
    // CMD0
    // --------------------------------------------------------

    r =
        SD_Command(
            0,
            0,
            0x95
        );


    if(
        r != 0x01
    )
    {
        SD_CS = 1;

        SD_SPI(0xFF);

        return 0;
    }


    // --------------------------------------------------------
    // CMD8
    // --------------------------------------------------------

    r =
        SD_Command(
            8,
            0x000001AAUL,
            0x87
        );


    // --------------------------------------------------------
    // SD V2
    // --------------------------------------------------------

    if(
        r == 0x01
    )
    {
        for(
            i = 0;
            i < 4;
            i++
        )
        {
            r7[i] =
                SD_SPI(0xFF);
        }


        if(
            r7[2] != 0x01 ||
            r7[3] != 0xAA
        )
        {
            SD_CS = 1;

            SD_SPI(0xFF);

            return 0;
        }


        tries = 0;


        while(
            tries < 2000
        )
        {
            (void)SD_Command(
                55,
                0,
                0x01
            );


            r =
                SD_Command(
                    41,
                    0x40000000UL,
                    0x01
                );


            if(
                r == 0x00
            )
            {
                break;
            }


            if(
                r != 0x01
            )
            {
                SD_CS = 1;

                SD_SPI(0xFF);

                return 0;
            }


            tries++;

            __delay_ms(1);
        }


        if(
            tries >= 2000
        )
        {
            SD_CS = 1;

            SD_SPI(0xFF);

            return 0;
        }


        // CMD58

        r =
            SD_Command(
                58,
                0,
                0x01
            );


        if(
            r != 0x00
        )
        {
            SD_CS = 1;

            SD_SPI(0xFF);

            return 0;
        }


        for(
            i = 0;
            i < 4;
            i++
        )
        {
            ocr[i] =
                SD_SPI(0xFF);
        }


        if(
            ocr[0] & 0x40
        )
        {
            sd_block_mode = 1;
        }


        SD_CS = 1;

        SD_SPI(0xFF);


        return 1;
    }


    // --------------------------------------------------------
    // SD V1 / MMC
    // --------------------------------------------------------

    tries = 0;


    while(
        tries < 1000
    )
    {
        r =
            SD_Command(
                55,
                0,
                0x01
            );


        if(
            r == 0x01
        )
        {
            r =
                SD_Command(
                    41,
                    0,
                    0x01
                );


            if(
                r == 0x00
            )
            {
                SD_CS = 1;

                SD_SPI(0xFF);

                sd_block_mode = 0;

                return 1;
            }


            if(
                r != 0x01
            )
            {
                break;
            }
        }


        tries++;

        __delay_ms(1);
    }


    // --------------------------------------------------------
    // MMC CMD1
    // --------------------------------------------------------

    tries = 0;


    while(
        tries < 2000
    )
    {
        r =
            SD_Command(
                1,
                0,
                0xFF
            );


        if(
            r == 0x00
        )
        {
            SD_CS = 1;

            SD_SPI(0xFF);

            sd_block_mode = 0;

            return 1;
        }


        if(
            r != 0x01
        )
        {
            break;
        }


        tries++;

        __delay_ms(1);
    }


    SD_CS = 1;

    SD_SPI(0xFF);


    return 0;
}


// ============================================================
// DIRECCION DEL BLOQUE
// ============================================================

uint32_t SD_Address(
    uint32_t block
)
{
    if(
        sd_block_mode
    )
    {
        return block;
    }


    return(
        block * 512UL
    );
}


// ============================================================
// CMD16
// ============================================================

uint8_t SD_SetBlock(void)
{
    uint8_t r;


    if(
        sd_block_mode
    )
    {
        return 1;
    }


    SD_CS = 0;


    r =
        SD_Command(
            16,
            512UL,
            0xFF
        );


    SD_CS = 1;

    SD_SPI(0xFF);


    return(
        r == 0x00
    );
}


// ============================================================
// ESPERAR NO BUSY
// ============================================================

uint8_t SD_NotBusy(void)
{
    uint16_t count = 0;

    uint8_t d;


    while(
        count < 20000
    )
    {
        d =
            SD_SPI(0xFF);


        if(
            d == 0xFF
        )
        {
            return 1;
        }


        count++;
    }


    return 0;
}


// ============================================================
// WRITE BLOCK
// ============================================================

uint8_t SD_WriteBlock(
    uint32_t block,
    const uint8_t *data,
    uint16_t len
)
{
    uint8_t r;

    uint16_t i;


    if(
        len > 512
    )
    {
        return 0;
    }


    SD_CS = 0;


    r =
        SD_Command(
            24,
            SD_Address(block),
            0xFF
        );


    if(
        r != 0x00
    )
    {
        SD_CS = 1;

        SD_SPI(0xFF);

        return 0;
    }


    SD_SPI(0xFE);


    for(
        i = 0;
        i < len;
        i++
    )
    {
        SD_SPI(
            data[i]
        );
    }


    while(
        i < 512
    )
    {
        SD_SPI(0xFF);

        i++;
    }


    SD_SPI(0xFF);
    SD_SPI(0xFF);


    r =
        SD_SPI(0xFF);


    if(
        (r & 0x1F) != 0x05
    )
    {
        SD_CS = 1;

        SD_SPI(0xFF);

        return 0;
    }


    if(
        !SD_NotBusy()
    )
    {
        SD_CS = 1;

        SD_SPI(0xFF);

        return 0;
    }


    SD_CS = 1;

    SD_SPI(0xFF);


    return 1;
}


// ============================================================
// READ BLOCK
// ============================================================

uint8_t SD_ReadBlock(
    uint32_t block,
    uint8_t *data,
    uint16_t len
)
{
    uint8_t r;

    uint8_t d;

    uint16_t i;

    uint16_t timeout = 0;


    if(
        len > 512
    )
    {
        return 0;
    }


    SD_CS = 0;


    r =
        SD_Command(
            17,
            SD_Address(block),
            0xFF
        );


    if(
        r != 0x00
    )
    {
        SD_CS = 1;

        SD_SPI(0xFF);

        return 0;
    }


    while(
        timeout < 20000
    )
    {
        d =
            SD_SPI(0xFF);


        if(
            d == 0xFE
        )
        {
            break;
        }


        timeout++;
    }


    if(
        timeout >= 20000
    )
    {
        SD_CS = 1;

        SD_SPI(0xFF);

        return 0;
    }


    for(
        i = 0;
        i < 512;
        i++
    )
    {
        d =
            SD_SPI(0xFF);


        if(
            i < len
        )
        {
            data[i] = d;
        }
    }


    SD_SPI(0xFF);
    SD_SPI(0xFF);


    SD_CS = 1;

    SD_SPI(0xFF);


    return 1;
}


// ============================================================
// METADATA
// ============================================================

uint8_t SD_WriteMeta(
    uint16_t count
)
{
    uint8_t data[6];


    data[0] = 'T';

    data[1] = 'M';

    data[2] = 1;

    data[3] = 0;

    data[4] =
        (uint8_t)count;

    data[5] =
        (uint8_t)(
            count >> 8
        );


    return(
        SD_WriteBlock(
            META_BLOCK,
            data,
            6
        )
    );
}


// ============================================================

uint8_t SD_ReadMeta(
    uint16_t *count
)
{
    uint8_t data[6];


    if(
        !SD_ReadBlock(
            META_BLOCK,
            data,
            6
        )
    )
    {
        return 0;
    }


    if(
        data[0] != 'T' ||
        data[1] != 'M' ||
        data[2] != 1
    )
    {
        *count = 0;

        return 0;
    }


    *count =
        (uint16_t)data[4]
        |
        ((uint16_t)data[5] << 8);


    return 1;
}


// ============================================================
// WRITE RECORD
// ============================================================

uint8_t SD_WriteRecord(
    uint32_t block,
    SensorRecord *r
)
{
    uint8_t data[12];


    data[0] = 0xA5;

    data[1] = 1;


    data[2] =
        (uint8_t)r->number;

    data[3] =
        (uint8_t)(
            r->number >> 8
        );


    data[4] =
        r->year;

    data[5] =
        r->month;

    data[6] =
        r->day;


    data[7] =
        r->hour;

    data[8] =
        r->minute;

    data[9] =
        r->second;


    data[10] =
        r->temp;

    data[11] =
        r->hum;


    return(
        SD_WriteBlock(
            block,
            data,
            12
        )
    );
}


// ============================================================
// READ RECORD
// ============================================================

uint8_t SD_ReadRecord(
    uint32_t block,
    SensorRecord *r
)
{
    uint8_t data[12];


    if(
        !SD_ReadBlock(
            block,
            data,
            12
        )
    )
    {
        return 0;
    }


    if(
        data[0] != 0xA5 ||
        data[1] != 1
    )
    {
        return 0;
    }


    r->number =
        (uint16_t)data[2]
        |
        ((uint16_t)data[3] << 8);


    r->year =
        data[4];

    r->month =
        data[5];

    r->day =
        data[6];


    r->hour =
        data[7];

    r->minute =
        data[8];

    r->second =
        data[9];


    r->temp =
        data[10];

    r->hum =
        data[11];


    return 1;
}


// ============================================================
// ORDENAR
// ============================================================

void SortArray(
    uint8_t *a,
    uint8_t n
)
{
    uint8_t i;
    uint8_t j;
    uint8_t temp;


    for(
        i = 0;
        i < n;
        i++
    )
    {
        for(
            j = 0;
            j < (uint8_t)(n - 1);
            j++
        )
        {
            if(
                a[j] >
                a[j + 1]
            )
            {
                temp = a[j];

                a[j] =
                    a[j + 1];

                a[j + 1] =
                    temp;
            }
        }
    }
}


// ============================================================
// RAIZ CUADRADA ENTERA
// ============================================================

uint8_t IntSqrt(
    uint32_t x
)
{
    uint8_t n;


    for(
        n = 0;
        n < 255;
        n++
    )
    {
        if(
            ((uint32_t)n *
             (uint32_t)n)
            >
            x
        )
        {
            return(
                (uint8_t)(
                    n - 1
                )
            );
        }
    }


    return 255;
}


// ============================================================
// PERCENTIL
// ============================================================

uint8_t StatPercentile(
    uint8_t *a,
    uint8_t n,
    uint8_t p
)
{
    uint16_t index;


    if(
        n == 0
    )
    {
        return 0;
    }


    index =
        (
            (uint16_t)(n - 1) *
            p
        )
        /
        100U;


    return(
        a[index]
    );
}


// ============================================================
// CONSTRUIR ESTADISTICAS
// ============================================================

uint8_t BuildStats(void)
{
    uint16_t start;

    uint16_t i;

    uint32_t sumT = 0;
    uint32_t sumH = 0;

    uint32_t sqT = 0;
    uint32_t sqH = 0;

    uint32_t varianceT;
    uint32_t varianceH;

    SensorRecord r;


    if(
        record_count == 0
    )
    {
        return 0;
    }


    // Ultimas 24 mediciones

    if(
        record_count > STAT_N
    )
    {
        start =
            record_count -
            STAT_N;
    }
    else
    {
        start = 0;
    }


    stat_n =
        (uint8_t)(
            record_count -
            start
        );


    stat_count =
        record_count;


    // --------------------------------------------------------
    // Cargar datos
    // --------------------------------------------------------

    for(
        i = 0;
        i < stat_n;
        i++
    )
    {
        if(
            !SD_ReadRecord(
                DATA_START +
                start +
                i,
                &r
            )
        )
        {
            return 0;
        }


        stat_temp[i] =
            r.temp;

        stat_hum[i] =
            r.hum;


        sumT +=
            r.temp;

        sumH +=
            r.hum;


        sqT +=
            (uint32_t)r.temp *
            r.temp;

        sqH +=
            (uint32_t)r.hum *
            r.hum;


        if(
            i == 0
        )
        {
            stat_min_t =
                r.temp;

            stat_max_t =
                r.temp;

            stat_min_h =
                r.hum;

            stat_max_h =
                r.hum;
        }
        else
        {
            if(
                r.temp <
                stat_min_t
            )
            {
                stat_min_t =
                    r.temp;
            }

            if(
                r.temp >
                stat_max_t
            )
            {
                stat_max_t =
                    r.temp;
            }

            if(
                r.hum <
                stat_min_h
            )
            {
                stat_min_h =
                    r.hum;
            }

            if(
                r.hum >
                stat_max_h
            )
            {
                stat_max_h =
                    r.hum;
            }
        }
    }


    // --------------------------------------------------------
    // MEDIA
    // --------------------------------------------------------

    stat_mean_t =
        (uint8_t)(
            sumT /
            stat_n
        );


    stat_mean_h =
        (uint8_t)(
            sumH /
            stat_n
        );


    // --------------------------------------------------------
    // VARIANZA
    // --------------------------------------------------------

    varianceT =
        (
            sqT * stat_n -
            sumT * sumT
        )
        /
        (
            (uint32_t)stat_n *
            stat_n
        );


    varianceH =
        (
            sqH * stat_n -
            sumH * sumH
        )
        /
        (
            (uint32_t)stat_n *
            stat_n
        );


    stat_std_t =
        IntSqrt(
            varianceT
        );


    stat_std_h =
        IntSqrt(
            varianceH
        );


    // --------------------------------------------------------
    // ORDENAR
    // --------------------------------------------------------

    SortArray(
        stat_temp,
        stat_n
    );


    SortArray(
        stat_hum,
        stat_n
    );


    // --------------------------------------------------------
    // PERCENTILES
    // --------------------------------------------------------

    stat_q1_t =
        StatPercentile(
            stat_temp,
            stat_n,
            25
        );


    stat_med_t =
        StatPercentile(
            stat_temp,
            stat_n,
            50
        );


    stat_q3_t =
        StatPercentile(
            stat_temp,
            stat_n,
            75
        );


    stat_q1_h =
        StatPercentile(
            stat_hum,
            stat_n,
            25
        );


    stat_med_h =
        StatPercentile(
            stat_hum,
            stat_n,
            50
        );


    stat_q3_h =
        StatPercentile(
            stat_hum,
            stat_n,
            75
        );


    return 1;
}


// ============================================================
// PRONOSTICO
// ============================================================
//
// Para evitar float y ahorrar memoria,
// usamos la tendencia de las dos ultimas muestras.
//
// T+1 = ultimo + delta
// T+2 = ultimo + 2*delta
// ...
// T+6 = ultimo + 6*delta
//
// ============================================================

uint8_t BuildForecast(void)
{
    SensorRecord last;
    SensorRecord previous;

    int16_t deltaT;
    int16_t deltaH;

    uint8_t i;

    int16_t t;
    int16_t h;


    forecast_ready = 0;


    if(
        record_count < 2
    )
    {
        return 0;
    }


    if(
        !SD_ReadRecord(
            DATA_START +
            record_count -
            1,
            &last
        )
    )
    {
        return 0;
    }


    if(
        !SD_ReadRecord(
            DATA_START +
            record_count -
            2,
            &previous
        )
    )
    {
        return 0;
    }


    deltaT =
        (int16_t)last.temp -
        (int16_t)previous.temp;


    deltaH =
        (int16_t)last.hum -
        (int16_t)previous.hum;


    for(
        i = 0;
        i < 6;
        i++
    )
    {
        t =
            (int16_t)last.temp +
            deltaT *
            (int16_t)(i + 1);


        h =
            (int16_t)last.hum +
            deltaH *
            (int16_t)(i + 1);


        if(
            t < 0
        )
        {
            t = 0;
        }


        if(
            t > 50
        )
        {
            t = 50;
        }


        if(
            h < 0
        )
        {
            h = 0;
        }


        if(
            h > 100
        )
        {
            h = 100;
        }


        forecast_t[i] =
            t;

        forecast_h[i] =
            h;
    }


    forecast_ready = 1;


    return 1;
}


// ============================================================
// BOTONES
// ============================================================

uint8_t Button(void)
{
    if(
        BTN_UP == 0
    )
    {
        __delay_ms(30);

        if(
            BTN_UP == 0
        )
        {
            while(
                BTN_UP == 0
            )
            {
                ;
            }

            return 1;
        }
    }


    if(
        BTN_DOWN == 0
    )
    {
        __delay_ms(30);

        if(
            BTN_DOWN == 0
        )
        {
            while(
                BTN_DOWN == 0
            )
            {
                ;
            }

            return 2;
        }
    }


    if(
        BTN_OK == 0
    )
    {
        __delay_ms(30);

        if(
            BTN_OK == 0
        )
        {
            while(
                BTN_OK == 0
            )
            {
                ;
            }

            return 3;
        }
    }


    if(
        BTN_BACK == 0
    )
    {
        __delay_ms(30);

        if(
            BTN_BACK == 0
        )
        {
            while(
                BTN_BACK == 0
            )
            {
                ;
            }

            return 4;
        }
    }


    return 0;
}


// ============================================================
// MENU
// ============================================================

void ShowMenu(
    uint8_t option
)
{
    LCD_Clear();


    LCD_Cursor(
        1,
        0
    );

    LCD_Line(
        "      MENU"
    );


    LCD_Cursor(
        2,
        0
    );


    if(
        option == 0
    )
    {
        LCD_Line(
            ">DATOS ACTUALES"
        );
    }
    else if(
        option == 1
    )
    {
        LCD_Line(
            ">ESTADISTICAS"
        );
    }
    else if(
        option == 2
    )
    {
        LCD_Line(
            ">PRONOSTICO"
        );
    }
    else
    {
        LCD_Line(
            ">CONFIGURACION"
        );
    }
}


// ============================================================
// CONFIGURACION DE HORA
// ============================================================

void ConfigureRTC(void)
{
    uint8_t field = 0;
    uint8_t h = rtc.hour;
    uint8_t m = rtc.min;
    uint8_t d = rtc.day;
    uint8_t mo = rtc.month;
    uint8_t y = rtc.year;
    uint8_t b;
    uint8_t redraw = 1;

    while(1)
    {
        /* Redibujar solo cuando cambia el campo o el valor.
           Esto evita el parpadeo del LCD. */
        if(redraw)
        {
            LCD_Clear();

            if(field == 0)
            {
                LCD_Line("HORA");
                LCD_Cursor(2,0);
                LCD_Num2(h);
                LCD_Char(':');
                LCD_Num2(m);
                LCD_Print("   AJUSTE");
            }
            else if(field == 1)
            {
                LCD_Line("MINUTOS");
                LCD_Cursor(2,0);
                LCD_Num2(h);
                LCD_Char(':');
                LCD_Num2(m);
                LCD_Print("   AJUSTE");
            }
            else if(field == 2)
            {
                LCD_Line("DIA");
                LCD_Cursor(2,0);
                LCD_Num2(d);
                LCD_Char('/');
                LCD_Num2(mo);
                LCD_Print("/20");
                LCD_Num2(y);
            }
            else if(field == 3)
            {
                LCD_Line("MES");
                LCD_Cursor(2,0);
                LCD_Num2(d);
                LCD_Char('/');
                LCD_Num2(mo);
                LCD_Print("/20");
                LCD_Num2(y);
            }
            else
            {
                LCD_Line("ANO");
                LCD_Cursor(2,0);
                LCD_Print("20");
                LCD_Num2(y);
                LCD_Print("  ");
                LCD_Num2(d);
                LCD_Char('/');
                LCD_Num2(mo);
            }

            redraw = 0;
        }

        b = Button();

        /* UP */
        if(b == 1)
        {
            if(field == 0)
            {
                if(h == 0) h = 23;
                else h--;
            }
            else if(field == 1)
            {
                if(m == 0) m = 59;
                else m--;
            }
            else if(field == 2)
            {
                if(d <= 1) d = 31;
                else d--;
            }
            else if(field == 3)
            {
                if(mo <= 1) mo = 12;
                else mo--;
            }
            else
            {
                if(y == 0) y = 99;
                else y--;
            }

            redraw = 1;
        }

        /* DOWN */
        else if(b == 2)
        {
            if(field == 0)
            {
                h++;
                if(h > 23) h = 0;
            }
            else if(field == 1)
            {
                m++;
                if(m > 59) m = 0;
            }
            else if(field == 2)
            {
                d++;
                if(d > 31) d = 1;
            }
            else if(field == 3)
            {
                mo++;
                if(mo > 12) mo = 1;
            }
            else
            {
                y++;
                if(y > 99) y = 0;
            }

            redraw = 1;
        }

        /* OK */
        else if(b == 3)
        {
            if(field < 4)
            {
                field++;
                redraw = 1;
            }
            else
            {
                if(RTC_Set(h,m,0,rtc.dow,d,mo,y))
                {
                    rtc.hour = h;
                    rtc.min = m;
                    rtc.sec = 0;
                    rtc.day = d;
                    rtc.month = mo;
                    rtc.year = y;

                    LCD_Clear();
                    LCD_Line("RTC GUARDADO");
                    LCD_Cursor(2,0);
                    LCD_Num2(h);
                    LCD_Char(':');
                    LCD_Num2(m);
                    LCD_Char(':');
                    LCD_Num2(0);
                    __delay_ms(800);
                }
                else
                {
                    LCD_Clear();
                    LCD_Line("ERROR RTC");
                    LCD_Cursor(2,0);
                    LCD_Line("NO GUARDADO");
                    __delay_ms(800);
                }

                return;
            }
        }

        /* BACK */
        else if(b == 4)
        {
            return;
        }
    }
}


// ============================================================
// DATOS
// ============================================================

void ShowData(void)
{
    LCD_Cursor(
        1,
        0
    );


    LCD_Print("T:");

    LCD_Num2(
        temperature
    );

    LCD_Print("C H:");

    LCD_Num2(
        humidity
    );

    LCD_Print("% ");


    LCD_Cursor(
        2,
        0
    );


    LCD_Print("HORA ");

    LCD_Num2(
        rtc.hour
    );

    LCD_Char(':');

    LCD_Num2(
        rtc.min
    );

    LCD_Char(':');

    LCD_Num2(
        rtc.sec
    );

    LCD_Print(" ");
}


// ============================================================
// FECHA
// ============================================================

void ShowDate(void)
{
    LCD_Cursor(
        1,
        0
    );


    LCD_Print("FECHA ");

    LCD_Num2(
        rtc.day
    );

    LCD_Char('/');

    LCD_Num2(
        rtc.month
    );

    LCD_Char('/');

    LCD_Num2(
        rtc.year
    );


    LCD_Cursor(
        2,
        0
    );


    LCD_Print("REG:");

    LCD_Num3(
        record_count
    );

    LCD_Print("   ");
}


// ============================================================
// ESTADISTICAS
// ============================================================

void ShowStats(
    uint8_t page
)
{
    uint8_t t;
    uint8_t h;

    if(page == 0)
    {
        LCD_Clear();
        LCD_Line("COUNT");
        LCD_Cursor(2,0);
        LCD_Print("N:");
        LCD_Num3(stat_count);
        return;
    }

    switch(page)
    {
        case 1:
            LCD_Line("MEAN");
            t = stat_mean_t;
            h = stat_mean_h;
            break;

        case 2:
            LCD_Line("STD");
            t = stat_std_t;
            h = stat_std_h;
            break;

        case 3:
            LCD_Line("MIN");
            t = stat_min_t;
            h = stat_min_h;
            break;

        case 4:
            LCD_Line("25 PERCENTIL");
            t = stat_q1_t;
            h = stat_q1_h;
            break;

        case 5:
            LCD_Line("MEDIANA");
            t = stat_med_t;
            h = stat_med_h;
            break;

        case 6:
            LCD_Line("75 PERCENTIL");
            t = stat_q3_t;
            h = stat_q3_h;
            break;

        default:
            LCD_Line("MAX");
            t = stat_max_t;
            h = stat_max_h;
            break;
    }

    LCD_Cursor(2,0);
    LCD_Print("T:");
    LCD_Num2(t);
    LCD_Print(" H:");
    LCD_Num2(h);
}


// ============================================================
// PRONOSTICO
// ============================================================

void ShowForecast(
    uint8_t page
)
{
    LCD_Clear();


    if(
        !forecast_ready
    )
    {
        LCD_Line(
            "PRONOSTICO"
        );

        LCD_Cursor(
            2,
            0
        );

        LCD_Line(
            "MIN 2 REGISTROS"
        );

        return;
    }


    LCD_Cursor(
        1,
        0
    );


    LCD_Print("T+");

    LCD_Char(
        (uint8_t)(
            '1' + page
        )
    );

    LCD_Print(":");

    LCD_Num2(
        (uint8_t)forecast_t[page]
    );

    LCD_Print("C");


    LCD_Cursor(
        2,
        0
    );


    LCD_Print("H+");

    LCD_Char(
        (uint8_t)(
            '1' + page
        )
    );

    LCD_Print(":");

    LCD_Num2(
        (uint8_t)forecast_h[page]
    );

    LCD_Print("%");
}


// ============================================================
// GUARDAR MEDICION
// ============================================================

uint8_t SaveMeasurement(void)
{
    uint32_t block;


    record.number =
        record_count;

    record.year =
        rtc.year;

    record.month =
        rtc.month;

    record.day =
        rtc.day;

    record.hour =
        rtc.hour;

    record.minute =
        rtc.min;

    record.second =
        rtc.sec;

    record.temp =
        temperature;

    record.hum =
        humidity;


    block =
        DATA_START +
        record_count;


    if(
        !SD_WriteRecord(
            block,
            &record
        )
    )
    {
        return 0;
    }


    record_count++;


    if(
        !SD_WriteMeta(
            record_count
        )
    )
    {
        return 0;
    }


    return 1;
}


// ============================================================
// MAIN
// ============================================================

void main(void)
{
    uint8_t button;

    uint8_t screen;

    uint8_t menuOption;

    uint8_t dataPage;

    uint8_t statsPage;

    uint8_t forecastPage;

    uint16_t rtcTick = 0;

    uint16_t dhtTick = 0;

    uint16_t refreshTick = 0;

    uint32_t lastLog;

    uint32_t now;

    uint32_t elapsed;


    // ========================================================
    // OSCILADOR
    // ========================================================

    OSCCON = 0x72;


    // ========================================================
    // TODO DIGITAL
    // ========================================================

    ANSEL = 0x00;

    ANSELH = 0x00;


    // ========================================================
    // LCD
    // ========================================================

    TRISC2 = 0;

    TRISD3 = 0;
    TRISD4 = 0;
    TRISD5 = 0;
    TRISD6 = 0;
    TRISD7 = 0;


    // ========================================================
    // DHT11
    // ========================================================

    TRISB0 = 1;


    // ========================================================
    // I2C
    // ========================================================

    TRISC3 = 1;
    TRISC4 = 1;


    // ========================================================
    // BOTONES
    // ========================================================

    TRISB1 = 1;
    TRISB2 = 1;
    TRISB3 = 1;
    TRISB4 = 1;


    OPTION_REGbits.nRBPU = 0;

    WPUB1 = 1;
    WPUB2 = 1;
    WPUB3 = 1;
    WPUB4 = 1;


    // ========================================================
    // INICIALIZAR
    // ========================================================

    Timer1_Init();

    I2C_Init();

    SD_SPI_Init();

    LCD_Init();


    // ========================================================
    // INICIO
    // ========================================================

    LCD_Clear();

    LCD_Line(
        "TERMOMETRO"
    );

    LCD_Cursor(
        2,
        0
    );

    LCD_Line(
        "INICIANDO..."
    );

    __delay_ms(1200);


    // ========================================================
    // SD
    // ========================================================

    sd_ok =
        SD_Init();


    if(
        !sd_ok
    )
    {
        LCD_Clear();

        LCD_Line(
            "ERROR SD"
        );

        LCD_Cursor(
            2,
            0
        );

        LCD_Line(
            "INIT"
        );

        while(1)
        {
            ;
        }
    }


    // ========================================================
    // BLOQUE
    // ========================================================

    if(
        !SD_SetBlock()
    )
    {
        LCD_Clear();

        LCD_Line(
            "ERROR SD"
        );

        LCD_Cursor(
            2,
            0
        );

        LCD_Line(
            "BLOCK"
        );

        while(1)
        {
            ;
        }
    }


    // ========================================================
    // METADATA
    // ========================================================

    if(
        !SD_ReadMeta(
            &record_count
        )
    )
    {
        record_count = 0;

        (void)SD_WriteMeta(
            0
        );
    }


    // ========================================================
    // RTC
    // ========================================================

#if RTC_SET_START == 1

    rtc_ok =
        RTC_Set(
            12,
            0,
            0,
            6,
            3,
            10,
            26
        );

#else

    rtc_ok =
        RTC_Read(
            &rtc
        );

#endif


    if(
        !rtc_ok
    )
    {
        LCD_Clear();

        LCD_Line(
            "ERROR RTC"
        );

        LCD_Cursor(
            2,
            0
        );

        LCD_Line(
            "I2C"
        );

        while(1)
        {
            ;
        }
    }


    // ========================================================
    // DHT11
    // ========================================================

    dht_ok =
        DHT_Read(
            &humidity,
            &temperature
        );


    if(
        !dht_ok
    )
    {
        humidity = 0;

        temperature = 0;
    }


    // ========================================================
    // RTC
    // ========================================================

    rtc_ok =
        RTC_Read(
            &rtc
        );


    if(
        !rtc_ok
    )
    {
        while(1)
        {
            ;
        }
    }


    // ========================================================
    // TIEMPO INICIAL
    // ========================================================

    lastLog =
        RTC_Seconds(
            &rtc
        );


    // ========================================================
    // MENU
    // ========================================================

    screen =
        MENU_SCREEN;

    menuOption = 0;

    dataPage = 0;

    statsPage = 0;

    forecastPage = 0;


    ShowMenu(
        menuOption
    );


    // ========================================================
    // LOOP PRINCIPAL
    // ========================================================

    while(1)
    {
        // ====================================================
        // ACTUALIZAR RTC
        // ====================================================

        rtcTick += 100;


        if(
            rtcTick >= 500
        )
        {
            rtcTick = 0;


            rtc_ok =
                RTC_Read(
                    &rtc
                );


            if(
                rtc_ok
            )
            {
                now =
                    RTC_Seconds(
                        &rtc
                    );


                if(
                    now >= lastLog
                )
                {
                    elapsed =
                        now -
                        lastLog;
                }
                else
                {
                    elapsed =
                        86400UL -
                        lastLog +
                        now;
                }


                // ============================================
                // UNA MEDICION CADA HORA
                // ============================================

                if(
                    elapsed >=
                    LOG_INTERVAL
                )
                {
                    dht_ok =
                        DHT_Read(
                            &humidity,
                            &temperature
                        );


                    if(
                        dht_ok
                    )
                    {
                        if(
                            SaveMeasurement()
                        )
                        {
                            LCD_Clear();

                            LCD_Line(
                                "GUARDADO OK"
                            );

                            LCD_Cursor(
                                2,
                                0
                            );

                            LCD_Print("N:");

                            LCD_Num3(
                                record_count
                            );

                            __delay_ms(700);


                            // Volver a pantalla
                            // actual

                            if(
                                screen ==
                                MENU_SCREEN
                            )
                            {
                                ShowMenu(
                                    menuOption
                                );
                            }
                            else if(
                                screen ==
                                DATA_SCREEN
                            )
                            {
                                ShowData();
                            }
                        }
                    }


                    lastLog =
                        now;
                }
            }
        }


        // ====================================================
        // DHT11 CADA 2 SEGUNDOS
        // ====================================================

        dhtTick += 100;


        if(
            dhtTick >= 2000
        )
        {
            dhtTick = 0;


            dht_ok =
                DHT_Read(
                    &humidity,
                    &temperature
                );
        }


        // ====================================================
        // BOTON
        // ====================================================

        button =
            Button();


        // ====================================================
        // MENU
        // ====================================================

        if(
            screen ==
            MENU_SCREEN
        )
        {
            if(
                button == 1
            )
            {
                if(
                    menuOption == 0
                )
                {
                    menuOption = 3;
                }
                else
                {
                    menuOption--;
                }


                ShowMenu(
                    menuOption
                );
            }


            else if(
                button == 2
            )
            {
                menuOption++;


                if(
                    menuOption > 3
                )
                {
                    menuOption = 0;
                }


                ShowMenu(
                    menuOption
                );
            }


            else if(
                button == 3
            )
            {
                if(
                    menuOption == 0
                )
                {
                    screen =
                        DATA_SCREEN;

                    dataPage = 0;

                    ShowData();
                }


                else if(
                    menuOption == 1
                )
                {
                    if(
                        BuildStats()
                    )
                    {
                        statsPage = 0;

                        screen =
                            STATS_SCREEN;

                        ShowStats(
                            statsPage
                        );
                    }
                    else
                    {
                        LCD_Clear();

                        LCD_Line(
                            "SIN DATOS"
                        );

                        __delay_ms(700);

                        ShowMenu(
                            menuOption
                        );
                    }
                }


                else if(
                    menuOption == 2
                )
                {
                    (void)BuildForecast();

                    forecastPage = 0;

                    screen =
                        FORECAST_SCREEN;

                    ShowForecast(
                        forecastPage
                    );
                }
                else
                {
                    ConfigureRTC();

                    lastLog =
                        RTC_Seconds(
                            &rtc
                        );

                    ShowMenu(
                        menuOption
                    );
                }
            }
        }


        // ====================================================
        // DATOS
        // ====================================================

        else if(
            screen ==
            DATA_SCREEN
        )
        {
            if(
                button == 1 ||
                button == 2
            )
            {
                dataPage =
                    !dataPage;


                if(
                    dataPage == 0
                )
                {
                    ShowData();
                }
                else
                {
                    ShowDate();
                }
            }


            else if(
                button == 3 ||
                button == 4
            )
            {
                screen =
                    MENU_SCREEN;

                ShowMenu(
                    menuOption
                );
            }
        }


        // ====================================================
        // ESTADISTICAS
        // ====================================================

        else if(
            screen ==
            STATS_SCREEN
        )
        {
            if(
                button == 1
            )
            {
                if(
                    statsPage == 0
                )
                {
                    statsPage = 7;
                }
                else
                {
                    statsPage--;
                }


                ShowStats(
                    statsPage
                );
            }


            else if(
                button == 2
            )
            {
                statsPage++;


                if(
                    statsPage > 7
                )
                {
                    statsPage = 0;
                }


                ShowStats(
                    statsPage
                );
            }


            else if(
                button == 3 ||
                button == 4
            )
            {
                screen =
                    MENU_SCREEN;

                ShowMenu(
                    menuOption
                );
            }
        }


        // ====================================================
        // PRONOSTICO
        // ====================================================

        else if(
            screen ==
            FORECAST_SCREEN
        )
        {
            if(
                button == 1
            )
            {
                if(
                    forecastPage == 0
                )
                {
                    forecastPage = 5;
                }
                else
                {
                    forecastPage--;
                }


                ShowForecast(
                    forecastPage
                );
            }


            else if(
                button == 2
            )
            {
                forecastPage++;


                if(
                    forecastPage > 5
                )
                {
                    forecastPage = 0;
                }


                ShowForecast(
                    forecastPage
                );
            }


            else if(
                button == 3 ||
                button == 4
            )
            {
                screen =
                    MENU_SCREEN;

                ShowMenu(
                    menuOption
                );
            }
        }


        // ====================================================
        // REFRESCAR DATOS
        // ====================================================

        refreshTick += 100;


        if(
            refreshTick >= 1000
        )
        {
            refreshTick = 0;


            if(
                screen ==
                DATA_SCREEN &&
                dataPage == 0
            )
            {
                ShowData();
            }
        }


        __delay_ms(100);
    }
}
