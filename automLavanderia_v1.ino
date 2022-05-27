#include <AsyncTaskLib.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Iniciamos el lcd
const int rs = 31, en = 33, d4 = 23, d5 = 25, d6 = 27, d7 = 29;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// LiquidCrystal_I2C lcd(0x20, 16, 2);
// LiquidCrystal_I2C lcd(0x27, 16, 2);

// Entradas
#define btnComenzar 35
#define btnEditar 37
#define btnDisminuir 39
#define btnAumentar 41
#define btnParar 43
#define sensorTemp A6
#define sensorPresion A0

// Salidas
#define MotorDirA 7
#define MotorDirB 6
#define MagnetPuerta 2
#define ValvulAgua 5
#define ElectrovVapor 4
#define ValvulOnOff 3
#define LedConfirmacion 0

// Definimos variables de los programas
uint8_t NivelAgua[3][4] = {{6, 6, 7, 4}, {5, 4, 4, 5}, {1, 3, 3, 2}};
uint8_t RotacionTam[3][4] = {{1, 1, 2, 2}, {2, 3, 3, 2}, {3, 2, 3, 2}};
uint8_t TemperaturaLim[3][4] = {{30, 0, 35, 45}, {45, 0, 40, 45}, {55, 0, 34, 30}};
uint8_t TemporizadorLim[3][4] = {{1, 2, 1, 1}, {1, 2, 3, 1}, {1, 2, 2, 2}};
uint8_t TiempoEntFase[3][4] = {{5, 4, 4, 5}, {3, 4, 4, 3}, {3, 4, 4, 3}};
uint8_t TiempoRotacion[3][2] = {{3, 2}, {3, 3}, {4, 3}};

// uint8_t NivelAgua[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
// uint8_t RotacionTam[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
// uint8_t TemperaturaLim[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
// uint8_t TemporizadorLim[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
// uint8_t TiempoEntFase[3][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
// uint8_t TiempoRotacion[3][2] = {{0, 0}, {0, 0}, {0, 0}};

// uint8_t NivelAgua[3][4];
// uint8_t RotacionTam[3][4];
// uint8_t TemperaturaLim[3][4];
// uint8_t TemporizadorLim[3][4];
// uint8_t TiempoRotacion[3][2];
// uint8_t TiempoEntFase[3][4];

// Variables bandera
boolean tiempoCumplido = false;
boolean programaTerminado = false;
boolean editandoProgramaEjecucion = false;
// boolean terminarEdicion = false;
boolean nivelActivo = LOW;

// Antirebote
uint8_t flagBtnAumentar1 = 0;
uint8_t flagBtnDisminuir1 = 0;
uint8_t flagBtnComenzar1 = 0;
uint8_t flagBtnEditar1 = 0;
uint8_t flagBtnEditar2 = 0;
uint8_t flagBtnEditar3 = 0;
uint8_t flagBtnParar1 = 0;

// Variables de proceso
boolean flagLCD = false;
uint8_t programa = 1;
uint8_t numeroVariable = 1;
uint8_t valorVariable = 0;
uint8_t fase = 1;
uint8_t nivelEdicion = 0;
uint8_t direccion = 1;
uint8_t PreDireccion = 0;

int8_t minutos[2] = {0, 0};
int8_t segundos[2] = {0, 0};
int16_t segunderoTemporizador = 0;
uint8_t valorTemperatura = 40;
uint8_t valorPresion = 4;

AsyncTask segundosMotor(1000, true, []()
                        { segundos[0]++; });

void pintarVariables()
{
  lcd.setCursor(10, 0);
  lcd.print(valorPresion);

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
  if (!editandoProgramaEjecucion){ pintarVariables(); } });

// AsyncTask delayTemporizador(100, true, []()
//                             { flagLCD = !flagLCD; });

void setup()
{
  // Definimos entradas
  pinMode(btnAumentar, INPUT);
  pinMode(btnComenzar, INPUT);
  pinMode(btnDisminuir, INPUT);
  pinMode(btnEditar, INPUT);
  pinMode(btnParar, INPUT);
  pinMode(sensorTemp, INPUT);
  pinMode(sensorPresion, INPUT);

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
  // lcd.blink();

  // Inicializamos el puerto serial para depurar
  Serial.begin(9600);
  Serial.println("Puerto serial iniciado");

  // Recuperamos valores del EEPROM
  // guardarValoresEEPROM();
  // delay(100);
  recuperarValoresEEPROM();
  // delay(20);
  // Serial.print("TemperaturaLim[2][1]: ");
  // Serial.println(TemperaturaLim[2][1]);
  // recuperarValoresEEPROM();
  // eeprom_read();

  // inicializamos subprocesos
  pintarVentanaSeleccion();
}

