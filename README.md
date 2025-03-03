# Space-War
### Projeto final do Embarcatech

## Especificação do Hardware
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

<div align=center>

![Circuito](https://github.com/ThiagoSousa81/SpaceWar/blob/main/circuito.png)

</div>

## Especificação do Firmware

O software é um jogo simples onde o jogador controla um personagem usando um joystick. O objetivo é evitar inimigos enquanto coleta pontos. O código é escrito em C e utiliza bibliotecas para controlar o hardware, como a comunicação com o OLED e a leitura do joystick. As funções principais incluem a inicialização dos componentes, o loop do jogo que atualiza a tela e a lógica para detectar colisões.<br>


O firmware é projetado para um jogo simples onde o jogador controla um personagem usando um joystick, evitando inimigos enquanto coleta pontos.

## Blocos funcionais
- Controle do joystick
- Controle da matriz de LEDs
- Comunicação I2C para o display
- Lógica do jogo (controle do jogador e inimigo)

## Descrição das funcionalidades
- `PLAYER()`: Controla o personagem do jogador.
- `ENEMY()`: Controla o movimento do inimigo.
- `menu_interface()`: Exibe a interface do menu.
- `score_display()`: Exibe a pontuação do jogador.
- `nota()`: Toca notas musicais através do buzzer.

## Definição das variáveis
- `screen`: Tela atual do display (menu, sobre, placar).
- `menu`: Opção do menu selecionada.
- `e_position`: Posição do inimigo.
- `score`: Pontuação do jogador.

## Fluxograma
Um fluxograma será criado para representar o fluxo do software, incluindo inicialização, loop do jogo e manipulação de eventos.

## Inicialização
O software inicializa o ADC, I2C, matriz de LEDs e botões, configurando o ambiente do jogo.

## Configurações dos registros
Funções para configurar GPIO, PWM e registros I2C estão incluídas para gerenciar interações de hardware.

## Estrutura e formato dos dados
Estruturas de dados incluem:
- `pixel_t`: Representa um pixel com componentes RGB.
- `ssd1306_t`: Estrutura para o display SSD1306.

## Protocolo de comunicação
I2C é usado para comunicação com o display SSD1306.

## Formato do pacote de dados
O formato de pacotes de dados não é explicitamente definido no código, mas a comunicação com o display utiliza protocolos I2C.
