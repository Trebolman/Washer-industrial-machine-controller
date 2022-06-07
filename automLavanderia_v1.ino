#include <OneWire.h>
#include <DallasTemperature.h>
#include <HX710B.h>
#include <AsyncTaskLib.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Iniciamos el lcd
const int rs = 19, en = 18, d4 = 17, d5 = 16, d6 = 15, d7 = 14; //
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Configuramos sensor de temperatura
// Use software SPI: CS, DI, DO, CLK
// Adafruit_MAX31865 thermo = Adafruit_MAX31865(53, 50, 51, 52);
// Adafruit_MAX31865 thermo = Adafruit_MAX31865(41);
// rdi = pin 39
// #define RREF 430.0
// #define RNOMINAL 100.0
// #include <OneWire.h>
// #include <DallasTemperature.h>
OneWire bus(47);
DallasTemperature thermo(&bus);
DeviceAddress sensorTemperatura = {0x28, 0xFF, 0x7, 0x3, 0x93, 0x16, 0x4, 0x7A};
uint8_t resolucion = 9;

// Configuramos sensor de presion
// SCK, SDI
// HX710B air_press(27, 29);
const int DOUT = 27; // sensor data pin
const int SCLK = 29; // sensor clock pin
HX710B pressure_sensor;

// Entradas
#define btnParar 2               //
#define btnDisminuir 3           //
#define btnAumentar 4            //
#define btnEditar 24             //
#define btnProgramarNivelAgua 43 //
#define btnComenzar 26           //
#define sensorPuerta 23          //
// #define reservaAnalogica A0
// #define reserva 25

// Salidas
#define MotorDirA 12    //
#define MotorDirB 11    //
#define ValvulAgua 10   //
#define ElectrovVapor 9 //
#define ValvulOnOff 8   //
#define MagnetPuerta 7  //
#define buzzer 41       //
// #define reserva 6
// #define reserva 5

// Definimos variables de los programas
uint8_t NivelAgua[3][4];
uint8_t RotacionTam[3][4];
uint8_t TemperaturaLim[3][4];
uint8_t TemporizadorLim[3][4];
uint8_t TiempoRotacion[3][2] = {{3, 2}, {3, 3}, {4, 3}};
uint8_t TiempoEntFase[3][4] = {{3, 3, 3, 5}, {4, 3, 2, 3}, {4, 3, 4, 5}};

// Variables bandera
boolean tiempoCumplido = false;
boolean programaTerminado = true;
boolean programaEnPausa = false;
boolean editandoProgramaEjecucion = false;
boolean nivelActivo = LOW;

// Antirebote
uint8_t flagBtnAumentar = 0;
uint8_t flagBtnDisminuir = 0;
uint8_t flagBtnComenzar1 = 0;
uint8_t flagBtnEditar1 = 0;
uint8_t flagBtnParar1 = 0;

// Variables de proceso
boolean pausa = false;
uint8_t programa = 1;
uint8_t numeroVariable = 1;
uint8_t valorVariable = 0;
uint8_t fase = 1;
uint8_t nivelEdicion = 0;
uint8_t direccion = 1;

int8_t minutos[2] = {0, 0};
int8_t segundos[2] = {0, 0};
int16_t segunderoTemporizador = 0;
uint8_t tiempoRotacion = 0;
uint8_t tiempoPausa = 0;
uint8_t segunderoEntreFase = 0;

// Variables para manejo de sensor temperatura
const int entradaAnalogica = A0;
float VoltajeRef = 0.0048875;
float VoltajeTP100 = 0.0;
float ResistenciaRef = 120.0;
float lecturaPin = 0.0;
float VoltajeIni = 5.0;
float resistenciaTP100 = 0.0;
float temperaturaMin = -50.0;
float temperaturaMax = 650.0;
float resistenciaMin = 80.31;
float resistenciaMax = 329.64;
float ValorCalibracion = -28.2;
uint8_t valorTemperatura = 0;
uint8_t valorTemperaturaLim = 0;
boolean sensarTemperatura = true;
// int valorTemperatura = 0;
// float sensorValue0 = 0;
// float temp0 = 0;

