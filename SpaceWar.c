#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "ws2812.pio.h"     // Biblioteca PIO para controle de LEDs WS2812

#define SMOOTHING_FACTOR 0.8 // Fator de suavização (0.0 a 1.0)

#include "inc/ssd1306.h"
#include "inc/font.h"

// Configurando Joystick
#define EIXO_Y 26    // ADC0
#define EIXO_X 27    // ADC1
float smoothed_value = 0;

#define LED_PIN 7
#define LED_COUNT 25     // Número de LEDs na matriz

// Configurando I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15

// Configurando Display
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// Posição do Menu
uint8_t menu = 0;

int map_value(float value, float in_min, float in_max, int out_min, int out_max) {
    // Mapeia o valor de uma faixa para outra
    return (int)((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min);
}

// Protótipos das funções
void JOYSTICK();

// Estrutura para representar um pixel com componentes RGB
struct pixel_t
{
    uint8_t G, R, B; // Componentes de cor: Verde, Vermelho e Azul
};

typedef struct pixel_t pixel_t; // Alias para a estrutura pixel_t
typedef pixel_t npLED_t;        // Alias para facilitar o uso no contexto de LEDs

npLED_t leds[LED_COUNT]; // Array para armazenar o estado de cada LED
PIO np_pio;              // Variável para referenciar a instância PIO usada
uint sm;                 // Variável para armazenar o número da máquina de estado (State Machine)

// Função para inicializar o PIO para controle dos LEDs
void npInit(uint pin)
{
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carregar o programa PIO
    np_pio = pio0;                                         // Usar o primeiro bloco PIO

    sm = pio_claim_unused_sm(np_pio, false); // Tentar usar uma state machine do pio0    

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // Inicializar state machine para LEDs

    // Inicializar todos os LEDs como apagados
    for (uint i = 0; i < LED_COUNT; ++i) 
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Função para atualizar os LEDs no hardware
void npUpdate()
{
    for (uint i = 0; i < LED_COUNT; ++i) // Iterar sobre todos os LEDs
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G); // Enviar componente Verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R); // Enviar componente Vermelho        
        pio_sm_put_blocking(np_pio, sm, leds[i].B); // Enviar componente Azul
    }
}

// Função para definir a cor de um LED específico
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    leds[index].R = r; // Definir componente vermelho
    leds[index].G = g; // Definir componente verde
    leds[index].B = b; // Definir componente azul
}

// Função para limpar (apagar) todos os LEDs
void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i) // Iterar sobre todos os LEDs
        npSetLED(i, 0, 0, 0);            // Definir cor como preta (apagado)
}

void matrixSetPlayer(int position, const uint8_t r, const uint8_t g, const uint8_t b) 
{
    npClear();
    /*  Gabarito do Display
    24, 23, 22, 21, 20
    15, 16, 17, 18, 19
    14, 13, 12, 11, 10
    05, 06, 07, 08, 09
    04, 03, 02, 01, 00
    */
    switch(position)
    {
        case 1:
            npSetLED(3, r, g, b);
            npSetLED(4, r, g, b);
            npSetLED(5, r, g, b);            
        break;
        case 2:
            npSetLED(2, r, g, b);
            npSetLED(3, r, g, b);
            npSetLED(4, r, g, b);
            npSetLED(6, r, g, b);
        break;
        case 3:
            npSetLED(1, r, g, b);
            npSetLED(2, r, g, b);
            npSetLED(3, r, g, b);
            npSetLED(7, r, g, b);
        break;
        case 4:
            npSetLED(0, r, g, b);
            npSetLED(1, r, g, b);
            npSetLED(2, r, g, b);
            npSetLED(8, r, g, b);
        break;
        case 5:
            npSetLED(0, r, g, b);
            npSetLED(1, r, g, b);
            npSetLED(9, r, g, b);            
        break;
    }
    
    npUpdate();
}

