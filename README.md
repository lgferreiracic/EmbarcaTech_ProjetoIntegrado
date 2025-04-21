# EmbarcaTech_ProjetoIntegrado.
<p align="center">
  <img src="Group 658.png" alt="EmbarcaTech" width="300">
</p>

## Atividade: Projeto Integrado

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/-Raspberry_Pi-C51A4A?style=for-the-badge&logo=Raspberry-Pi)
![GitHub](https://img.shields.io/badge/github-%23121011.svg?style=for-the-badge&logo=github&logoColor=white)
![Windows 11](https://img.shields.io/badge/Windows%2011-%230079d5.svg?style=for-the-badge&logo=Windows%2011&logoColor=white)

## Descrição do Projeto

Este projeto tem como objetivo integrar e aplicar os conhecimentos adquiridos durante a Fase 1 da capacitação. Como o tema era livre, optei pelo desenvolvimento de um jogo interativo no estilo arcade. O jogador controla uma nave representada por um LED azul, que deve desviar de obstáculos que descem pela matriz de LEDs. O jogo se inspira no clássico Spacewar, reconhecido por sua relevância histórica na computação gráfica.

Além da lógica do jogo, o projeto explora o uso de múltiplos periféricos conectados ao microcontrolador RP2040, com destaque para controle por joystick, feedback sonoro com buzzer e visual com LED RGB, além da exibição de telas no display OLED.

## Componentes Utilizados

- **Joystick (ADC nos eixos X e Y)**: Captura dos valores analógicos de movimentação.
- **Microcontrolador Raspberry Pi Pico W (RP2040)**: Responsável pelo controle dos pinos GPIO.
- **LED RGB**: Com os pinos conectados às GPIOs 11, 12 e 13.
- **Botão A**: Conectado à GPIO 5.
- **Botão B**: Conectado à GPIO 6
- **Botão do Joystick**: Conectado à GPIO 22.
- **Display SSD1306**: Conectado via I2C nas GPIOs 14 e 15.
- **Matriz de LEDs WS2812B**: Conectada à GPIO 7.
- **Buzzer 1 e Buzzer 2**: Emitindo alertas sonoros, conectados às GPIOs 10 e 21.
- **Potenciômetro do Joystick**: Conectado às entradas analógicas GPIO 26 (eixo X) e GPIO 27 (eixo Y).

## Ambiente de Desenvolvimento

- **VS Code**: Ambiente de desenvolvimento utilizado para escrever e debugar o código.
- **Linguagem C**: Linguagem de programação utilizada no desenvolvimento do projeto.
- **Pico SDK**: Kit de Desenvolvimento de Software utilizado para programar a placa Raspberry Pi Pico W.
- **Simulador Wokwi**: Ferramenta de simulação utilizada para testar o projeto.

## Guia de Instalação

1. Clone o repositório:
2. Importe o projeto utilizando a extensão da Raspberry Pi.
3. Compile o código utilizando a extensão da Raspberry Pi.
4. Caso queira executar na placa BitDogLab, insira o UF2 na placa em modo bootsel.
5. Para a simulação, basta executar pela extensão no ambiente integrado do VSCode.

## Guia de Uso

1. Ao iniciar, o display OLED mostra a tela de boas-vindas.
2. Pressione o botão do joystick para começar o jogo.
3. Mova a nave para a esquerda ou direita com o joystick.
4. Desvie dos obstáculos vermelhos para acumular pontos.
5. Ao atingir certos pontos, o jogo aumenta de velocidade e ativa feedbacks visuais e sonoros.
6. Caso colida, a nave explode e o jogo é encerrado.
7. Pressione Botão A para reiniciar ou Botão B para encerrar a aplicação.

## Testes

Testes básicos foram implementados para garantir que cada componente está funcionando corretamente. 

## Desenvolvedor

[Lucas Gabriel Ferreira](https://github.com/usuario-lider)

## Vídeo da Solução

[Link do YouTube](https://www.youtube.com/watch?v=Yg7zrFLfNIc)