// Sensor de presion
uint16_t nivelPresion1 = 650;
uint16_t nivelPresion2 = 750;
uint16_t nivelPresion3 = 850;
uint16_t nivelPresion4 = 950;

uint16_t valorPresion = 0;
uint16_t valorPresionLim = 0;
uint8_t valorNivel = 1;
boolean sensarPresion = false;

AsyncTask segundosMotor(1000, true, []()
                        { segundos[0]++; });

void pintarVariables()
{
  lcd.setCursor(6, 0);
  lcd.print(fase);

  lcd.setCursor(10, 0);
  lcd.print(valorNivel);

  lcd.setCursor(14, 0);
  lcd.print(valorTemperatura);

  lcd.setCursor(3, 1);
  if (minutos[1] < 10)
  {
    lcd.print(0);
    lcd.print(minutos[1]);
  }
  else
  {
    lcd.print(minutos[1]);
  }
  lcd.setCursor(6, 1);
  if (segundos[1] < 10)
  {
    lcd.print(0);
    lcd.print(segundos[1]);
  }
  else
  {
    lcd.print(segundos[1]);
  }
}

AsyncTask segundosTemporizador(1000, true, []()
                               { 
  segunderoTemporizador--;
  minutos[1] = (segunderoTemporizador / 60);
  segundos[1] = segunderoTemporizador - (minutos[1] * 60); 
  controladorSensorTemperatura();
  controladorSensorPresion();
  if (!editandoProgramaEjecucion){ pintarVariables(); 
  } });

void setup()
{
  // Definimos entradas
  pinMode(btnAumentar, INPUT);
  pinMode(btnComenzar, INPUT);
  pinMode(btnDisminuir, INPUT);
  pinMode(btnEditar, INPUT);
  pinMode(btnParar, INPUT);

  // Definimos salidas
  pinMode(MagnetPuerta, OUTPUT);
  pinMode(ValvulAgua, OUTPUT);
  pinMode(MotorDirA, OUTPUT);
  pinMode(MotorDirB, OUTPUT);
  pinMode(ElectrovVapor, OUTPUT);
  pinMode(ValvulOnOff, OUTPUT);

  // Inicializamos salidas
  digitalWrite(MagnetPuerta, LOW);
  digitalWrite(ValvulAgua, LOW);
  digitalWrite(ElectrovVapor, LOW);
  digitalWrite(ValvulOnOff, LOW);
  digitalWrite(MotorDirA, LOW);
  digitalWrite(MotorDirB, LOW);

  // Inicializamos el lcd
  lcd.begin(16, 2);

  // Iniciamos el modulo MAX31865
  // thermo.begin(MAX31865_2WIRE); // set to 2WIRE or 4WIRE as necessary
  // valorTemperatura = analogRead(A0);
  thermo.begin();
  thermo.setResolution(resolucion);
  // thermo.setResolution(thermo, resolucion);

  // Inicializamos el puerto serial para depurar
  Serial.begin(115200);
  Serial.println("Puerto serial iniciado");

  // Iniciamos sensor de presion
  pressure_sensor.begin(DOUT, SCLK);

  // Recuperamos valores del EEPROM
  recuperarValoresEEPROM();

  // inicializamos subprocesos
  pintarVentanaSeleccion();
  pintarConsolaSerial();
}

