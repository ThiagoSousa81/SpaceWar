# Space-War
### Projeto final do Embarcatech

## Específicação do Hardware
- Raspberry Pi Pico
- Neopixels (rgb1 a rgb26)
- Pushbuttons (btn1, btn2)
- Resistors (r1 a r5)
- SSD1306 OLED
- Analog Joystick

### Pinagem utilizada

GPIO05 - Botão A com Pull-Up
GPIO06 - Botão B com Pull-Up
GPIO07 - Acesso ao PIO0 do RP2040 para Matriz de LEDs WS2812
GPIO22 - Acesso ao ADC do RP2040 

## Específicação do Firmware (software)
O software é um jogo simples onde o jogador controla um personagem usando um joystick. O objetivo é evitar inimigos enquanto coleta pontos. O código é escrito em C e utiliza bibliotecas para controlar o hardware, como a comunicação com o OLED e a leitura do joystick. As funções principais incluem a inicialização dos componentes, o loop do jogo que atualiza a tela e a lógica para detectar colisões.


## Execução do Projeto

## Referências
