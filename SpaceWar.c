/* 
|=======================================|
|                                       |
|            Thiago Sousa               |
|   https://github.com/ThiagoSousa81    |
|                                       |
|=======================================|
*/

#include <stdio.h>                     // Biblioteca padrão para entrada e saída
#include "pico/stdlib.h"              // Biblioteca padrão do Raspberry Pi Pico
#include "hardware/i2c.h"             // Biblioteca para comunicação I2C
#include "hardware/pio.h"             // Biblioteca para PIO (Programmable Input/Output)
#include "hardware/adc.h"             // Biblioteca para conversão analógica-digital
#include "hardware/timer.h"           // Biblioteca para gerenciamento de temporizadores
#include "ws2812.pio.h"               // Biblioteca PIO para controle de LEDs WS2812
#include "hardware/pwm.h"             // Biblioteca para interface PWM

#define SMOOTHING_FACTOR 0.8 // Fator de suavização para a leitura do joystick (0.0 a 1.0)

#include "inc/ssd1306.h"
#include "inc/font.h"

/* Configurações do Joystick */
#define EIXO_Y 26    // Pino ADC para o eixo Y do joystick
#define EIXO_X 27    // Pino ADC para o eixo X do joystick
float smoothed_value = 0; // Valor suavizado da leitura do joystick


/* Configurações da Matriz de LEDs */
#define LED_PIN 7              // Pino de controle dos LEDs
#define LED_COUNT 25           // Número total de LEDs na matriz


/* Configurações do I2C */
#define I2C_PORT i2c1          // Porta I2C utilizada
#define I2C_SDA 14             // Pino SDA para comunicação I2C
#define I2C_SCL 15             // Pino SCL para comunicação I2C


/* Configurações do Display */
#define DISPLAY_WIDTH 128      // Largura do display em pixels
#define DISPLAY_HEIGHT 64      // Altura do display em pixels
ssd1306_t ssd;                // Estrutura para o display SSD1306


// Frequências das notas musicais em Hertz

/*
Dó (C) - 132 Hz
Ré (D) - 148.5 Hz
Mi (E) - 165 Hz
Fá (F) - 175.956 Hz
Sol (G) - 198 Hz
Lá (A) - 220.044 Hz
Si (B) - 247.500 Hz
*/

#define DO 264 // Define a frequência da nota Dó
#define RE 297 // Define a frequência da nota Ré
#define MI 330 // Define a frequência da nota Mi
#define FA 351.912 // Define a frequência da nota Fá
#define SOL 396 // Define a frequência da nota Sol
#define LA 440.088 // Define a frequência da nota Lá
#define SI 495 // Define a frequência da nota Si


// Pinos do Buzzer
#define BUZZER_A 21 // Define o pino GPIO 21 como o pino conectado ao buzzer A
#define BUZZER_B 10 // Define o pino GPIO 10 como o pino conectado ao buzzer B


/* Configurações dos Botões */
const uint BUTTON_A = 5; // Pino GPIO do botão A
const uint BUTTON_B = 6; // Pino GPIO do botão B
static volatile uint32_t last_time = 0; // Armazena o tempo do último clique de botão (em microssegundos)


/* Estado do Jogo */
uint8_t screen = 0;                // Tela atual do display (0 = menu, 1 = sobre, 2 = placar)
uint8_t menu = 0;                  // Opção do menu selecionada
volatile uint8_t e_position = 3;   // Posição do inimigo
volatile bool vivo = true;         // Estado do jogador (vivo ou morto)
volatile bool play = false;        // Estado do jogo (em execução ou pausado)
volatile bool disparo_em_andamento = false; // Indica se um disparo está em andamento
volatile uint16_t score = 0;       // Pontuação do jogador