void loop()
{
  // Eligiendo programa
  if (digitalRead(btnAumentar) == nivelActivo)
  {
    if (flagBtnAumentar == 0)
    {
      flagBtnAumentar = 1;
      programa++;
      if (programa > 3)
        programa = 1;
      pintarVentanaSeleccion();
      pintarConsolaSerial();
    }
  }
  else
  {
    flagBtnAumentar = 0;
  }

  if (digitalRead(btnDisminuir) == nivelActivo)
  {
    if (flagBtnDisminuir == 0)
    {
      flagBtnDisminuir = 1;
      programa--;
      if (programa < 1)
      {
        programa = 3;
      }
      pintarVentanaSeleccion();
      pintarConsolaSerial();
    }
  }
  else
  {
    flagBtnDisminuir = 0;
  }

  // Comenzando programa seleccionado
  if (digitalRead(btnComenzar) == nivelActivo)
  {
    if (flagBtnComenzar1 == 0)
    {
      flagBtnComenzar1 = 1;
      iniciarPrograma();
      while (!tiempoCumplido)
      {
        controladorTemporizador();
        controladorDireccionMotor();
        // controladorSensorTemperatura();
        if (digitalRead(btnParar) == nivelActivo)
        {
          if (flagBtnParar1 == 0)
          {
            Serial.println("Programa terminado con boton");
            flagBtnParar1 = 1;
            terminarPrograma();
            // eeprom_write();
            // delay(100);
          }
        }
        else
        {
          flagBtnParar1 = 0;
        }

        if (digitalRead(btnEditar) == nivelActivo)
        {
          if (flagBtnEditar1 == 0)
          {
            flagBtnEditar1 = 1;
            editandoProgramaEjecucion = true;
            editarPrograma();
          }
        }
        else
        {
          flagBtnEditar1 = 0;
        }
      }
    }
  }
  else
  {
    flagBtnComenzar1 = 0;
  }

  // Edicion de programa
  if (digitalRead(btnEditar) == nivelActivo)
  {
    if (flagBtnEditar1 == 0)
    {
      // Edicion de numero de variable
      flagBtnEditar1 = 1;
      editarPrograma();
    }
  }
  else
  {
    flagBtnEditar1 = 0;
  }

  if (digitalRead(btnParar) == nivelActivo)
  {
    if (flagBtnParar1 == 0)
    {
      guardarValoresEEPROM();
    }
  }
  else
  {
    flagBtnParar1 = 0;
  }
}

// Subprocesos de pintar el LCD
void pintarVentanaSeleccion()
{
  lcd.clear();
  // primera fila
  lcd.setCursor(0, 0);
  lcd.print("P");
  lcd.setCursor(1, 0);
  lcd.print(programa + 21);

  lcd.setCursor(4, 0);
  lcd.print("N");
  for (uint8_t i = 0; i < 4; i++)
  {
    lcd.setCursor(i + 5, 0);
    lcd.print(NivelAgua[programa - 1][i]);
  }

  lcd.setCursor(10, 0);
  lcd.print("T");
  for (uint8_t i = 0; i < 4; i++)
  {
    lcd.setCursor(i + 11, 0);
    lcd.print(TemporizadorLim[programa - 1][i]);
  }

  // Segunda fila
  lcd.setCursor(0, 1);
  lcd.print("T");
  lcd.setCursor(2, 1);
  lcd.print("/");
  lcd.setCursor(5, 1);
  lcd.print("/");
  lcd.setCursor(8, 1);
  lcd.print("/");
  for (uint8_t i = 0; i < 4; i++)
  {
    if (TemperaturaLim[programa - 1][i] < 10)
    {
      lcd.setCursor(3 * i, 1);
      lcd.print(0);
      lcd.print(TemperaturaLim[programa - 1][i]);
    }
    else
    {
      lcd.setCursor(3 * i, 1);
      lcd.print(TemperaturaLim[programa - 1][i]);
    }
  }

  lcd.setCursor(11, 1);
  lcd.print("R");
  for (uint8_t i = 0; i < 4; i++)
  {
    lcd.setCursor(i + 12, 1);
    lcd.print(RotacionTam[programa - 1][i]);
  }
}

