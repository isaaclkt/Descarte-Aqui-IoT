#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"
#include <ESP32Servo.h>

// Tela oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 21
#define OLED_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// HX711
#define HX711_DT 16
#define HX711_SCK 4

HX711 balanca;

// HC-SR04
#define TRIG_PIN 19
#define ECHO_PIN 18
#define DISTANCIA_DETECCAO 5


// Sensor indutivo
#define SENSOR_METAL 32

// Servo responsável pela separação

#define SERVO_SEPARACAO_PIN 25

Servo servoSeparacao;

// Servo responsável pela portinha do não-metal

#define SERVO_PORTA_PIN 26

Servo servoPorta;

// Posições do servo de separação

#define SERVO_NAO_METAL 20
#define SERVO_METAL 160
#define SERVO_INICIAL 90

// Posições servo da portinha

#define PORTA_FECHADA 0
#define PORTA_ABERTA 90

// Tempos:

#define TEMPO_ESPERA 4000

#define TEMPO_PORTA_ABERTA 5000

#define TEMPO_ESTABILIZACAO 3000


// Distância medida pelo sensor ultrassônico

float medirDistancia() {

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(TRIG_PIN, LOW);

    long duracao = pulseIn(ECHO_PIN, HIGH, 30000);

    if (duracao == 0) {
        return -1;
    }

    float distancia = duracao * 0.0343 / 2;

    return distancia;
}


// Ler peso do objeto na balança e exibir no display

void lerPeso() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("DESCARTE AQUI");

    display.setCursor(0, 20);
    display.println("METAL DETECTADO!");

    display.setCursor(0, 35);
    display.println("Aguardando peso...");

    display.display();

    delay(TEMPO_ESTABILIZACAO);

    if (balanca.is_ready()) {

        long leitura = balanca.read();

        display.clearDisplay();

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(0, 0);
        display.println("DESCARTE AQUI");

        display.setCursor(0, 20);
        display.println("PILHA");

        display.setCursor(0, 35);
        display.println("Peso:");

        display.setCursor(0, 50);
        display.println(leitura);

        display.display();

    } else {

        display.clearDisplay();

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(0, 0);
        display.println("DESCARTE AQUI");

        display.setCursor(0, 25);
        display.println("Erro na balanca!");

        display.display();
    }
}


// Porta servo não metal

void abrirPorta() {

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("NAO PILHA DETECTADO!");

    display.setCursor(0, 20);
    display.println("RETIRE O OBJETO!");

    display.setCursor(0, 35);
    display.println("Porta aberta!");

    display.display();


    servoPorta.write(PORTA_ABERTA);
    delay(TEMPO_PORTA_ABERTA);

    servoPorta.write(PORTA_FECHADA);

    delay(1000);
}


void setup() {

    Serial.begin(115200);


    // Oled

    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

        Serial.println("OLED nao encontrado!");

        while (true);
    }


    // Balanca

    balanca.begin(HX711_DT, HX711_SCK);


    // Ultrassonico

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    digitalWrite(TRIG_PIN, LOW);


    // Sensor indutivo

    pinMode(SENSOR_METAL, INPUT_PULLUP);


    // Servo separacao

    servoSeparacao.attach(SERVO_SEPARACAO_PIN);

    servoSeparacao.write(SERVO_INICIAL);


    // Servo porta

    servoPorta.attach(SERVO_PORTA_PIN);

    servoPorta.write(PORTA_FECHADA);

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.println("DESCARTE AQUI");

    display.setCursor(0, 20);
    display.println("Sistema iniciado!");

    display.display();

    delay(2000);
}

void loop() {

    float distancia = medirDistancia();

    int estadoMetal = digitalRead(SENSOR_METAL);

    //Objeto detectado

    if (distancia > 0 && distancia <= DISTANCIA_DETECCAO) {

        //Caminho da pilha
        if (estadoMetal == LOW) {

            servoSeparacao.write(SERVO_METAL);

            display.clearDisplay();

            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);

            display.setCursor(0, 0);
            display.println("DESCARTE AQUI");

            display.setCursor(0, 20);
            display.println("Objeto detectado!");

            display.setCursor(0, 35);
            display.println("PILHA");

            display.setCursor(0, 50);
            display.println("Separando...");

            display.display();

            delay(TEMPO_ESPERA);

            lerPeso();

            servoSeparacao.write(SERVO_INICIAL);

            delay(1000);
        }

        //Caminho da não pilha

        else {

            servoSeparacao.write(SERVO_NAO_METAL);

            display.clearDisplay();

            display.setTextSize(1);
            display.setTextColor(SSD1306_WHITE);

            display.setCursor(0, 0);
            display.println("DESCARTE AQUI");

            display.setCursor(0, 20);
            display.println("Objeto detectado!");

            display.setCursor(0, 35);
            display.println("NAO E PILHA");

            display.setCursor(0, 50);
            display.println("Separando...");

            display.display();

            delay(TEMPO_ESPERA);

            abrirPorta();

            servoSeparacao.write(SERVO_INICIAL);

            delay(1000);
        }
    }


    //Nenhum objeto detectado

    else {

        display.clearDisplay();

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(0, 0);
        display.println("DESCARTE AQUI");

        display.setCursor(0, 25);
        display.println("Aguardando objeto...");

        display.display();
    }

    delay(300);
}