/* Função para mapear valores de uma faixa para outra */
int map_value(float value, float in_min, float in_max, int out_min, int out_max) {
    return (int)((value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min); // Retorna o valor mapeado
}


/* Protótipos das funções */
void nota(uint32_t frequencia, uint32_t tempo_ms); // Função para tocar notas
void PLAYER();                                     // Função para controlar o jogador
void ENEMY();                                      // Função para controlar o inimigo
void menu_interface();                             // Função para exibir a interface do menu
void score_display();                              // Função para exibir a pontuação
void menu_select();                                // Função para selecionar opções do menu
bool repeating_timer_callback();                   // Função de callback do temporizador
void gpio_irq_handler(uint gpio, uint32_t events); // Função de interrupção para os botões


/* Estrutura para representar um pixel com componentes RGB */
struct pixel_t {
    uint8_t G, R, B; // Componentes de cor: Verde, Vermelho e Azul
};


typedef struct pixel_t pixel_t;   // Alias para a estrutura pixel_t
typedef pixel_t npLED_t;          // Alias para facilitar o uso no contexto de LEDs


npLED_t leds[LED_COUNT]; // Array para armazenar o estado de cada LED na matriz
PIO np_pio;              // Variável para referenciar a instância PIO usada para controle de LEDs
uint sm;                 // Variável para armazenar o número da máquina de estado (State Machine)


/* Função para inicializar o PIO para controle dos LEDs */
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carregar o programa PIO
    np_pio = pio0;                                         // Usar o primeiro bloco PIO

    sm = pio_claim_unused_sm(np_pio, false); // Tentar usar uma state machine do pio0    

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // Inicializar state machine para LEDs

    // Inicializar todos os LEDs como apagados
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0; // Componente vermelho
        leds[i].G = 0; // Componente verde
        leds[i].B = 0; // Componente azul
    }
}

// Função para configurar PWM no pino especificado
void configure_pwm(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0f);  // Define a divisão do clock
    pwm_init(slice_num, &config, true);
}

/* Função para atualizar os LEDs no hardware */
void npUpdate() {
    for (uint i = 0; i < LED_COUNT; ++i) { // Iterar sobre todos os LEDs
        pio_sm_put_blocking(np_pio, sm, leds[i].G); // Enviar componente Verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R); // Enviar componente Vermelho        
        pio_sm_put_blocking(np_pio, sm, leds[i].B); // Enviar componente Azul
    }
}


/* Função para definir a cor de um LED específico */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
    leds[index].R = r; // Definir componente vermelho
    leds[index].G = g; // Definir componente verde
    leds[index].B = b; // Definir componente azul
}


/* Função para limpar (apagar) todos os LEDs */
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i) // Iterar sobre todos os LEDs
        npSetLED(i, 0, 0, 0);            // Definir cor como preta (apagado)
}


/* Função para definir a posição do jogador na matriz de LEDs */
void matrixSetPlayer(int position, const uint8_t r, const uint8_t g, const uint8_t b) {
    /* Gabarito do Display
    24, 23, 22, 21, 20
    15, 16, 17, 18, 19
    14, 13, 12, 11, 10
    05, 06, 07, 08, 09
    04, 03, 02, 01, 00
    */


    // Limpa a área onde o jogador será desenhado
    for(int i = 0; i < 10; i++) {
        npSetLED(i, 0, 0, 0); // Apaga os LEDs
    }


    // Define a posição do jogador na matriz de LEDs
    switch(position) {
        case 1:
            npSetLED(3, r, g, b); // Define LEDs para a posição 1
            npSetLED(4, r, g, b);
            npSetLED(5, r, g, b);            
            break;
        case 2:
            npSetLED(2, r, g, b); // Define LEDs para a posição 2
            npSetLED(3, r, g, b);
            npSetLED(4, r, g, b);
            npSetLED(6, r, g, b);
            break;
        case 3:
            npSetLED(1, r, g, b); // Define LEDs para a posição 3
            npSetLED(2, r, g, b);
            npSetLED(3, r, g, b);
            npSetLED(7, r, g, b);
            break;
        case 4:
            npSetLED(0, r, g, b); // Define LEDs para a posição 4
            npSetLED(1, r, g, b);
            npSetLED(2, r, g, b);
            npSetLED(8, r, g, b);
            break;
        case 5:
            npSetLED(0, r, g, b); // Define LEDs para a posição 5
            npSetLED(1, r, g, b);
            npSetLED(9, r, g, b);            
            break;
    }        
}