void pintarVentanaEdicionMenu()
{
  lcd.clear();
  // primera fila
  lcd.setCursor(0, 0);
  lcd.print("P");
  lcd.setCursor(1, 0);
  lcd.print(programa + 21);

  lcd.setCursor(4, 0);
  lcd.print("Seleccione:");

  // Segunda fila
  switch (numeroVariable)
  {
  case 1:
    lcd.setCursor(0, 1);
    lcd.print("Fase:");
    lcd.setCursor(5, 1);
    lcd.print(fase);
    lcd.setCursor(8, 1);
    lcd.print("Nivel:");
    lcd.setCursor(14, 1);
    lcd.print(NivelAgua[programa - 1][fase - 1]);
    break;

  case 2:
    lcd.setCursor(0, 1);
    lcd.print("Fase:");
    lcd.setCursor(5, 1);
    lcd.print(fase);
    lcd.setCursor(8, 1);
    lcd.print("Tiemp:");
    lcd.setCursor(14, 1);
    lcd.print(TemporizadorLim[programa - 1][fase - 1]);
    break;

  case 3:
    lcd.setCursor(0, 1);
    lcd.print("Fase:");
    lcd.setCursor(5, 1);
    lcd.print(fase);
    lcd.setCursor(8, 1);
    lcd.print("Tempe:");
    lcd.setCursor(14, 1);
    lcd.print(TemperaturaLim[programa - 1][fase - 1]);
    break;

  case 4:
    lcd.setCursor(0, 1);
    lcd.print("Fase:");
    lcd.setCursor(5, 1);
    lcd.print(fase);
    lcd.setCursor(8, 1);
    lcd.print("VRota:");
    lcd.setCursor(14, 1);
    lcd.print(RotacionTam[programa - 1][fase - 1]);
    break;

  default:
    break;
  }
}

void pintarVentanaEjecucion()
{
  lcd.clear();
  // primera fila
  lcd.setCursor(0, 0);
  lcd.print("P");
  lcd.setCursor(1, 0);
  lcd.print(programa + 21);

  lcd.setCursor(4, 0);
  lcd.print("F:");
  lcd.setCursor(6, 0);
  lcd.print(fase);

  lcd.setCursor(8, 0);
  lcd.print("N:");
  lcd.setCursor(10, 0);
  lcd.print(valorNivel);

  lcd.setCursor(12, 0);
  lcd.print("T:");
  lcd.setCursor(14, 0);
  lcd.print(valorTemperatura);

  // Segunda fila
  lcd.setCursor(0, 1);
  lcd.print("Ti:");
  lcd.setCursor(3, 1);
  if (minutos[1] < 10)
  {
    lcd.print(0);
    lcd.print(minutos[1]);
  }
  else
  {
    lcd.print(minutos[1]);
  }
  lcd.setCursor(5, 1);
  lcd.print(":");
  lcd.setCursor(6, 1);
  if (segundos[1] < 10)
  {
    lcd.print(0);
    lcd.print(segundos[1]);
  }
  else
  {
    lcd.print(segundos[1]);
  }
  lcd.setCursor(12, 1);
  lcd.print("R:");
  lcd.setCursor(14, 1);
  lcd.print(RotacionTam[programa - 1][fase - 1]);
}

// Subprocesos de obtencion/edicin de variables
void editarValorVariable()
{
  switch (numeroVariable)
  {
  case 1:
    NivelAgua[programa - 1][fase - 1] = valorVariable;
    break;

  case 2:
    TemporizadorLim[programa - 1][fase - 1] = valorVariable;
    break;

  case 3:
    TemperaturaLim[programa - 1][fase - 1] = valorVariable;
    break;

  case 4:
    RotacionTam[programa - 1][fase - 1] = valorVariable;
    break;

  default:
    break;
  }
}

void obtenerValorVariable()
{
  switch (numeroVariable)
  {
  case 1:
    valorVariable = NivelAgua[programa - 1][fase - 1];
    break;

  case 2:
    valorVariable = TemporizadorLim[programa - 1][fase - 1];
    break;

  case 3:
    valorVariable = TemperaturaLim[programa - 1][fase - 1];
    break;

  case 4:
    valorVariable = RotacionTam[programa - 1][fase - 1];
    break;

  default:
    break;
  }
}

// subprecesos del temporizador
void iniciarTemporizador()
{
  // minutos[1] = min;
  uint16_t segunderoTemp = 0;
  if (!programaEnPausa)
  {
    // segunderoTemporizador = TemporizadorLim[programa - 1][fase - 1] * 60;
    segunderoTemp = TemporizadorLim[programa - 1][fase - 1];
    segunderoTemporizador = segunderoTemp * 10;
    Serial.print("SegunderoTemporizador en ejecucion: ");
    Serial.println(segunderoTemporizador);
  }
  else
  {
    segunderoTemp = TiempoEntFase[programa - 1][fase - 1];
    segunderoTemporizador = segunderoTemp;
    Serial.print("SegunderoTemporizador en pausa: ");
    Serial.println(segunderoTemporizador);
  }
  // Serial.println(segunderoTemporizador);
  segundosTemporizador.Start();
}

