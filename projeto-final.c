#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "include/ssd1306.h"
#include "hardware/pwm.h"

// Definições dos pinos
#define LED_RED 13
#define LED_GREEN 11
#define JOYSTICK_X 26
#define BUZZER 21

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define STEP 10

int x = 100;
int alert = 0;

PIO pio;
uint sm;
ssd1306_t ssd; // Inicializa a estrutura do display
bool cor = true;

// Protótipo das funções
void setup_all();
int clamp(int value, int min, int max);
void beep_alerta(int alert);
void atualizar_display(int valor, const char *linha2, const char *linha3);

int main()
{
    stdio_init_all();
    setup_all();

    while (true)
    {
        adc_select_input(0);
        uint16_t vrx_value = adc_read(); // Eixo X

        x += (vrx_value < 1900) ? -STEP : (vrx_value > 2100 ? STEP : 0);
        x = clamp(x, 0, 300);
        printf("%d\n", x);

        if (x < 30){
            alert = 3; // Nível Crítico
        }
        else if (x < 50){
            alert = 2; // Nível Alto
        }
        else if (x < 80){
            alert = 1; // Nível Médio
        }
        else {
            alert = 0; // Sem alerta
        }

        beep_alerta(alert);

        if (alert == 0)
        {
            gpio_put(LED_GREEN, true);
            gpio_put(LED_RED, false);
            atualizar_display(x, "CAMINHO", "LIVRE");
        }
        else
        {
            gpio_put(LED_RED, true);
            gpio_put(LED_GREEN, false);
            atualizar_display(x, "OBSTACULO", "DETECTADO"); 
        }

        sleep_ms(10);
    }
}

// Função inicializa os pinos para LEDs e botões
void setup_all()
{
    // Inicialização e configuração do LED
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_put(LED_RED, false);
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_put(LED_GREEN, false);
    // Inicialização e configuração do joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    // Buzzer
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(BUZZER, 0);
    // Display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);                                      // Envia os dados para o display
    ssd1306_fill(&ssd, false);                                    // Limpa o display
    ssd1306_send_data(&ssd);
}

int clamp(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

void beep_alerta(int nivel)
{
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    uint32_t clock = clock_get_hz(clk_sys);

    // Frequências e padrões para cada nível de alerta
    uint frequencia;
    int beeps, duracao, intervalo;

    switch (nivel)
    {
    case 3: // Nível Crítico (Muito Perto)
        frequencia = 4000;
        beeps = 3;
        duracao = 50;
        intervalo = 20;
        break;
    case 2: // Nível Alto
        frequencia = 2000;
        beeps = 1;
        duracao = 50;
        intervalo = 75;
        break;
    case 1: // Nível Médio
        frequencia = 1000;
        beeps = 1;
        duracao = 100;
        intervalo = 100;
        break;
    default: // Sem alerta
        return;
    }

    uint32_t divisor = clock / (frequencia * 4096);
    pwm_set_clkdiv(slice_num, divisor);

    for (int i = 0; i < beeps; i++)
    {
        pwm_set_gpio_level(BUZZER, 2048);
        sleep_ms(duracao);
        pwm_set_gpio_level(BUZZER, 0);
        sleep_ms(intervalo);
    }
}

// Função atualiza o display com duas linhas de texto
void atualizar_display(int valor, const char *linha2, const char *linha3)
{
    ssd1306_fill(&ssd, !cor);
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor);

    char buffer[12];
    sprintf(buffer, "%d cm", valor);

    ssd1306_draw_string(&ssd, buffer, 40, 10);
    ssd1306_draw_string(&ssd, linha2, 20, 25);
    ssd1306_draw_string(&ssd, linha3, 20, 35);

    ssd1306_send_data(&ssd);

    ssd1306_fill(&ssd, !cor);
    ssd1306_send_data(&ssd);
}