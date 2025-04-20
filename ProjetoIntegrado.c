#include <stdio.h>    
#include <math.h>          
#include "pico/stdlib.h"     
#include "pico/bootrom.h" 
#include "hardware/adc.h"   
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "inc/ssd1306.h"  
#include "ws2812.pio.h"

// Definição dos pinos correspondentes aos botões, buzzers, LEDs e Joystick
#define BUTTON_A_PIN 5
#define BUTTON_B_PIN 6
#define WS2812_PIN 7 
#define BUZZER_A_PIN 10 
#define BUZZER_B_PIN 21 
#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_BUTTON_PIN 22
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 12
#define LED_RED_PIN 13

#define NUM_PIXELS 25 // Define o número de LEDs RGB
#define VOLUME 0.005 // Define o volume padrão para os buzzers

// Definição da estrutura RGB para representar as cores
typedef struct {
    double R; // Intensidade da cor vermelha
    double G; // Intensidade da cor verde
    double B; // Intensidade da cor azul
} RGB;

// Definição dos parâmetros do display OLED
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_ADDR 0x3C

// Definição dos parâmetros do I2C
#define I2C_PORT i2c1
#define SDA_PIN 14
#define SCL_PIN 15

// Definição do erro de margem para o joystick
#define MARGIN_OF_ERROR 200

volatile uint32_t button_a_time = 0; // Variável para debounce do botão A
volatile uint32_t button_b_time = 0; // Variável para debounce do botão B
volatile uint32_t joystick_button_time = 0; // Variável para debounce do botão do joystick
PIO pio; //Variável para armazenar a configuração da PIO
uint sm; //Variável para armazenar o estado da máquina
ssd1306_t ssd; // Declaração da estrutura do display OLED

// Função para debounce dos botões
bool debounce(volatile uint32_t *last_time){
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *last_time > 250){ 
        *last_time = current_time;
        return true;
    }
    return false;
}

// Função para inicialização dos LEDs
void init_leds(){
    // Inicialização do LED verde
    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_put(LED_GREEN_PIN, false);

    // Inicialização do LED azul
    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_put(LED_BLUE_PIN, false);

    // Inicialização do LED vermelho
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_put(LED_RED_PIN, false);          
}

// Inicializa todos os buzzers
void buzzer_init_all(){
    // Inicializa o buzzer A
    gpio_init(BUZZER_A_PIN);
    gpio_set_dir(BUZZER_A_PIN, GPIO_OUT);
    gpio_put(BUZZER_A_PIN, false); // Desliga o buzzer A inicialmente

    // Inicializa o buzzer B
    gpio_init(BUZZER_B_PIN);
    gpio_set_dir(BUZZER_B_PIN, GPIO_OUT);
    gpio_put(BUZZER_B_PIN, false); // Desliga o buzzer B inicialmente
}

// Configura o PWM no pino do buzzer com uma frequência especificada
void set_buzzer_frequency(uint pin, uint frequency) {
    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);

    // Configurar o pino como saída de PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);

    // Configurar o PWM com frequência desejada
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (frequency * 4096)); // Calcula divisor do clock

    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Inicializa com duty cycle 0 (sem som)
}

// Função para tocar o buzzer com volume ajustável
void play_buzzer(uint pin, uint frequency, float volume) {
    // Configura o PWM com a frequência desejada
    set_buzzer_frequency(pin, frequency);

    // Calcula o nível de duty cycle baseado no volume (0.0 a 1.0)
    uint16_t duty_cycle = (uint16_t)(volume * 65535); // Volume ajusta o duty cycle (0–65535)

    // Define o duty cycle para controlar o volume
    pwm_set_gpio_level(pin, duty_cycle);
}

// Função para inicialização do display OLED
void init_display(ssd1306_t *ssd){
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    ssd1306_init(ssd, SSD1306_WIDTH, SSD1306_HEIGHT, false, SSD1306_ADDR, I2C_PORT);
    ssd1306_config(ssd);
    ssd1306_fill(ssd, false);
    ssd1306_send_data(ssd);
}

// Função para limpar o display
void clear_display(ssd1306_t *ssd){
    ssd1306_fill(ssd, false); // Limpa o display
    ssd1306_send_data(ssd); // Atualiza o display
  }

