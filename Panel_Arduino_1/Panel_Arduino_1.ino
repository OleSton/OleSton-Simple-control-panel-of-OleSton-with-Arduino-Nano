// Version 2.

#include <SoftwareSerial.h>

#define PpanelID    2   // номер панели СМ в сети
#define MasterID    1   // номер контроллера СМ в сети

#define ProgNuber   8   // номер сценария стирки
#define TemperWash  30  // температура воды стирки
#define SpeedSpin   40  // скорость отжима, на контроллере СМ будет умножена на 10

#define LedPin      13  // номер пина светодиода
#define ButtPin     7   // номер пина кнопки                                 
bool flag_bt = false;   // флаг блок.дребез. кнопки
bool flag_send = false; // флаг режима отпраки данных контроллеру
uint8_t errConn = 0;    // счетчик ошибок приема данных от контроллера
uint32_t btnTimer = 0;  // переменная хранения миллисекунд
uint8_t StartWash = 0;  // переменная хранения режим Стирки (Старт или Стоп)
void ButtonRead(void);  // функция обработки состояния кнопки Старт/Стоп
SoftwareSerial softSerial(2, 3);    // Создаём объект softSerial указывая выводы RX, TX (можно указывать любые выводы Arduino UNO)
struct Str {
  uint8_t ID;         // id номер для кого сообщение
  uint8_t Start;      // Старт / Стоп стирка
  uint8_t Prog;       // Номер сценария стирки
  uint8_t Temper;     // Температура воды стирки
  uint8_t Speen;      // Обороты отжима белья
  uint8_t Status;     // Статус стирки
  uint8_t FlagEnd;    // Флаг окончания стирки
  uint8_t rez1;       // резерв
  uint8_t rez2;       // резерв
  uint8_t cTemp;      // Текущая температура воды в баке
  uint8_t sTemp;      // Заданная температура воды
  uint8_t LevWat;     // Уровень воды в баке
  uint8_t NumProg;    // Текущая исполняемая программа стирки из сценария стирки
  uint8_t FlagDoor;   // Флаг состояния люка
  uint8_t CountProg;  // Количество сценариев стирок на SD карте
  uint16_t Speed;     // Текущая скорость вращения барабана
  uint16_t Pause;     // Текущее значение паузы
  uint8_t CRC;        // резерв
};

Str buf;              // создаём структуру для приема и отправки данных от/к контроллеру СМ

void setup() {
  pinMode(ButtPin, INPUT_PULLUP); // настраиваем пин кнопки на вход с подтяжкой
  pinMode(LedPin, OUTPUT);       // настраиваем пин светодиода на выход

  softSerial.begin(115200);     // Инициируем передачу данных по программной шине UART на скорости 11520 (между Arduino и компьютером)
  softSerial.write("\nHello!\n");
  Serial.begin(9600);    // Инициируем передачу данных по аппаратной  шине UART на скорости 9600 (между Arduino и модулем) SERIAL_8E1

}

void loop() {
  ButtonRead();             // читаем состояние кнопки Старт/Стоп
  sendData();
  checkErr();
  if (buf.FlagEnd == 1)    // Если от контроллера (мастера) выставлен флаг конец стирки
  {
    StartWash   = 0;                        // Отключаем стирку на панели
    //buf.FlagEnd = 0;                      // Очищаем флаг конца стирки
    buf.Start   = 0;
  }
}
void ButtonRead()                             // функция чтения состояния кнопки с подавлением дребезга
{
  bool btnState = !digitalRead(ButtPin);      // читаем инвертированное значение
  if (btnState && !flag_bt && millis() - btnTimer > 100)
  {
    StartWash ^= 1;                           // инвертируем переменную при каждом нажатии кнопки
    digitalWrite(LedPin, StartWash);          // отображаем состояние Старт/Стоп светодиодом
    flag_bt = true;                           // переключаем флаг для запрета повторного входа в функцию
    btnTimer = millis();                      // запоминаем значение миллисекунд
    softSerial.print("\nStartWash: ");
    softSerial.println(StartWash);           // для отладки выводим состояние переменной
  }
  if (!btnState && flag_bt && millis() - btnTimer > 100)
  {
    flag_bt = false;                         // по истечении времени опускаем флаг
    btnTimer = millis();                     // запоминаем значение миллисекунд
  }
}

void serialEvent()                           // функция получении символа в UART
{
  if (Serial.available() > (byte)sizeof(buf)) // Если пришло сообщение размером с нашу структуру
  {
    byte tmp[(byte)sizeof(buf)] = {0,};
    Serial.readBytes((byte*)&tmp, (byte)sizeof(buf));
    byte crc = 0;
    for (byte i = 0; i < 20 - 1; i++) 
    {
      crc ^= tmp[i];
    }
    crc ^= 20 + 2;

   // softSerial.print("\ncrc "); softSerial.print(crc); //softSerial.print(" len ");softSerial.print((byte)sizeof(buf));
    if (tmp[(byte)sizeof(buf) - 1] != crc || tmp[0] != PpanelID)
    {
      errConn++;
      softSerial.print("  ERR "); softSerial.print(errConn);
      return;
    }

    memcpy((byte*)&buf, tmp, sizeof(tmp));

    softSerial.print("\n");
    softSerial.print(buf.ID); softSerial.print(" ");      softSerial.print(buf.Start); softSerial.print(" ");     softSerial.print(buf.Prog); softSerial.print(" ");
    softSerial.print(buf.Temper); softSerial.print(" ");  softSerial.print(buf.Speen); softSerial.print(" ");     softSerial.print(buf.Status); softSerial.print(" ");
    softSerial.print(buf.FlagEnd); softSerial.print(" "); softSerial.print(buf.rez1); softSerial.print(" ");      softSerial.print(buf.rez2); softSerial.print(" ");
    softSerial.print(buf.cTemp); softSerial.print(" ");   softSerial.print(buf.sTemp); softSerial.print(" ");     softSerial.print(buf.LevWat); softSerial.print(" ");
    softSerial.print(buf.NumProg); softSerial.print(" "); softSerial.print(buf.FlagDoor); softSerial.print(" ");  softSerial.print(buf.CountProg); softSerial.print(" ");
    softSerial.print(buf.Speed); softSerial.print(" ");   softSerial.print(buf.Pause); softSerial.print(" ");     softSerial.println(buf.CRC); softSerial.print("\n");

    if (buf.FlagEnd == 1)                   // Если от контроллера (мастера) выставлен флаг конец стирки
    {
      StartWash   = 0;                      // Отключаем стирку на панели
      buf.FlagEnd = 0;                      // Очищаем флаг конца стирки
    }
    flag_send = true;                       // Поднимаем флаг отправки данных
    delay(50);                              // Задержка перед отправкой данных
  }

}
void sendData()
{
  if (flag_send == false) return;           // Если флаг не поднят выходим из функции отправки данных
  
  byte tmp_buf[20] = {0,};
  tmp_buf[0] = MasterID;
  tmp_buf[1] = StartWash;
  tmp_buf[2] = 10;
  tmp_buf[3] = 40;
  tmp_buf[4] = 50;
  for (byte i = 0; i < 20 - 1; i++)       // Подсчет CRC  отправляемых данных
  tmp_buf[19] ^= tmp_buf[i];
  buf.CRC = tmp_buf[19] ^ 20 + 2;
  
  Serial.write((byte*)&tmp_buf, sizeof(tmp_buf));// Отвечаем контроллеру (мастеру)
  flag_send = false;
}

void checkErr()
{
  if (errConn < 1) return;
  errConn = 0;
  Serial.end();
  Serial.begin(9600);
}
