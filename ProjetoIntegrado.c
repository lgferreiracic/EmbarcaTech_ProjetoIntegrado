#include <stdio.h>    
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

// Definição dos componentes do jogo
#define OBSTACLE 2
#define SHIP 1
#define EMPTY 0

// Definição da estrutura RGB para representar as cores
typedef struct {
    double R; // Intensidade da cor vermelha
    double G; // Intensidade da cor verde
    double B; // Intensidade da cor azul
} RGB;

volatile bool collision = false; // Variável para verificar colisão
volatile uint32_t button_a_time = 0; // Variável para debounce do botão A
volatile uint32_t button_b_time = 0; // Variável para debounce do botão B
volatile uint32_t joystick_button_time = 0; // Variável para debounce do botão do joystick
PIO pio; //Variável para armazenar a configuração da PIO
uint sm; //Variável para armazenar o estado da máquina
ssd1306_t ssd; // Declaração da estrutura do display OLED
uint score = 0; // Variável para armazenar a pontuação
uint delay = 300;
int ship_pos = 2; // Posição inicial da nave (coluna)
uint8_t space[5][5] = {0};

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

// Função para inicialização dos botões
void init_buttons(){
    // Inicialização do ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    // Inicialização do botão A
    gpio_init(BUTTON_A_PIN);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_A_PIN);
    
    // Inicialização do botão B
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);
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

// Função para tocar o buzzer por um tempo especificado (em milissegundos)
void play_buzzer(uint pin, uint frequency, uint duration_ms) {

    set_buzzer_frequency(pin, frequency);   
    pwm_set_gpio_level(pin, 32768);           
    sleep_ms(duration_ms);                   
    pwm_set_gpio_level(pin, 0);              
}

void play_denied_sound(){
    gpio_put(LED_RED_PIN, true); // Acende LED vermelho
    play_buzzer(BUZZER_A_PIN, 3300, 100);
    gpio_put(LED_RED_PIN, false); // Apaga LED vermelho
    sleep_ms(50);
    gpio_put(LED_RED_PIN, true); // Acende LED vermelho
    play_buzzer(BUZZER_A_PIN, 3300, 100);
    gpio_put(LED_RED_PIN, false); // Apaga LED vermelho
}

void play_success_sound(){
    gpio_put(LED_GREEN_PIN, true); // Acende LED verde
    play_buzzer(BUZZER_A_PIN, 4400, 100);
    gpio_put(LED_GREEN_PIN, false); // Apaga LED verde
    sleep_ms(50);
    gpio_put(LED_GREEN_PIN, true); // Acende LED verde
    play_buzzer(BUZZER_A_PIN, 4400, 100);
    gpio_put(LED_GREEN_PIN, false); // Apaga LED verde
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

// Função para gerar um obstáculo aleatório na linha superior
void generate_obstacle() {
    uint x = rand() % 5;
    space[0][x] = OBSTACLE;
}

// Função para mover os obstáculos para baixo
void move_obstacles() {
    // Primeiro, limpa obstáculos antigos da linha da nave que foram evitados na rodada anterior
    for (int x = 0; x < 5; x++) {
        if (space[4][x] == OBSTACLE && x != ship_pos) {
            space[4][x] = EMPTY;
        }
    }

    // Move os obstáculos de cima para baixo
    for (int y = 5 - 2; y >= 0; y--) {
        for (int x = 0; x < 5; x++) {
            if (space[y][x] == OBSTACLE) {
                // Verifica colisão com a nave
                if (y + 1 == 4 && x == ship_pos) {
                    play_denied_sound();
                    collision = true;
                }

                // Move o obstáculo para baixo
                space[y][x] = EMPTY;
                space[y + 1][x] = OBSTACLE;
            }
        }
    }
}

// Função para mover a nave com base na entrada do joystick
void move_ship(uint16_t x) {
    // Apaga nave da posição anterior
    space[4][ship_pos] = EMPTY;

    // Atualiza posição com base na entrada analógica
    if (x < 2048 - MARGIN_OF_ERROR) {
        if (ship_pos > 0) {
            
            if(space[4][ship_pos - 1] == 2){
                collision = true; // Colisão com o obstáculo
                play_denied_sound(); // Toca o som de colisão
                return;
            }
            ship_pos--;
        }
        else {
            if(space[4][4] == 2){
                collision = true; // Colisão com o obstáculo
                play_denied_sound(); // Toca o som de colisão
                return;
            }
            ship_pos = 4; // Se a nave estiver na borda esquerda, volta para a direita
        }
        
    } else if (x > 2048 + MARGIN_OF_ERROR) {
        if(ship_pos < 4) {
            if(space[4][ship_pos + 1] == 2){
                collision = true; // Colisão com o obstáculo
                play_denied_sound(); // Toca o som de colisão
                return;
            }
            ship_pos++;
        }
        else {
            if(space[4][0] == 2){
                collision = true; // Colisão com o obstáculo
                play_denied_sound(); // Toca o som de colisão
                return;
            }
            ship_pos = 0; // Se a nave estiver na borda direita, volta para a esquerda
        }
    }

    // Atualiza matriz
    space[4][ship_pos] = SHIP;
}

// Função para resetar a matriz de LEDs e a posição da nave
void reset_space() {
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            space[y][x] = EMPTY;
        }
    }
    ship_pos = 2; // Reseta a posição da nave
    space[4][ship_pos] = SHIP; // Coloca a nave na posição inicial
}

