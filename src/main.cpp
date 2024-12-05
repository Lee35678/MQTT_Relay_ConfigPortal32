#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ConfigPortal32.h>

char* ssid_pfix = (char*)"IoT_LeeDongHyeok";
char mqttServer[100];
const int mqttPort = 1883;
#define RELAY 2

unsigned long interval = 10000;
unsigned long lastPublished = -interval;

WiFiClient wifiClient;
PubSubClient client(wifiClient);

String user_config_html = ""
    "<p><input type='text' name='broker' placeholder='MQTT Server' length=40></p>";

// 임계값 설정
const float TEMP_THRESHOLD = 30.0; // 온도 임계값
const float HUMI_THRESHOLD = 70.0; // 습도 임계값

void msgCB(char* topic, byte* payload, unsigned int length) {
    char msgBuffer[100];
    int i;

    // MQTT 메시지를 문자열로 변환
    for(i = 0; i < (int)length; i++) {
        msgBuffer[i] = payload[i];
    }
    msgBuffer[i] = '\0';

    Serial.printf("Received [%s]: %s\n", topic, msgBuffer);

    // 수신된 토픽이 센서 데이터 토픽인지 확인
    if (strcmp(topic, "id/LeeDH/sensor/data") == 0) {
        float receivedTemp, receivedHumi;
        
        // JSON 형식 데이터 파싱
        sscanf(msgBuffer, "{\"temperature\":%f,\"humidity\":%f}", &receivedTemp, &receivedHumi);
        Serial.printf("Parsed Data -> Temp: %.1f, Humi: %.1f\n", receivedTemp, receivedHumi);

        // 온도와 습도 값에 따라 릴레이 제어
        if (receivedTemp > TEMP_THRESHOLD || receivedHumi > HUMI_THRESHOLD) {
            digitalWrite(RELAY, HIGH); // 릴레이 ON
            Serial.println("Relay ON");
        } else {
            digitalWrite(RELAY, LOW); // 릴레이 OFF
            Serial.println("Relay OFF");
        }
        
        pubStatus(); // 상태 전송
    }
}
void pubStatus();

void setup() {
    Serial.begin(115200);
    pinMode(RELAY, OUTPUT);
    
    loadConfig();
    
    if(!cfg.containsKey("config") || strcmp((const char*)cfg["config"], "done")) {
        configDevice();
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);  // w_pw 사용
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.printf("\nIP address : ");
    Serial.println(WiFi.localIP());
    
    if (cfg.containsKey("broker")) {
        sprintf(mqttServer, "%s", (const char*)cfg["broker"]);
    }
    
    client.setServer(mqttServer, mqttPort);
    client.setCallback(msgCB);
    
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        String clientId = "ESP32Relay-" + String(random(0xffff), HEX);
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
        } else {
            Serial.print("failed with state ");
            Serial.println(client.state());
            delay(2000);
        }
    }
    
    client.subscribe("id/+/relay/cmd");
    digitalWrite(RELAY, LOW);
}

void loop() {
    client.loop();
    unsigned long currentMillis = millis();
    if(currentMillis - lastPublished >= interval) {
        lastPublished = currentMillis;
        pubStatus();
    }
}

void pubStatus() {
    char buf[10];
    if (digitalRead(RELAY) == HIGH) {
        sprintf(buf, "on");
    } else {
        sprintf(buf, "off");
    }
    client.publish("id/relay/evt", buf);
}

void msgCB(char* topic, byte* payload, unsigned int length) {
    char msgBuffer[20];
    int i;
    for(i = 0; i < (int)length; i++) {
        msgBuffer[i] = payload[i];
    }
    msgBuffer[i] = '\0';
    
    Serial.printf("\n%s -> %s", topic, msgBuffer);
    
    if(!strcmp(msgBuffer, "on")) {
        digitalWrite(RELAY, HIGH);
    } else if(!strcmp(msgBuffer, "off")) {
        digitalWrite(RELAY, LOW);
    }
    
    pubStatus();
}