/* Função para definir a posição do inimigo na matriz de LEDs */
void matrixSetEnemy(int position, const uint8_t r, const uint8_t g, const uint8_t b) {    
    /* Gabarito do Display
    24, 23, 22, 21, 20
    15, 16, 17, 18, 19
    14, 13, 12, 11, 10
    05, 06, 07, 08, 09
    04, 03, 02, 01, 00
    */

    
    // Limpa a área onde o inimigo será desenhado
    for(int i = 10; i < 25; i++) {
        npSetLED(i, 0, 0, 0); // Apaga os LEDs
    }


    // Define a posição do inimigo na matriz de LEDs
    switch(position) {
        case 1:
            npSetLED(15, r, g, b); // Define LEDs para a posição 1
            npSetLED(23, r, g, b);
            npSetLED(24, r, g, b);            
            break;
        case 2:
            npSetLED(16, r, g, b); // Define LEDs para a posição 2
            npSetLED(22, r, g, b);
            npSetLED(23, r, g, b);
            npSetLED(24, r, g, b);
            break;
        case 3:
            npSetLED(17, r, g, b); // Define LEDs para a posição 3
            npSetLED(21, r, g, b);
            npSetLED(22, r, g, b);
            npSetLED(23, r, g, b);
            break;
        case 4:
            npSetLED(18, r, g, b); // Define LEDs para a posição 4
            npSetLED(20, r, g, b);
            npSetLED(21, r, g, b);
            npSetLED(22, r, g, b);
            break;
        case 5:
            npSetLED(19, r, g, b); // Define LEDs para a posição 5
            npSetLED(20, r, g, b);
            npSetLED(21, r, g, b);            
            break;
    }        
}



/* Função para desenhar o tiro do jogador na matriz de LEDs */
void shot_player(int position, const uint8_t r, const uint8_t g, const uint8_t b) {
    /* Gabarito do Display
    24, 23, 22, 21, 20
    15, 16, 17, 18, 19
    14, 13, 12, 11, 10
    05, 06, 07, 08, 09
    04, 03, 02, 01, 00
    */

   
    // Limpa a área onde o tiro será desenhado
    for(int i = 10; i < 15; i++) {
        npSetLED(i, 0, 0, 0); // Apaga os LEDs
    }


    switch(position)
    {
        case 1:
            npSetLED(14, r, g, b);
            npSetLED(15, r, g, b);
            npSetLED(24, r, g, b);            
        break;
        case 2:
            npSetLED(23, r, g, b);
            npSetLED(16, r, g, b);                        
            npSetLED(13, r, g, b);            
        break;
        case 3:
            npSetLED(22, r, g, b);
            npSetLED(17, r, g, b);
            npSetLED(12, r, g, b);            
        break;
        case 4:
            npSetLED(21, r, g, b);
            npSetLED(18, r, g, b);
            npSetLED(11, r, g, b);            
        break;
        case 5:
            npSetLED(20, r, g, b);
            npSetLED(19, r, g, b);
            npSetLED(10, r, g, b);            
        break;
    }      

}

