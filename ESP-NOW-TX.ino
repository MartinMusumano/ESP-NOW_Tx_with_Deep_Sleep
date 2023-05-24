#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <ESP32Time.h>
#include "esp_sleep.h"

// Configura MAC Address del RX (puede ser broadcast)
uint8_t slaveAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Struct que da forma a los datos enviados (se requiere el mismo en tx y rx)
typedef struct payload {
  char id[18];
  unsigned int detecciones;
};

ESP32Time rtc;
payload myData;  // Instancia de payload utilizada
RTC_DATA_ATTR unsigned int detecciones = 0;
RTC_DATA_ATTR unsigned long ultima_deteccion = 0;
RTC_DATA_ATTR unsigned long nro_mensaje = 1;

#define uS_TO_mS_FACTOR 1000ULL /* Factor de Conversion */
#define TIME_TO_SLEEP 180000ULL  /* Tiempo de Sleep (milisegundos) */
#define SENSOR_PIN GPIO_NUM_4
#define LED_PIN 2

void setup() {
  //pinMode(SENSOR_PIN, INPUT);
  uint8_t sensor_state = digitalRead(SENSOR_PIN); //lee rapidamente en caso de wake up
  Serial.begin(115200);

  esp_reset_reason_t reset_reason = esp_reset_reason();

  if (reset_reason == ESP_RST_DEEPSLEEP)  // Si se reinicia al despertar de Deep Sleep
  {
    Serial.println("Reinicio desde Deep Sleep");
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0)  // Si se despierta por GPIO
    {
      if (sensor_state == HIGH){
        esp_sleep_enable_ext0_wakeup(SENSOR_PIN, LOW);  //Invierte nivel para evitar reentradas
        if(rtc.getEpoch() - ultima_deteccion > 4UL){      // Minima cant. de segundos entre detecciones
          ultima_deteccion = rtc.getEpoch();
          detecciones += 1;
          Serial.print("DETECCIONES: ");
          Serial.println(detecciones);
        }
      }
      else
        esp_sleep_enable_ext0_wakeup(SENSOR_PIN, HIGH);
    } 
    else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) // Si se despierta por Timer
    {  
      esp_sleep_enable_ext0_wakeup(SENSOR_PIN, HIGH);      //Habilita despertar por GPIO

      if (!Init_ESP_NOW() || !SendMessage()) {
        //Hacer algo si hay error
        return;
      } 
      else
        nro_mensaje+=1;  
    }
  } 
  else if (reset_reason == ESP_RST_POWERON)   // Si se reinicia por Power On manual
  {  
    detecciones = 0;
    ultima_deteccion = 0;
    nro_mensaje = 1;
    rtc.setTime(0),
    Serial.println("Reinicio manual");
    esp_sleep_enable_ext0_wakeup(SENSOR_PIN, HIGH);  //Habilita despertar por GPIO
  } 
  else {
    Serial.print("Otro tipo de reinicio: ");
  }

  // Configura el timer para despertar y enviar mensaje
  unsigned long long new_time_to_sleep = TIME_TO_SLEEP * nro_mensaje - rtc.getEpoch()*1000ULL;
  esp_sleep_enable_timer_wakeup(new_time_to_sleep * uS_TO_mS_FACTOR);  //Habilita despertar por Timer
  Serial.println(rtc.getDateTime());

  Serial.println("Ingresando a Deep Sleep");
  Serial.flush();
  esp_wifi_stop();
  esp_deep_sleep_start();
}

void loop() {
}


//---------------- Funciones ESP-NOW---------------------

int Init_ESP_NOW(void) 
{
  WiFi.mode(WIFI_STA);  // Wi-Fi Station

  if (esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR) != ESP_OK)  // Configura Protocolo LR
    Serial.println("Error al configurar LR PROTOCOL");

  if (esp_now_init() != ESP_OK)  // Inicia ESP-NOW
  {
    Serial.println("Error iniciando ESP-NOW");
    return 0;
  }

  // Registra al slave (RX)
  esp_now_peer_info_t slaveInfo = {};
  memcpy(slaveInfo.peer_addr, slaveAddress, 6);
  slaveInfo.channel = 0;
  slaveInfo.encrypt = false;

  // Añade slave
  if (esp_now_add_peer(&slaveInfo) != ESP_OK) {
    Serial.println("Error registrando al slave");
  }

  return 1;
}

int SendMessage(void) 
{
  // Actualiza valores
  WiFi.macAddress().toCharArray(myData.id, sizeof(myData.id));
  myData.detecciones = detecciones;

  // Envia el mensaje mediante ESP-NOW
  esp_err_t result = esp_now_send(slaveAddress, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Mensaje enviado:");
    Serial.println(myData.id);
    // pinMode(LED_PIN, OUTPUT);  //Unicamente para pruebas
    // digitalWrite(LED_PIN, HIGH);
    // delay(500);
    return 1;
  } 
  else {
    Serial.println("Error al enviar el mensaje");
    return 0;
  }
}