void iniciarTemporizadorMotor()
{
  tiempoRotacion = TiempoRotacion[RotacionTam[programa - 1][fase - 1] - 1][0];
  tiempoPausa = TiempoRotacion[RotacionTam[programa - 1][fase - 1] - 1][1];
}

void iniciarSensorTemperatura()
{
  sensarTemperatura = true;
  valorTemperaturaLim = TemperaturaLim[programa-1][fase-1];
}

void iniciarSensorPresion()
{
  sensarPresion = true;
  valorPresionLim = NivelAgua[programa-1][fase-1];
}

void controladorTemporizador()
{
  if (!programaTerminado)
  {
    segundosTemporizador.Update();
    if (!programaEnPausa)
    {
      if (segunderoTemporizador == 0)
      {
        pausarPrograma();
        iniciarTemporizador();
        pintarConsolaSerial();
      }
    }
    else
    {
      if (segunderoTemporizador == 0)
      {
        segundosTemporizador.Stop();
        fase++;
        reiniciarPrograma();
        if (fase > 4)
        {
          terminarPrograma();
        }
        else
        {
          iniciarTemporizador();
        }
        pintarConsolaSerial();
      }
    }
  }
}

// Subprocesos del motor
void controladorDireccionMotor()
{
  if (!programaTerminado || !programaEnPausa)
  {
    segundosMotor.Update();
    switch (direccion)
    {
    case 1:
      if (segundos[0] == tiempoRotacion)
      {
        direccion = 2;
        pausa = true;
        segundos[0] = 0;
        digitalWrite(MotorDirA, LOW);
        digitalWrite(MotorDirB, LOW);
        // Serial.print("Controlador motor - direccion");
        // Serial.println(direccion);
      }
      break;

    case 2:
      if (segundos[0] == tiempoPausa)
      {
        if (pausa == true)
        {
          direccion = 3;
          digitalWrite(MotorDirA, LOW);
          digitalWrite(MotorDirB, HIGH);
        }
        else
        {
          direccion = 1;
          digitalWrite(MotorDirA, HIGH);
          digitalWrite(MotorDirB, LOW);
        }
        segundos[0] = 0;
      }
      // Serial.print("Controlador motor - direccion");
      // Serial.println(direccion);
      break;

    case 3:
      if (segundos[0] == tiempoRotacion)
      {
        direccion = 2;
        pausa = false;
        segundos[0] = 0;
        digitalWrite(MotorDirA, LOW);
        digitalWrite(MotorDirB, LOW);
      }
      break;

    default:
      break;
    }
  }
}

// Controlador del sensor de temperatura
void controladorSensorTemperatura()
{
  if (!programaTerminado)
  {
    thermo.requestTemperatures();
    valorTemperatura = round(thermo.getTempCByIndex(0));
    // Serial.print("Valor temperatura (Celcius): ");
    // Serial.println(valorTemperatura);
    if (valorTemperatura >= valorTemperaturaLim && sensarTemperatura)
    {
      sensarTemperatura = false;
      digitalWrite(ElectrovVapor, LOW);
    }
  }
}

// Controlador sensor de presion
void controladorSensorPresion()
{
  if (pressure_sensor.is_ready())
  {
    valorPresion = pressure_sensor.pascal();
    // Serial.print("Valor presion (Pascal): ");
    Serial.println(valorPresion);
    if (valorPresion <= nivelPresion1)
    {
      valorNivel = 1;
    }
    else if (valorPresion > nivelPresion1 && valorPresion <= nivelPresion2)
    {
      valorNivel = 2;
    }
    else if (valorPresion > nivelPresion2 && valorPresion <= nivelPresion3)
    {
      valorNivel = 3;
    }
    else if (valorPresion > nivelPresion3)
    {
      valorNivel = 4;
    }

    if (valorNivel >= valorPresionLim && sensarPresion)
    {
      sensarPresion = false;
      digitalWrite(ValvulAgua, LOW);
    }
  }
  else
  {
    Serial.println("Pressure sensor not found.");
  }
}