/* Função principal do programa */
int main() {
    stdio_init_all(); // Inicializa a biblioteca padrão

    // Iniciando ADC
    adc_init();
    adc_gpio_init(EIXO_X); // Inicializa o pino do eixo X
    adc_gpio_init(EIXO_Y); // Inicializa o pino do eixo Y

    // Iniciando o I2C na frequência de 400kHz
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Configura o pino SDA para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Configura o pino SCL para I2C
    gpio_pull_up(I2C_SDA); // Habilita o pull-up interno no pino SDA
    gpio_pull_up(I2C_SCL); // Habilita o pull-up interno no pino SCL

    npInit(LED_PIN);  // Inicializa os LEDs
    matrixSetPlayer(3, 0, 80, 80); // Define a posição inicial do jogador
    //matrixSetEnemy(3, 80, 80, 0); // (Comentado) Define a posição inicial do inimigo



    /* Inicializando os botões */
    gpio_init(BUTTON_A); // Inicializa o botão A
    gpio_set_dir(BUTTON_A, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_A); // Habilita o pull-up interno
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Configuração da interrupção com callback do botão A    

    gpio_init(BUTTON_B); // Inicializa o botão B
    gpio_set_dir(BUTTON_B, GPIO_IN); // Configura o pino como entrada
    gpio_pull_up(BUTTON_B); // Habilita o pull-up interno
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Configuração da interrupção com callback do botão B    



    /* Iniciando e configurando o Display */    
    ssd1306_init(&ssd, DISPLAY_WIDTH, DISPLAY_HEIGHT, false, 0x3c, I2C_PORT); // Inicializa o display SSD1306
    ssd1306_config(&ssd); // Configura o display
    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
    menu_interface(); // Exibe a interface do menu

    /* Estrutura para o temporizador */
    struct repeating_timer timer;
    
    // Configura o temporizador para chamar a função de callback a cada segundo
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer);

    nota(DO, 250); // Toca a nota Sol por 250 ms
    sleep_ms(250);
    nota(FA, 250); // Toca a nota Lá por 250 ms
    sleep_ms(250);
    nota(SI, 250); // Toca a nota Si por 250 ms
    sleep_ms(250);

    while (true) { // Loop principal do jogo
        PLAYER(); // Atualiza a lógica do jogador

        if (screen == 0) {
            menu_select(); // Seleciona a opção do menu
        }

        npUpdate(); // Atualiza a matriz de LEDs
        sleep_ms(20); // Aguarda um curto período
    }    
}


/* Função para exibir a interface do menu */
void menu_interface() {
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);  // Desenha a borda fixa
    ssd1306_draw_string(&ssd, "PLAY", 48, 24); // Desenha a opção "PLAY"
    ssd1306_draw_string(&ssd, "SOBRE", 44, 34); // Desenha a opção "SOBRE"
    ssd1306_rect(&ssd, 24, 32, 7, 7, true, true);  // Indica a opção selecionada
    ssd1306_rect(&ssd, 34, 32, 7, 7, true, false); // Indica a opção não selecionada
    ssd1306_send_data(&ssd); // Atualiza o display
}


/* Função para selecionar opções do menu */
void menu_select() {
    adc_select_input(1); // Lê o eixo X (ADC1)   
    uint16_t x_value = adc_read(); // Lê o valor do eixo X
    
    adc_select_input(0); // Lê o eixo Y (ADC0)
    uint16_t y_value = adc_read(); // Lê o valor do eixo Y
    if (y_value > 3000) {            
        menu = 0; // Seleciona a opção "PLAY"
        ssd1306_rect(&ssd, 24, 32, 7, 7, true, true); // Indica a opção "PLAY" como selecionada             
        ssd1306_draw_char(&ssd, ' ', 32, 34); // Limpa a opção "SOBRE"
        ssd1306_rect(&ssd, 34, 32, 7, 7, true, false); // Indica a opção "SOBRE" como não selecionada
    } else if (y_value < 1000) {
        menu = 1; // Seleciona a opção "SOBRE"            
        ssd1306_draw_char(&ssd, ' ', 32, 24); // Limpa a opção "PLAY"
        ssd1306_rect(&ssd, 24, 32, 7, 7, true, false); // Indica a opção "PLAY" como não selecionada
        ssd1306_rect(&ssd, 34, 32, 7, 7, true, true); // Indica a opção "SOBRE" como selecionada
    }
    ssd1306_send_data(&ssd); // Atualiza o display
}


/* Função para exibir a pontuação */
void score_display() {
    char buffer[4]; // Buffer para armazenar a pontuação como string

    sprintf(buffer, "%u", score); // Converte uint16_t para string

    ssd1306_fill(&ssd, false); // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // Desenha a borda fixa
    ssd1306_draw_string(&ssd, "SCORE", 28, 28); // Desenha o texto "SCORE"
    ssd1306_draw_string(&ssd, buffer, 76, 28); // Desenha a pontuação
    ssd1306_send_data(&ssd); // Atualiza o display
}

