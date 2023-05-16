#include <Wire.h>;
#include <LiquidCrystal_I2C.h>;
LiquidCrystal_I2C lcd(0x27,20,4);

// Pinos referentes a conexão do motor
#define dirPin 37
#define stepPin 38
#define enaPin 39

// Pinos referentes a botoes de interrupcao.
#define emergenciaPin 2
#define trabalhoPin   3
#define sensorTopoPin 16
#define sensorBasePin 43

// Pinos referentes a botos de movimentacao do motor manual.
#define moveBaixoPin 4
#define moveCimaPin  5

// Pinos referentes ao potenciometro de velocidade
#define velocidaTrabalhoPin A10

//Pinos referentes ao Rele
#define relePin 29

// Definicao para sentido de movimento
#define movimentoBaixo HIGH
#define movimentoCima  LOW

enum Status {PARADO, TRABALHO, FINAL, EMERGENCIA};
Status status     = PARADO;
Status statusTela = FINAL;

bool finalTrabalho = false;

unsigned long ciclos = 0;
unsigned long velocidadeTrabalho  = 600;
unsigned long velocidadeMovimento = 20;
unsigned long velocidadeTela      = 600;
unsigned long velTrabalhoHash[10] = {1500, 1400, 1300, 1200, 1100, 1000, 900, 800, 700, 500};
unsigned long contador = 0;

bool lcdNeedUpdate = true;

int sensorBaseEstado = HIGH;
int sensorBaseUltimoEstado = HIGH;
unsigned long sensorBaseUltimoDebounceTime = 0;

int sensorTopoEstado = HIGH;
int sensorTopoUltimoEstado = HIGH;
unsigned long sensorTopoUltimoDebounceTime = 0;

int botaoTrabalhoEstado = HIGH;
int botaoTrabalhoUltimoEstado = HIGH;
unsigned long botaoTrabalhoUltimoDebounceTime = 0;

int botaoEmergenciaEstado = HIGH;
int botaoEmergenciaUltimoEstado = HIGH;
unsigned long botaoEmergenciaUltimoDebounceTime = 0;

int botaoMoveBaixoEstado = HIGH;
int botaoMoveBaixoUltimoEstado = HIGH;
unsigned long botaoMoveBaixoUltimoDebounceTime = 0;

int botaoMoveCimaEstado = HIGH;
int botaoMoveCimaUltimoEstado = HIGH;
unsigned long botaoMoveCimaUltimoDebounceTime = 0;


unsigned long debounceDelay = 50;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("RGTX PERFORMANCE");
  lcd.setCursor(13, 3);
  lcd.print("QTD:  0");

  // Define atuacao e tipo dos pinos.
  pinMode(emergenciaPin, INPUT_PULLUP);
  pinMode(trabalhoPin  , INPUT_PULLUP);
  pinMode(sensorTopoPin, INPUT_PULLUP);
  pinMode(sensorBasePin, INPUT_PULLUP);
  pinMode(moveBaixoPin , INPUT_PULLUP);
  pinMode(moveCimaPin  , INPUT_PULLUP);

  // Define Rele.
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, LOW);

  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enaPin, OUTPUT);

  digitalWrite(enaPin, HIGH);

  status = PARADO;
  Serial.begin(9600);
}

void loop() {

  if (LOW == lerBotaoEmergencia()){
   emergencia();
  } else {
    if (LOW == lerBotaoTrabalho()) {
      trabalho();
    }

    if (TRABALHO == status) {
      
      if (LOW == lerSensorBase()) {
        sensorBase();
      }
      
    }
  }
  
  velocidadeTrabalho =  map(analogRead(velocidaTrabalhoPin), 0, 1023, 1, 10);
  updateLcdScreen();
  
  switch (status) {
    case EMERGENCIA:
      digitalWrite(relePin, LOW);
      digitalWrite(enaPin, HIGH);
      
      if (HIGH == lerBotaoEmergencia()) {
        status = PARADO;
        ciclos = 0;
      }
      
      break;
    
    case PARADO:
      digitalWrite(relePin, LOW);
      
      if (LOW == lerBotaoMoveBaixo() && HIGH == lerSensorBase()) {
        digitalWrite(dirPin, movimentoBaixo);
        moveMotor(velocidadeMovimento);
      } else if (LOW == lerBotaoMoveCima() && HIGH == lerSensorTopo()) {
          digitalWrite(dirPin, movimentoCima);
          moveMotor(velocidadeMovimento);
      } else {
        digitalWrite(enaPin, HIGH);
      }
      
      break;
    
    case TRABALHO:
      if(0 == ciclos) {
        digitalWrite(relePin, HIGH);
        delay(2000);
      }
      digitalWrite(dirPin, movimentoBaixo);
      moveMotor(1200);
      ciclos++;
      break;
    
    case FINAL:
      if (ciclos > 0) {
        if (finalTrabalho) {
          digitalWrite(enaPin, HIGH);
          digitalWrite(relePin, LOW);
          finalTrabalho = false;
          delay(2000);
        }
        
        digitalWrite(dirPin, movimentoCima);
        moveMotor(velocidadeMovimento);
        ciclos--;

      } else {
        status = PARADO;
      }

      break;
    
    default:
      status = PARADO;
      break;
  }
}

void moveMotor(int velocidade) {
  if (HIGH == digitalRead(enaPin)) {
    digitalWrite(enaPin, LOW);
  }
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(velocidade);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(velocidade);
}