// Função para desenhar a matriz de LEDs com base na matriz de jogo
void draw_matrix() {
    RGB pixels[NUM_PIXELS];

    for (int i = 0; i < NUM_PIXELS; i++) {
        int x = i % 5;
        int y = i / 5;

        if (space[y][x] == SHIP) {
            pixels[i].R = 0;
            pixels[i].G = 0;
            pixels[i].B = 1;
        } else if (space[y][x] == OBSTACLE) {
            pixels[i].R = 1;
            pixels[i].G = 0;
            pixels[i].B = 0;
        } else {
            pixels[i].R = 0;
            pixels[i].G = 0;
            pixels[i].B = 0;
        }
    }
    desenho_pio(pixels, pio, sm);
}

// Função de callback para os botões
void gpio_irq_handler(uint gpio, uint32_t events){
    if (gpio == BUTTON_A_PIN){
        if (debounce(&button_a_time)){
            collision = false; // Reseta a colisão
            reset_space(); // Reseta a matriz de LEDs
        }
    }
    else if (gpio == BUTTON_B_PIN){
        if (debounce(&button_b_time)){
            clear_matrix();
            ssd1306_fill(&ssd, false); // Limpa o display
            ssd1306_send_data(&ssd); // Atualiza o display
            reset_usb_boot(0,0);
        }
    }
}

// Inicializa todos os periféricos do sistema
void init_all_hardware() {
    stdio_init_all();
    init_display(&ssd);
    init_leds();
    init_buttons();
    buzzer_init_all();
    matrix_init();

    // Configuração de interrupções dos botões
    gpio_set_irq_enabled_with_callback(BUTTON_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Semente aleatória
    srand(to_ms_since_boot(get_absolute_time()));
    draw_matrix(); // Exibe estado inicial
}

// Atualiza lógica do jogo se não houve colisão
void update_game_logic(uint16_t x_value, uint16_t y_value) {
    score += 10;
    move_obstacles();
    generate_obstacle();
    
    if (!collision)
        move_ship(x_value);

    draw_matrix();
    joystic_movimentation(&ssd, x_value, y_value);
}

// Lida com colisão (reset de variáveis)
void handle_collision() {
    score = 0;
    delay = 300;
}

// Verifica e ajusta a progressão do jogo
void check_score_progression() {
    if (score % 500 == 0 && score != 0) {
        play_success_sound();
        if (delay > 100) delay -= 50; // Limita para não ficar muito rápido
    }
}

// Função principal
int main() {
    init_all_hardware();

    while (true) {
        // Leitura do joystick
        adc_select_input(1);
        uint16_t x_value = adc_read();
        adc_select_input(0);
        uint16_t y_value = adc_read();

        if (!collision)
            update_game_logic(x_value, y_value);
        else
            handle_collision();

        check_score_progression();
        sleep_ms(delay);
    }

    return 0;
}