// Subprocesos de manejo de programas
void iniciarPrograma()
{
  tiempoCumplido = false;
  programaTerminado = false;
  programaEnPausa = false;
  numeroVariable = 1;
  direccion = 1;
  fase = 1;

  digitalWrite(MagnetPuerta, HIGH);
  digitalWrite(ValvulAgua, HIGH);
  digitalWrite(ElectrovVapor, HIGH);
  digitalWrite(ValvulOnOff, HIGH);

  // reiniciamos temporizadores
  segundos[0] = 0;
  segundos[1] = 0;
  minutos[0] = 0;
  minutos[1] = 0;

  segundosMotor.Start();
  iniciarTemporizador();
  iniciarSensorTemperatura();
  iniciarTemporizadorMotor();
  pintarVentanaEjecucion();
}

void reiniciarPrograma()
{
  programaEnPausa = false;
  iniciarTemporizador();
  digitalWrite(ValvulAgua, HIGH);
  valorTemperaturaLim = TemperaturaLim[programa-1][fase-1];
  valorPresionLim = NivelAgua[programa-1][fase-1];
  if (valorTemperaturaLim > 0)
  {
    iniciarSensorTemperatura();
    digitalWrite(ElectrovVapor, HIGH);
  }
  if (valorPresionLim > 0)
  {
    iniciarSensorPresion();
    digitalWrite(ValvulAgua, HIGH);
  }
  digitalWrite(ValvulOnOff, HIGH);
  segundosMotor.Start();
  segundosTemporizador.Start();
}

void terminarPrograma()
{
  if (!programaTerminado)
  {
    tiempoCumplido = true;
    programaTerminado = true;
    programaEnPausa = false;
    numeroVariable = 1;
    fase = 1;

    // reiniciamos los temporizadores
    segundos[0] = 0;
    segundos[1] = 0;
    minutos[0] = 0;
    minutos[1] = 0;
    segunderoTemporizador = 0;
    segundosMotor.Stop();
    segundosTemporizador.Stop();

    digitalWrite(MotorDirA, LOW);
    digitalWrite(MotorDirB, LOW);
    digitalWrite(MagnetPuerta, LOW);
    digitalWrite(ValvulAgua, LOW);
    digitalWrite(ElectrovVapor, LOW);
    digitalWrite(ValvulOnOff, LOW);

    pintarVentanaSeleccion();
    Serial.println("Programa concluido exitosamente");
  }
}

void pausarPrograma()
{
  programaEnPausa = true;
  segundosMotor.Stop();
  digitalWrite(MotorDirA, LOW);
  digitalWrite(MotorDirB, LOW);
  digitalWrite(ValvulAgua, LOW);
  digitalWrite(ElectrovVapor, LOW);
  digitalWrite(ValvulOnOff, LOW);
}

void recuperarValoresEEPROM()
{
  programa = EEPROM.read(0);
  fase = EEPROM.read(1);
  minutos[1] = EEPROM.read(2);
  segundos[1] = EEPROM.read(3);

  for (uint8_t i = 0; i < 3; i++)
  {
    for (uint8_t j = 0; j < 4; j++)
    {
      NivelAgua[i][j] = EEPROM.read(4 * (i + 1) + j);
      TemperaturaLim[i][j] = EEPROM.read(4 * (i + 4) + j);
      TemporizadorLim[i][j] = EEPROM.read(4 * (i + 7) + j);
      RotacionTam[i][j] = EEPROM.read(4 * (i + 10) + j);
      // TiempoEntFase[i][j] = EEPROM.read(4 * (i + 13) + j);
    }
  }
  Serial.println("Recuperado exitosamente");
}