void matrixSetEnemy(int position, const uint8_t r, const uint8_t g, const uint8_t b)
{
    npClear();
    /*  Gabarito do Display
    24, 23, 22, 21, 20
    15, 16, 17, 18, 19
    14, 13, 12, 11, 10
    05, 06, 07, 08, 09
    04, 03, 02, 01, 00
    */
    switch(position)
    {
        case 1:
            npSetLED(15, r, g, b);
            npSetLED(23, r, g, b);
            npSetLED(24, r, g, b);            
        break;
        case 2:
            npSetLED(16, r, g, b);
            npSetLED(22, r, g, b);
            npSetLED(23, r, g, b);
            npSetLED(24, r, g, b);
        break;
        case 3:
            npSetLED(17, r, g, b);
            npSetLED(21, r, g, b);
            npSetLED(22, r, g, b);
            npSetLED(23, r, g, b);
        break;
        case 4:
            npSetLED(18, r, g, b);
            npSetLED(20, r, g, b);
            npSetLED(21, r, g, b);
            npSetLED(22, r, g, b);
        break;
        case 5:
            npSetLED(19, r, g, b);
            npSetLED(20, r, g, b);
            npSetLED(21, r, g, b);            
        break;
    }
    
    npUpdate();
}


int main()
{
    stdio_init_all();    

    //  Iniciando ADC
    adc_init();
    adc_gpio_init(EIXO_X);    

    // I2C Initialisation. Using it at 2000Khz.
    i2c_init(I2C_PORT, 400*5000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    npInit(LED_PIN);  // Inicializar os LEDs
    matrixSetPlayer(3, 0, 80, 80);
    matrixSetEnemy(3, 80, 80, 0);

    // Iniciando e configurando o Display
    ssd1306_t ssd;
    ssd1306_init(&ssd, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, 0x3c, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
    ssd1306_fill(&ssd, false);    

    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);  // borda fixa
    ssd1306_draw_string(&ssd, "PLAY", 48, 24);
    ssd1306_draw_string(&ssd, "SOBRE", 44, 34);
    ssd1306_rect(&ssd, 24, 32, 7, 7, true, true);  
    ssd1306_rect(&ssd, 34, 32, 7, 7, true, false); 
    ssd1306_send_data(&ssd); // atualiza display

    while (true) {
        // Lê o eixo X (ADC1)
        matrixSetEnemy(3, 80, 80, 0);
        JOYSTICK();
        adc_select_input(1);
        uint16_t x_value = adc_read();
        printf("Valor no display %d\n", x_value); 
        
        adc_select_input(0);
        uint16_t y_value = adc_read();
        if (y_value > 3000){            
            menu = 0; // PLAY
            ssd1306_rect(&ssd, 24, 32, 7, 7, true, true);             
            ssd1306_draw_char(&ssd, ' ', 32, 34);
            ssd1306_rect(&ssd, 34, 32, 7, 7, true, false); 
        }
        else if (y_value < 1000){
            menu = 1; // ABOUT            
            ssd1306_draw_char(&ssd, ' ', 32, 24);
            ssd1306_rect(&ssd, 24, 32, 7, 7, true, false); 
            ssd1306_rect(&ssd, 34, 32, 7, 7, true, true); 
        }
        ssd1306_send_data(&ssd); // atualiza display

        sleep_ms(30);
    }    
}

// Função do Joystick
void JOYSTICK() {    
    adc_select_input(1);
    uint16_t raw_value = adc_read();    
        
    // Mapeia a tensão para a faixa de 1 a 5
    float mapped_value = map_value(raw_value, 29, 4081, 1, 5) + 1;
    
    // Suaviza o valor
    // Suaviza a transição entre o valor atual e o novo valor
    smoothed_value += ((mapped_value - smoothed_value) * SMOOTHING_FACTOR);
    
    // Imprime o valor suavizado
    printf("Valor suavizado: %d\n", (int)smoothed_value);

    npClear();
    matrixSetPlayer(smoothed_value, 0, 80, 80);
    npUpdate();    
}