//rotina para inicialização da matrix de leds - ws2812b
uint matrix_init() {
    //Configurações da PIO
    pio = pio0; 
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, WS2812_PIN);
 }
 
 //rotina para definição da intensidade de cores do led
 uint32_t matrix_rgb(double r, double g, double b){
   unsigned char R, G, B;
   R = r * 255;
   G = g * 255;
   B = b * 255;
   return (G << 24) | (R << 16) | (B << 8);
 }
 
 //rotina para acionar a matrix de leds - ws2812b
 void set_leds(PIO pio, uint sm, double r, double g, double b) {
     uint32_t valor_led;
     for (int16_t i = 0; i < NUM_PIXELS; i++) {
         valor_led = matrix_rgb(r, g, b);
         pio_sm_put_blocking(pio, sm, valor_led);
     }
 }
 
 // Função para converter a posição do matriz para uma posição do vetor.
int getIndex(int x, int y) {
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0) {
        return 24-(y * 5 + x); // Linha par (esquerda para direita).
    } else {
        return 24-(y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}

//rotina para acionar a matrix de leds - ws2812b
void desenho_pio(RGB pixels[NUM_PIXELS], PIO pio, uint sm) {
    for (int i = 0; i < NUM_PIXELS; i++) {
        int x = i % 5;
        int y = i / 5;
        int index = getIndex(x, y);
        pio_sm_put_blocking(pio, sm, matrix_rgb(pixels[index].R, pixels[index].G, pixels[index].B));
    }
}

// Função para apagar a matriz de leds.
void clear_matrix(){
    RGB BLACK = {0, 0, 0}; // Cor preta (apagado)
    RGB pixels[NUM_PIXELS];
    for (int i = 0; i < NUM_PIXELS; i++) {
        pixels[i] = BLACK;
    }
    desenho_pio(pixels, pio0, 0);
}

// Função de callback para os botões
void gpio_irq_handler(uint gpio, uint32_t events){
    if (gpio == BUTTON_A_PIN){
        if (debounce(&button_a_time)){
        }
    }
    else if (gpio == JOYSTICK_BUTTON_PIN){
        bool current_state = gpio_get(JOYSTICK_BUTTON_PIN);

        if (!current_state && debounce(&joystick_button_time)){ 
        }
    }
    else if (gpio == BUTTON_B_PIN){
        if (debounce(&button_b_time)){
            clear_matrix();
            clear_display(&ssd);
            reset_usb_boot(0,0);
        }
    }
}

// Função para inicialização dos botões
void init_buttons(){
    // Inicialização do ADC
    adc_init();

    // Inicialização do botão A
    adc_gpio_init(JOYSTICK_X_PIN);
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicialização do botão B
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicialização do botão do joystick
    adc_gpio_init(JOYSTICK_Y_PIN);
    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

// Função para movimentação do quadrado no display OLED
void joystic_movimentation(ssd1306_t *ssd, uint16_t x_value, uint16_t y_value){
    static uint16_t square_x;
    static uint16_t square_y;

    square_x = (x_value * SSD1306_WIDTH) / 4095; // Calcula a posição do quadrado no eixo x
    square_y = SSD1306_HEIGHT - ((y_value * SSD1306_HEIGHT) / 4095); // Calcula a posição do quadrado no eixo y

    square_x = (square_x < 0) ? 0 : (square_x > SSD1306_WIDTH - 8) ? SSD1306_WIDTH - 8 : square_x; // Limita a posição do quadrado no eixo x
    square_y = (square_y < 0) ? 0 : (square_y > SSD1306_HEIGHT - 8) ? SSD1306_HEIGHT - 8 : square_y; // Limita a posição do quadrado no eixo y

    ssd1306_fill(ssd, false); // Limpa o display
    ssd1306_rect(ssd, square_y, square_x, 8, 8, true, true); // Desenha o quadrado

    ssd1306_send_data(ssd);
}

// Função principal
int main(){
    stdio_init_all(); // Inicialização da comunicação serial
    init_display(&ssd); // Inicialização do display OLED
    init_leds(); // Inicialização dos LEDs
    init_buttons(); // Inicialização dos botões e do joystick
    buzzer_init_all(); // Inicialização dos buzzers
    matrix_init(); // Inicialização da matriz de LEDs

    while (true){
        adc_select_input(1); // Seleciona o pino do ADC para leitura do eixo x do joystick
        uint16_t x_value = adc_read(); // Lê o valor do eixo x do joystick
        adc_select_input(0); // Seleciona o pino do ADC para leitura do eixo y do joystick
        uint16_t y_value = adc_read(); // Lê o valor do eixo y do joystick
        
        joystic_movimentation(&ssd, x_value, y_value); // Movimenta o quadrado no display OLED
        sleep_ms(100); // Delay para evitar a leitura do ADC em alta frequência
    }
    return 0;
}