void guardarValoresEEPROM()
{
  EEPROM.update(0, programa);
  EEPROM.update(1, fase);
  EEPROM.update(2, minutos[1]);
  EEPROM.update(3, segundos[1]);

  for (uint8_t i = 0; i < 3; i++)
  {
    for (uint8_t j = 0; j < 4; j++)
    {
      EEPROM.update(4 * (i + 1) + j, NivelAgua[i][j]);
      EEPROM.update(4 * (i + 4) + j, TemperaturaLim[i][j]);
      EEPROM.update(4 * (i + 7) + j, TemporizadorLim[i][j]);
      EEPROM.update(4 * (i + 10) + j, RotacionTam[i][j]);
    }
  }

  // for (uint8_t i = 0; i < 3; i++)
  // {
  //   for (uint8_t j = 0; j < 2; j++)
  //   {
  //     EEPROM.update(2 * (i + 32) + j, TiempoRotacion[i][j]);
  //   }
  // }
  Serial.println("Guardado exitosamente");

  // valorTemperatura = ReadSensor(); //Lectura simulada del sensor
  // EEPROM.update( eeAddress, valorTemperatura );  //Grabamos el valor
  // eeAddress += sizeof(float);  //Obtener la siguiente posicion para escribir
  // if(eeAddress >= EEPROM.length()) eeAddress = 0;  //Comprobar que no hay desbordamiento
  // delay(30000); //espera 30 segunos
}

void pintarConsolaSerial()
{
  Serial.println("-------------------");
  Serial.print("Programa: ");
  Serial.println(programa);
  Serial.print("Fase: ");
  Serial.println(fase);
  Serial.print("ProgramaTerminado: ");
  Serial.println(programaTerminado);
  Serial.print("Programa en pausa: ");
  Serial.println(programaEnPausa);
  // Serial.print("Valor obtenido: ");
  // Serial.println(valorVariable);
}

void asignarBlinkLCD()
{
  switch (nivelEdicion)
  {
  case 1:
    lcd.setCursor(5, 1);
    break;

  case 2:
    lcd.setCursor(8, 1);
    break;

  case 3:
    lcd.setCursor(14, 1);
    break;

  default:
    break;
  }
}

