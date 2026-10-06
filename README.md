# MQTT_Relay_ConfigPortal32

MQTT로 온습도 데이터를 구독해 온도 30℃ 또는 습도 70%를 넘으면 릴레이를 켜고, 릴레이 상태를 주기적으로 발행하는 ESP32 펌웨어.

## 개요

WiFi 정보와 MQTT 브로커 주소는 `ConfigPortal32` 설정 포털로 입력받는다. 브로커에 연결한 뒤 센서 노드(`MQTT_Sensor`)가 발행하는 `id/LeeDH/sensor/data` JSON을 구독하고, 임계값과 비교해 릴레이를 켜거나 끈다. 릴레이 상태(`on`/`off`)는 10초마다, 그리고 센서 메시지를 받을 때마다 발행한다.

## 하드웨어

- 보드: ESP32 DOIT DevKit V1 (`esp32doit-devkit-v1`)
- 액추에이터: 릴레이 모듈

| 장치 | 신호 | ESP32 핀 |
|------|------|----------|
| 릴레이 | 제어 입력 | GPIO 2 (`RELAY`) |

## 동작 방식

### 부팅 및 설정

1. `loadConfig()`로 저장된 설정을 읽는다
2. `config` 항목이 없거나 값이 `done`이 아니면 `configDevice()`로 설정 포털 실행
   - 포털 AP 이름 접두어: `ssid_pfix`
   - 추가 입력란(`user_config_html`): MQTT 서버 주소(`broker`)
3. 저장된 `ssid`, `w_pw`로 WiFi 접속
4. `broker` 값으로 MQTT 서버(포트 1883)에 접속. 클라이언트 ID는 `ESP32Relay-<랜덤 16진수>`, 실패 시 2초 후 재시도
5. 토픽 구독 후 릴레이를 `LOW`(꺼짐)로 초기화

### MQTT 토픽

| 방향 | 토픽 | 내용 |
|------|------|------|
| 구독 | `id/LeeDH/sensor/data` | `{"temperature":<온도>,"humidity":<습도>}` |
| 구독 | `id/+/relay/cmd` | 구독만 하며, 콜백에서 처리하는 코드는 없음 |
| 발행 | `id/relay/evt` | 릴레이 상태 `on` 또는 `off` |

### 릴레이 제어 로직 (`msgCB`)

- `id/LeeDH/sensor/data` 메시지를 `sscanf`로 파싱
- `온도 > TEMP_THRESHOLD(30.0)` 또는 `습도 > HUMI_THRESHOLD(70.0)`이면 릴레이 `HIGH`, 아니면 `LOW`
- 판정 직후 `pubStatus()`로 상태 발행
- 별도로 `loop()`에서 `interval` = 10000 ms마다 상태 발행

## 개발 환경

- PlatformIO, platform `espressif32`, framework `arduino`
- build_flags: `-D MQTT_MAX_PACKET_SIZE=256`
- lib_deps
  - `knolleary/PubSubClient@^2.8`
  - `yhur/ConfigPortal32`
- 업로드 속도 460800, 시리얼 모니터 속도 115200

## 설정

- WiFi·브로커 주소 : 코드 수정 없이 첫 부팅 시 설정 포털에서 입력
- `ssid_pfix` : 설정 포털 AP 이름 접두어
- `TEMP_THRESHOLD`, `HUMI_THRESHOLD` : 릴레이를 켜는 온도·습도 기준
- `interval` : 상태 발행 주기
- `RELAY` : 릴레이 제어 핀
- `platformio.ini`의 `upload_port`, `monitor_port` : 자신의 PC에서 보드가 잡힌 COM 포트로 변경

## 빌드 및 실행

```bash
pio run -t upload
pio device monitor -b 115200
```

## 폴더 구조

```
MQTT_Relay_ConfigPortal32/
├── platformio.ini   # 보드·포트·MQTT 패킷 크기·라이브러리 설정
└── src/
    └── main.cpp     # 설정 포털 + MQTT 구독/발행 + 임계값 릴레이 제어
```
