#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

// Configura MAC Address del RX (puede ser broadcast)
uint8_t slaveAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Struct que da forma a los datos enviados (se requiere el mismo en tx y rx)
typedef struct payload {
  String id;
  int detecciones;
};

payload myData;  // Instancia de payload utilizada
RTC_DATA_ATTR int detecciones = 0;  

#define uS_TO_S_FACTOR 1000000ULL /* Factor de Conversion */
#define TIME_TO_SLEEP 20ULL       /* Tiempo de Sleep (segundos) */
#define SENSOR_PIN GPIO_NUM_4
#define LED_PIN 2

void setup() 
{
  Serial.begin(115200);
  delay(200);

  esp_reset_reason_t reset_reason = esp_reset_reason();

  if (reset_reason == ESP_RST_DEEPSLEEP)  // Si se reinicia al despertar de Deep Sleep
  {
    Serial.println("Reinicio desde Deep Sleep");
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0){ // Si se despierta por GPIO
      detecciones += 1;
      Serial.print("Detecciones actuales: ");
      Serial.println(detecciones);
    } 
      
    else if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) { // Si se despierta por Timer
      int err = Init_ESP_NOW();
      if (!err)
        return;
      else
        SendMessage();
    }
  } 
  else if (reset_reason == ESP_RST_POWERON) {   // Si se reinicia por evento de encendido
    detecciones = 0;
    Serial.println("Reinicio manual");
  }
  else
    Serial.println("Otro tipo de reinicio");

  //----------------- Deep Sleep ----------------------
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);  //Habilita despertar por Timer
    esp_sleep_enable_ext0_wakeup(SENSOR_PIN, HIGH);    //Habilita despertar por GPIO 33

    Serial.println("Ingresando a Deep Sleep");
    Serial.flush(); 
    esp_deep_sleep_start();
}

void loop() {
}

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
    return 0;
  }
}

int SendMessage(void)
{
  // En este caso simula valores
  myData.id = WiFi.macAddress();
  myData.detecciones = detecciones;

  // Envia el mensaje mediante ESP-NOW
  esp_err_t result = esp_now_send(slaveAddress, (uint8_t *)&myData, sizeof(myData));
  if (result == ESP_OK) {
    Serial.println("Mensaje enviado correctamente");
    pinMode(LED_PIN, OUTPUT);   //Unicamente para pruebas
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    return 1;
  } 
  else{
    Serial.println("Error al enviar el mensaje");
    return 0;
  }
}