void editarPrograma()
{
  uint8_t faseTemp = fase;
  numeroVariable = 1;
  nivelEdicion = 1;
  obtenerValorVariable();
  pintarVentanaEdicionMenu();
  lcd.blink();
  // pintarConsolaSerial();
  asignarBlinkLCD();
  while (nivelEdicion == 1)
  {
    if (digitalRead(btnAumentar) == nivelActivo)
    {
      if (flagBtnAumentar == 0)
      {
        flagBtnAumentar = 1;
        fase++;
        if (fase > 4)
        {
          fase = 1;
        }
        pintarVentanaEdicionMenu();
        asignarBlinkLCD();
        // pintarConsolaSerial();
      }
    }
    else
    {
      flagBtnAumentar = 0;
    }

    if (digitalRead(btnDisminuir) == nivelActivo)
    {
      if (flagBtnDisminuir == 0)
      {
        flagBtnDisminuir = 1;
        fase--;
        if (fase < 1)
        {
          fase = 4;
        }
        pintarVentanaEdicionMenu();
        asignarBlinkLCD();
        // pintarConsolaSerial();
      }
    }
    else
    {
      flagBtnDisminuir = 0;
    }

    if (digitalRead(btnEditar) == nivelActivo)
    {
      if (flagBtnEditar1 == 0)
      {
        flagBtnEditar1 = 1;
        nivelEdicion = 2;

        obtenerValorVariable();
        // pintarConsolaSerial();
        asignarBlinkLCD();

        // Edicion de valor de numeroVariable
        while (nivelEdicion == 2)
        {
          if (editandoProgramaEjecucion)
          {
            controladorDireccionMotor();
            controladorTemporizador();
          }
          if (digitalRead(btnAumentar) == nivelActivo)
          {
            if (flagBtnAumentar == 0)
            {
              flagBtnAumentar = 1;
              numeroVariable++;
              if (numeroVariable > 4)
              {
                numeroVariable = 1;
              }
              obtenerValorVariable();
              pintarVentanaEdicionMenu();
              // pintarConsolaSerial();
              asignarBlinkLCD();
            }
          }
          else
          {
            flagBtnAumentar = 0;
          }

          if (digitalRead(btnDisminuir) == nivelActivo)
          {
            if (flagBtnDisminuir == 0)
            {
              flagBtnDisminuir = 1;
              numeroVariable--;
              if (numeroVariable < 1)
              {
                numeroVariable = 4;
              }
              obtenerValorVariable();
              pintarVentanaEdicionMenu();
              // pintarConsolaSerial();
              asignarBlinkLCD();
            }
          }
          else
          {
            flagBtnDisminuir = 0;
          }

          if (digitalRead(btnEditar) == nivelActivo)
          {
            if (flagBtnEditar1 == 0)
            {
              flagBtnEditar1 = 1;
              nivelEdicion = 3;

              // pintarConsolaSerial();
              asignarBlinkLCD();
              // Edicion valor de variables
              while (nivelEdicion == 3)
              {
                if (editandoProgramaEjecucion)
                {
                  controladorDireccionMotor();
                  controladorTemporizador();
                }
                if (digitalRead(btnAumentar) == nivelActivo)
                {
                  if (flagBtnAumentar == 0)
                  {
                    flagBtnAumentar = 1;
                    valorVariable++;

                    if (numeroVariable == 4)
                    {
                      if (valorVariable > 3)
                      {
                        valorVariable = 1;
                      }
                    }
                    else if (numeroVariable == 1)
                    {
                      if (valorVariable > 10)
                      {
                        valorVariable = 1;
                      }
                    }
                    editarValorVariable();
                    pintarVentanaEdicionMenu();
                    // pintarConsolaSerial();
                    asignarBlinkLCD();
                  }
                }
                else
                {
                  flagBtnAumentar = 0;
                }

                if (digitalRead(btnDisminuir) == nivelActivo)
                {
                  if (flagBtnDisminuir == 0)
                  {
                    flagBtnDisminuir = 1;
                    valorVariable--;
                    if (numeroVariable == 4)
                    {
                      // valorVariable = 1;
                      if (valorVariable < 1)
                      {
                        valorVariable = 3;
                      }
                    }
                    else if (numeroVariable == 1)
                    {
                      if (valorVariable < 1)
                      {
                        valorVariable = 10;
                      }
                    }
                    else
                    {
                      if (valorVariable < 0)
                      {
                        valorVariable = 0;
                      }
                    }
                    editarValorVariable();
                    pintarVentanaEdicionMenu();
                    // pintarConsolaSerial();
                    asignarBlinkLCD();
                  }
                }
                else
                {
                  flagBtnDisminuir = 0;
                }

                if (digitalRead(btnParar) == nivelActivo)
                {
                  if (flagBtnParar1 == 0)
                  {
                    flagBtnParar1 = 1;
                    nivelEdicion = 2;
                    pintarConsolaSerial();
                    asignarBlinkLCD();
                  }
                }
                else
                {
                  flagBtnParar1 = 0;
                }
              }
            }
          }
          else
          {
            flagBtnEditar1 = 0;
          }

          if (digitalRead(btnParar) == nivelActivo)
          {
            if (flagBtnParar1 == 0)
            {
              flagBtnParar1 = 1;
              nivelEdicion = 1;
              // pintarConsolaSerial();
              asignarBlinkLCD();
            }
          }
          else
          {
            flagBtnParar1 = 0;
          }
        }
      }
    }
    else
    {
      flagBtnEditar1 = 0;
    }

    if (digitalRead(btnComenzar) == nivelActivo)
    {
      if (flagBtnComenzar1 == 0)
      {
        flagBtnComenzar1 = 1;
        fase = faseTemp;
        if (editandoProgramaEjecucion)
        {
          iniciarTemporizador();
          iniciarSensorTemperatura();
          pintarVentanaEjecucion();
        }
        else
        {
          pintarVentanaSeleccion();
        }
        guardarValoresEEPROM();
        editarValorVariable();
        nivelEdicion = 0;
        numeroVariable = 1;
        editandoProgramaEjecucion = false;
        lcd.noBlink();
      }
    }
    else
    {
      flagBtnComenzar1 = 0;
    }
  }
}