void emergencia() {
    status = EMERGENCIA;
    ciclos = 0;
}

void trabalho() {
    if (FINAL == status) {
      return;
    }
    status = TRABALHO;
}

void sensorBase() {
    switch (status) {
      case TRABALHO:
        status = FINAL;
        finalTrabalho = true;
        contador++;
        updateLcdScreen();
        break;
      case FINAL:
        status = FINAL;
        break;
      default:
        status = PARADO;
    }
}

void updateLcdScreen() {
  
  if (status != statusTela) {
    statusTela = status;
    lcdNeedUpdate = true;

    lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(0, 2);
  } else {
    lcdNeedUpdate = false;
  }
  
  switch (status) {
    case EMERGENCIA:
      if (true == lcdNeedUpdate) {
        lcd.print("EM EMERGENCIA !!!");
      }
      
      break;

    case PARADO:
      if (true == lcdNeedUpdate) {
        lcd.print("VEL TRABALHO: ");
        lcd.print(velocidadeTrabalho);
      }

      if (velocidadeTela != velocidadeTrabalho) {
        velocidadeTela = velocidadeTrabalho;
        lcd.setCursor(14, 2);
        lcd.print(velocidadeTrabalho);
      }
      
      break;
    
    case FINAL:
      if (true == lcdNeedUpdate) {
        lcd.print("FINAL DE CICLO ...");
        if (contador < 10) {
          lcd.setCursor(19, 3);
        } else if(contador < 100) {
          lcd.setCursor(18, 3);
        } else {
          lcd.setCursor(17, 3);
        }

        lcd.print(contador);
      }
      
      break;
    
    case TRABALHO:
      if (true == lcdNeedUpdate) {
        lcd.print("EM TRABALHO ...");
      }
      break;
  }  
}

int lerSensorBase() {
  int sensorBaseLeitura = digitalRead(sensorBasePin);
      if (sensorBaseLeitura != sensorBaseUltimoEstado) {
        sensorBaseUltimoDebounceTime = millis();
      }

      if ((millis() - sensorBaseUltimoDebounceTime) > debounceDelay) {
        if (sensorBaseLeitura != sensorBaseEstado) {
          sensorBaseEstado = sensorBaseLeitura;
        }
      }

      sensorBaseUltimoEstado = sensorBaseLeitura;

      return sensorBaseEstado;
}

int lerSensorTopo() {
  int sensoTopoLeitura = digitalRead(sensorTopoPin);
      if (sensoTopoLeitura != sensorTopoUltimoEstado) {
        sensorTopoUltimoDebounceTime = millis();
      }

      if ((millis() - sensorTopoUltimoDebounceTime) > debounceDelay) {
        if (sensoTopoLeitura != sensorTopoEstado) {
          sensorTopoEstado = sensoTopoLeitura;
        }
      }

      sensorTopoUltimoEstado = sensoTopoLeitura;

      return sensorTopoEstado;
}

int lerBotaoTrabalho() {
  int botaoTrabalhoLeitura = digitalRead(trabalhoPin);
      if (botaoTrabalhoLeitura != botaoTrabalhoUltimoEstado) {
        botaoTrabalhoUltimoDebounceTime = millis();
      }

      if ((millis() - botaoTrabalhoUltimoDebounceTime) > debounceDelay) {
        if (botaoTrabalhoLeitura != botaoTrabalhoEstado) {
          botaoTrabalhoEstado = botaoTrabalhoLeitura;
        }
      }

      botaoTrabalhoUltimoEstado = botaoTrabalhoLeitura;

      return botaoTrabalhoEstado;
}

int lerBotaoEmergencia() {
  int  botaoEmergencia = digitalRead(emergenciaPin);
      if (botaoEmergencia != botaoEmergenciaUltimoEstado) {
        botaoEmergenciaUltimoDebounceTime = millis();
      }

      if ((millis() - botaoEmergenciaUltimoDebounceTime) > debounceDelay) {
        if ( botaoEmergencia != botaoEmergenciaEstado) {
          botaoEmergenciaEstado = botaoEmergencia;
        }
      }

      botaoEmergenciaUltimoEstado = botaoEmergencia;

      return botaoEmergenciaEstado;
}

int lerBotaoMoveBaixo() {
  int  botaoMoveBaixo = digitalRead(moveBaixoPin);
      if (botaoMoveBaixo != botaoMoveBaixoUltimoEstado) {
        botaoMoveBaixoUltimoDebounceTime = millis();
      }

      if ((millis() - botaoMoveBaixoUltimoDebounceTime) > debounceDelay) {
        if ( botaoMoveBaixo != botaoMoveBaixoEstado) {
          botaoMoveBaixoEstado = botaoMoveBaixo;
        }
      }

      botaoMoveBaixoUltimoEstado = botaoMoveBaixo;

      return botaoMoveBaixoEstado;
}

int lerBotaoMoveCima() {
  int  botaoMoveCima = digitalRead(moveCimaPin);
      if (botaoMoveCima != botaoMoveCimaUltimoEstado) {
        botaoMoveCimaUltimoDebounceTime = millis();
      }

      if ((millis() - botaoMoveCimaUltimoDebounceTime) > debounceDelay) {
        if ( botaoMoveCima != botaoMoveCimaEstado) {
          botaoMoveCimaEstado = botaoMoveCima;
        }
      }

      botaoMoveCimaUltimoEstado = botaoMoveCima;

      return botaoMoveCimaEstado;
}