// Função para tocar uma frequência por uma duração específica
void nota(uint32_t frequencia, uint32_t tempo_ms) {
    configure_pwm(BUZZER_A); // Configura o PWM para o buzzer A
    configure_pwm(BUZZER_B); // Configura o PWM para o buzzer B

    uint32_t delay = 1000000 / (frequencia * 2);  // Calcula o tempo de atraso em microssegundos
    uint32_t ciclo = frequencia * tempo_ms / 1000;  // Calcula o número de ciclos necessários

    // Configura o PWM para a frequência desejada
    pwm_set_gpio_level(BUZZER_A, 32768); // Define o nível de PWM (50% de duty cycle)
    pwm_set_gpio_level(BUZZER_B, 32768); // Define o nível de PWM (50% de duty cycle)

    // Loop para gerar a onda quadrada no buzzer
    for (uint32_t i = 0; i < ciclo; i++) {
        sleep_us(delay); // Espera pelo tempo de atraso calculado
    }

    pwm_set_gpio_level(BUZZER_A, 0); // Desliga o PWM

    pwm_set_gpio_level(BUZZER_B, 0); // Desliga o PWM
}

// Função do Joystick
void PLAYER() 
{    
    adc_select_input(1);
    uint16_t raw_value = adc_read();    
        
    // Mapeia a tensão para a faixa de 1 a 5
    float mapped_value = map_value(raw_value, 31, 4081, 1, 5) + 1;
    
    // Suaviza a transição entre o valor atual e o novo valor
    smoothed_value += ((mapped_value - smoothed_value) * SMOOTHING_FACTOR);
    
    // Imprime o valor suavizado
    //printf("Valor suavizado: %d\n", (int)smoothed_value);
    
    matrixSetPlayer(smoothed_value, 0, 80, 80);     
}

void ENEMY()
{
    // Gera um número aleatório entre 0 e 1
    int direction = rand() % 2; // 0 para decrementar, 1 para incrementar

    // Atualiza a posição com base na direção
    if (direction == 0) {
        // Tenta decrementar a posição
        if (e_position > 1) {
            e_position--;
        }
    } else {
        // Tenta incrementar a posição
        if (e_position < 5) {
            e_position++;
        }
    }
    matrixSetEnemy(e_position, 80, 80, 0);
    // Exibe a nova posição
    //printf("Nova posição do inimigo: %d\n", e_position);
}

bool repeating_timer_callback(struct repeating_timer *t)
{
    if (vivo && play)
    {
        ENEMY();
    }    
    // Retorna true para manter o temporizador repetindo. Se retornar false, o temporizador para.
    // A propriedade vivo indica se o player não foi atingido
    return true;
}

// Função de interrupção com debouncing do botão A
void gpio_irq_handler(uint gpio, uint32_t events)
{
    // Obtém o tempo atual em microssegundos
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    // Verifica se passou tempo suficiente desde o último evento
    if (current_time - last_time > 500000) // 500 ms de debouncing
    {
        if (gpio == BUTTON_A)
        {            
            if (menu == 0 && play == false)
            {
                play = true;
                screen = 2;
                score_display();
            }
            else if (menu == 1 && !play)
            {                
                screen = 1;
                ssd1306_fill(&ssd, false);
                ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);  // borda fixa                                                

                ssd1306_draw_string(&ssd, "SPACE WAR", 24, 20);                

                ssd1306_draw_string(&ssd, "THIAGOSOUSA81", 12, 30);            

                ssd1306_draw_string(&ssd, "EMBARCATECH", 18, 40);
                ssd1306_send_data(&ssd);                        
            }
            else if (screen == 1 && menu == 1) 
            {
                menu_interface();
                screen = 0;                
            }
            last_time = current_time; // Atualiza o tempo do último evento
        }
        else if (gpio == BUTTON_B)
        {
            if (play == true)
            {
                //nota(LA, 250); // Toca a nota Lá por 250 ms 

                shot_player(smoothed_value, 80, 0, 80);

                // Se atingir o enemy 
                if (smoothed_value == e_position || smoothed_value == e_position + 1 || smoothed_value == e_position - 1)
                {
                    matrixSetEnemy(e_position, 80, 80, 80);
                    score++;
                    score_display();

                } 
            }            
            last_time = current_time; // Atualiza o tempo do último evento
        }
    }
}