void loop()
{
  // Eligiendo programa
  if (digitalRead(btnAumentar) == nivelActivo)
  {
    if (flagBtnAumentar1 == 0)
    {
      flagBtnAumentar1 = 1;
      programa++;
      if (programa > 3)
        programa = 1;
      pintarVentanaSeleccion();
    }
  }
  else
  {
    flagBtnAumentar1 = 0;
  }

  if (digitalRead(btnDisminuir) == nivelActivo)
  {
    if (flagBtnDisminuir1 == 0)
    {
      flagBtnDisminuir1 = 1;
      Serial.println("btnDisminuir");
      programa--;
      if (programa < 1)
      {
        programa = 3;
      }
      pintarVentanaSeleccion();
      // eeprom_write();
      // delay(100);
    }
  }
  else
  {
    flagBtnDisminuir1 = 0;
  }

  // Comenzando programa seleccionado
  if (digitalRead(btnComenzar) == nivelActivo)
  {
    if (flagBtnComenzar1 == 0)
    {
      flagBtnComenzar1 = 1;
      // Serial.println("btnComenzar - Iniciar programa");
      iniciarPrograma();
      // iniciarTemporizador();
      while (!tiempoCumplido)
      {
        // revisarTemporizador();
        controladorTemporizador();
        controladorDireccionMotor();
        if (digitalRead(btnParar) == nivelActivo)
        {
          if (flagBtnParar1 == 0)
          {
            flagBtnParar1 = 1;
            // Serial.println("btnParar - Para programa");
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
            // Edicion de numero de variable
            flagBtnEditar1 = 1;
            numeroVariable = 1;
            nivelEdicion = 1;
            editandoProgramaEjecucion = true;
            obtenerValorVariable();
            pintarVentanaEdicionMenu();
            lcd.blink();
            // pintarConsolaSerial();
            asignarBlinkLCD();
            while (nivelEdicion == 1)
            {
              controladorTemporizador();
              controladorDireccionMotor();
              if (digitalRead(btnAumentar) == nivelActivo)
              {
                if (flagBtnAumentar1 == 0)
                {
                  flagBtnAumentar1 = 1;
                  fase++;
                  if (fase > 4)
                  {
                    fase = 1;
                  }

                  pintarVentanaEdicionMenu();
                  pintarConsolaSerial();
                  // editarValorVariable();

                  // eeprom_write();
                  // delay(100);
                }
              }
              else
              {
                flagBtnAumentar1 = 0;
              }

              if (digitalRead(btnDisminuir) == nivelActivo)
              {
                if (flagBtnDisminuir1 == 0)
                {
                  flagBtnDisminuir1 = 1;
                  fase--;
                  if (fase < 1)
                  {
                    fase = 4;
                  }
                  pintarVentanaEdicionMenu();
                  pintarConsolaSerial();
                  // editarValorVariable();
                  // eeprom_write();
                  // delay(100);
                }
              }
              else
              {
                flagBtnDisminuir1 = 0;
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
                    controladorTemporizador();
                    controladorDireccionMotor();
                    if (digitalRead(btnAumentar) == nivelActivo)
                    {
                      if (flagBtnAumentar1 == 0)
                      {
                        flagBtnAumentar1 = 1;
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
                      flagBtnAumentar1 = 0;
                    }

                    if (digitalRead(btnDisminuir) == nivelActivo)
                    {
                      if (flagBtnDisminuir1 == 0)
                      {
                        flagBtnDisminuir1 = 1;
                        numeroVariable--;
                        if (numeroVariable < 1)
                        {
                          numeroVariable = 4;
                        }
                        obtenerValorVariable();
                        pintarVentanaEdicionMenu();
                        // pintarConsolaSerial();
                        asignarBlinkLCD();
                        // eeprom_write();
                        // delay(100);
                      }
                    }
                    else
                    {
                      flagBtnDisminuir1 = 0;
                    }

                    if (digitalRead(btnEditar) == nivelActivo)
                    {
                      if (flagBtnEditar1 == 0)
                      {
                        flagBtnEditar1 = 1;
                        nivelEdicion = 3;

                        // pintarConsolaSerial();
                        asignarBlinkLCD();
                        // Editar valor de fase
                        while (nivelEdicion == 3)
                        {
                          controladorTemporizador();
                          controladorDireccionMotor();
                          if (digitalRead(btnAumentar) == nivelActivo)
                          {
                            if (flagBtnAumentar1 == 0)
                            {
                              flagBtnAumentar1 = 1;
                              valorVariable++;

                              if (numeroVariable == 4)
                              {
                                // valorVariable = 1;
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
                              // eeprom_write();
                              // delay(100);
                            }
                          }
                          else
                          {
                            flagBtnAumentar1 = 0;
                          }

                          if (digitalRead(btnDisminuir) == nivelActivo)
                          {
                            if (flagBtnDisminuir1 == 0)
                            {
                              flagBtnDisminuir1 = 1;
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
                              // eeprom_write();
                              // delay(100);
                            }
                          }
                          else
                          {
                            flagBtnDisminuir1 = 0;
                          }

                          if (digitalRead(btnParar) == nivelActivo)
                          {
                            if (flagBtnParar1 == 0)
                            {
                              flagBtnParar1 = 1;
                              nivelEdicion = 2;
                              pintarConsolaSerial();
                              asignarBlinkLCD();
                              // fase = 0;
                              // eeprom_write();
                              // delay(100);
                            }
                          }
                          else
                          {
                            flagBtnParar1 = 0;
                          }
                        }

                        // eeprom_write();
                        // delay(100);
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
                        // fase = 0;
                        // eeprom_write();
                        // delay(100);
                      }
                    }
                    else
                    {
                      flagBtnParar1 = 0;
                    }
                  }

                  // eeprom_write();
                  // delay(100);
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
                  nivelEdicion = 0;
                  numeroVariable = 1;
                  editandoProgramaEjecucion = false;
                  lcd.noBlink();
                  pintarVentanaEjecucion();
                  // escribirVariableEEPROM(0, programa - 1);
                  // fase = 1;
                  // eeprom_write();
                  // delay(100);
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
      numeroVariable = 1;
      nivelEdicion = 1;
      editandoProgramaEjecucion = false;
      obtenerValorVariable();
      pintarVentanaEdicionMenu();
      lcd.blink();
      // pintarConsolaSerial();
      asignarBlinkLCD();
      while (nivelEdicion == 1)
      {
        if (digitalRead(btnAumentar) == nivelActivo)
        {
          if (flagBtnAumentar1 == 0)
          {
            flagBtnAumentar1 = 1;
            fase++;
            if (fase > 4)
            {
              fase = 1;
            }

            pintarVentanaEdicionMenu();
            pintarConsolaSerial();
            // editarValorVariable();

            // eeprom_write();
            // delay(100);
          }
        }
        else
        {
          flagBtnAumentar1 = 0;
        }

        if (digitalRead(btnDisminuir) == nivelActivo)
        {
          if (flagBtnDisminuir1 == 0)
          {
            flagBtnDisminuir1 = 1;
            fase--;
            if (fase < 1)
            {
              fase = 4;
            }
            pintarVentanaEdicionMenu();
            pintarConsolaSerial();
            // editarValorVariable();
            // eeprom_write();
            // delay(100);
          }
        }
        else
        {
          flagBtnDisminuir1 = 0;
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

              if (digitalRead(btnAumentar) == nivelActivo)
              {
                if (flagBtnAumentar1 == 0)
                {
                  flagBtnAumentar1 = 1;
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
                flagBtnAumentar1 = 0;
              }

              if (digitalRead(btnDisminuir) == nivelActivo)
              {
                if (flagBtnDisminuir1 == 0)
                {
                  flagBtnDisminuir1 = 1;
                  numeroVariable--;
                  if (numeroVariable < 1)
                  {
                    numeroVariable = 4;
                  }
                  obtenerValorVariable();
                  pintarVentanaEdicionMenu();
                  // pintarConsolaSerial();
                  asignarBlinkLCD();
                  // eeprom_write();
                  // delay(100);
                }
              }
              else
              {
                flagBtnDisminuir1 = 0;
              }

              if (digitalRead(btnEditar) == nivelActivo)
              {
                if (flagBtnEditar1 == 0)
                {
                  flagBtnEditar1 = 1;
                  nivelEdicion = 3;

                  // pintarConsolaSerial();
                  asignarBlinkLCD();
                  // Editar valor de fase
                  while (nivelEdicion == 3)
                  {

                    if (digitalRead(btnAumentar) == nivelActivo)
                    {
                      if (flagBtnAumentar1 == 0)
                      {
                        flagBtnAumentar1 = 1;
                        valorVariable++;

                        if (numeroVariable == 4)
                        {
                          // valorVariable = 1;
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
                        // eeprom_write();
                        // delay(100);
                      }
                    }
                    else
                    {
                      flagBtnAumentar1 = 0;
                    }

                    if (digitalRead(btnDisminuir) == nivelActivo)
                    {
                      if (flagBtnDisminuir1 == 0)
                      {
                        flagBtnDisminuir1 = 1;
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
                        // eeprom_write();
                        // delay(100);
                      }
                    }
                    else
                    {
                      flagBtnDisminuir1 = 0;
                    }

                    if (digitalRead(btnParar) == nivelActivo)
                    {
                      if (flagBtnParar1 == 0)
                      {
                        flagBtnParar1 = 1;
                        nivelEdicion = 2;
                        pintarConsolaSerial();
                        asignarBlinkLCD();
                        // fase = 0;
                        // eeprom_write();
                        // delay(100);
                      }
                    }
                    else
                    {
                      flagBtnParar1 = 0;
                    }
                  }

                  // eeprom_write();
                  // delay(100);
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
                  // fase = 0;
                  // eeprom_write();
                  // delay(100);
                }
              }
              else
              {
                flagBtnParar1 = 0;
              }
            }

            // eeprom_write();
            // delay(100);
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
            pintarVentanaSeleccion();
            // escribirVariableEEPROM(0, programa - 1);
            nivelEdicion = 0;
            numeroVariable = 1;
            lcd.noBlink();
            // fase = 1;
            // eeprom_write();
            // delay(100);
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
  for (size_t i = 0; i < 4; i++)
  {
    lcd.setCursor(i + 5, 0);
    lcd.print(NivelAgua[programa - 1][i]);
  }

  lcd.setCursor(10, 0);
  lcd.print("T");
  for (size_t i = 0; i < 4; i++)
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
  for (size_t i = 0; i < 4; i++)
  {
    lcd.setCursor(3 * i, 1);
    lcd.print(TemperaturaLim[programa - 1][i]);
  }

  lcd.setCursor(12, 1);
  lcd.print("R");
  for (size_t i = 0; i < 3; i++)
  {
    lcd.setCursor(i + 13, 1);
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
  lcd.print(valorPresion);

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
  segunderoTemporizador = TemporizadorLim[programa - 1][fase - 1] * 60;
  Serial.println(segunderoTemporizador);
  segundosTemporizador.Start();
}

void controladorTemporizador()
{
  segundosTemporizador.Update();

  if (segunderoTemporizador == 0)
  {
    segundosTemporizador.Stop();
    fase++;
    pintarVentanaEjecucion();
    iniciarTemporizador();

    if (fase >= 4)
    {
      terminarPrograma();
    }
  }
}

// Subprocesos del motor
void controladorDireccionMotor()
{
  segundosMotor.Update();
  switch (direccion)
  {
  case 1:
    if (segundos[0] == TiempoRotacion[RotacionTam[programa - 1][fase - 1] - 1][0])
    {
      direccion = 2;
      PreDireccion = 1;
      // segundosMotor.Stop();
      // segundosMotor.Start();
      segundos[0] = 0;
    }
    digitalWrite(MotorDirA, HIGH);
    digitalWrite(MotorDirB, LOW);
    break;

  case 2:
    if (segundos[0] == TiempoRotacion[RotacionTam[programa - 1][fase - 1] - 1][1])
    {
      if (PreDireccion == 1)
      {
        direccion = 3;
      }
      else
      {
        direccion = 1;
      }
      // segundosMotor.Stop();
      // segundosMotor.Start();
      segundos[0] = 0;
    }
    digitalWrite(MotorDirA, LOW);
    digitalWrite(MotorDirB, LOW);
    break;

  case 3:
    if (segundos[0] == TiempoRotacion[RotacionTam[programa - 1][fase - 1] - 1][0])
    {
      direccion = 2;
      PreDireccion = 2;
      // segundosMotor.Stop();
      // segundosMotor.Start();
      segundos[0] = 0;
    }
    digitalWrite(MotorDirA, LOW);
    digitalWrite(MotorDirB, HIGH);
    break;

  default:
    break;
  }
}

// Subprocesos de manejo de programas
void iniciarPrograma()
{
  tiempoCumplido = false;
  numeroVariable = 1;
  fase = 1;

  digitalWrite(MagnetPuerta, HIGH);
  digitalWrite(ValvulAgua, HIGH);
  digitalWrite(ElectrovVapor, HIGH);
  digitalWrite(ValvulOnOff, HIGH);

  // reiniciamos temporizadores
  segundos[0] = 0;
  segundos[1] = 0;

  // desactivamos funciones asincronicas
  segundosMotor.Start();
  iniciarTemporizador();

  pintarVentanaEjecucion();
  // delayTemporizador.Start();
  direccion = 1;
}

void terminarPrograma()
{
  tiempoCumplido = true;
  programaTerminado = true;
  numeroVariable = 1;

  // apagamos motor
  digitalWrite(MotorDirA, LOW);
  digitalWrite(MotorDirB, LOW);

  digitalWrite(MagnetPuerta, LOW);
  digitalWrite(ValvulAgua, LOW);
  digitalWrite(ElectrovVapor, LOW);
  digitalWrite(ValvulOnOff, LOW);

  // desactivamos funciones asincronicas
  segundosMotor.Stop();
  segundosTemporizador.Stop();

  // reiniciamos los temporizadores
  minutos[1] = 0;
  segundos[1] = 0;
  minutos[0] = 0;
  segundos[0] = 0;
  segunderoTemporizador = 0;

  // mostramos valores de prueba
  // Serial.println("Tiempo motor");
  // Serial.println(segundos[0]);
  // Serial.println("Tiempo temporizador");
  // Serial.println(segundos[1]);

  // reiniciamos temporizadores
  segundos[0] = 0;
  segundos[1] = 0;

  // mostramos nueva pantalla en el LCD
  pintarVentanaSeleccion();
}

// Subprocesos de manejo del EEPROM
void escribirVariableEEPROM(uint8_t indice, uint8_t valor)
{
  EEPROM.write(indice, valor);
  // EEPROM.write(2, fase);
  // EEPROM.write(3, );
  // EEPROM.write(4, (Counter / 1000) % 10);
  // EEPROM.write(4, 7);
}

void recuperarValoresEEPROM()
{
  programa = EEPROM.read(0);
  // fase = EEPROM.read(1);
  // minutos[1] = EEPROM.read(2);
  // segundos[1] = EEPROM.read(3);
  TemperaturaLim[programa-1][0] = EEPROM.read(4);
  // for (uint8_t i = 0; i < 3; i++)
  // {
  //   for (uint8_t j = 0; i < 4; j++)
  //   {
  //     NivelAgua[i][j] = EEPROM.read(4 + (12 * 0) + (i + j));
  //     TemperaturaLim[i][j] = EEPROM.read(4 + (12 * 1) + (i + j));
  //     TemporizadorLim[i][j] = EEPROM.read(4 + (12 * 2) + (i + j));
  //     RotacionTam[i][j] = EEPROM.read(4 + (12 * 3) + (i + j));
  //     TiempoEntFase[i][j] = EEPROM.read(4 + (12 * 4) + (i + j));
  //   }
  // }
  // for (uint8_t i = 0; i < 3; i++)
  // {
  //   for (uint8_t j = 0; i < 2; j++)
  //   {
  //     TiempoRotacion[i][j] = EEPROM.read(4 + (12 * 5) + (i + j));
  //   }
  // }
}

void guardarValoresEEPROM()
{
  EEPROM.put(0, programa);
  EEPROM.put(1, fase);
  EEPROM.put(2, minutos[1]);
  EEPROM.put(3, segundos[1]);
  // EEPROM.put(4, TemperaturaLim[programa-1][0]);

  for (uint8_t i = 0; i < 3; i++)
  {
    for (uint8_t j = 0; i < 4; j++)
    {
      EEPROM.put(4 + (12 * 0) + (i + j), NivelAgua[i][j]);
      EEPROM.put(4 + (12 * 1) + (i + j), TemperaturaLim[i][j]);
      EEPROM.put(4 + (12 * 2) + (i + j), TemporizadorLim[i][j]);
      EEPROM.put(4 + (12 * 3) + (i + j), RotacionTam[i][j]);
      EEPROM.put(4 + (12 * 4) + (i + j), TiempoEntFase[i][j]);
    }
  }
  for (uint8_t i = 0; i < 3; i++)
  {
    for (uint8_t j = 0; i < 2; j++)
    {
      EEPROM.put(4 + (12 * 5) + (i + j), TiempoRotacion[i][j]);
    }
  }
  Serial.println("Guardado exitosamente");
}

void pintarConsolaSerial()
{
  Serial.println("-------------------");
  Serial.print("Nivel de edicion: ");
  switch (nivelEdicion)
  {
  case 1:
    Serial.println("Edicion de fase");
    break;

  case 2:
    Serial.println("Edicion de numero de variable");
    break;

  case 3:
    Serial.println("Edicion de valor de variable");
    break;

  default:
    break;
  }
  Serial.print("Programa: ");
  Serial.println(programa);
  Serial.print("Fase: ");
  Serial.println(fase);
  Serial.print("Numero variable: ");
  Serial.println(numeroVariable);
  Serial.print("Valor obtenido: ");
  Serial.println(valorVariable);
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