#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define wifi_ssid "WIFI_NAME"
#define wifi_password "WIFI_PASSWORD"

#define mqtt_server "MQTT_SERVER_IP"
#define mqtt_user "MQTT_USER_NAME"
#define mqtt_password "MQTT_PASSWORD"

class ReedSensor {
  public:
    int pin;
    int last_status;
    unsigned long last_switch_time;
    String mqtt_topic;
};

ReedSensor *front;
ReedSensor *back;
ReedSensor *deck;

#define FRONT_DOOR_STATUS_TOPIC "NodeMCU_Door/GPIO14/Status"
#define FRONT_DOOR_STATUS_PIN 14           //Black Wire
unsigned long FRONT_DOOR_LAST_SWITCH_TIME = 0;
int FRONT_DOOR_LAST_STATUS_VALUE = 2;


#define BACK_DOOR_STATUS_TOPIC "NodeMCU_Door/GPIO12/Status"
#define BACK_DOOR_STATUS_PIN 12             //White Wire
unsigned long BACK_DOOR_LAST_SWITCH_TIME = 0;
int BACK_DOOR_LAST_STATUS_VALUE = 2;

#define DECK_DOOR_STATUS_TOPIC "NodeMCU_Door/GPIO13/Status"
#define DECK_DOOR_STATUS_PIN 13             // Gray Wire
unsigned long DECK_DOOR_LAST_SWITCH_TIME = 0;
int DECK_DOOR_LAST_STATUS_VALUE = 2;


void init_reed_sensor(ReedSensor *senor, int pin, String mqtt_topic) {
senor->pin = pin;
senor->last_status = 2;
senor->last_switch_time = 0;
senor->mqtt_topic = mqtt_topic;
}

void setup_reed_switches() {
front = new ReedSensor();
init_reed_sensor(front, FRONT_DOOR_STATUS_PIN, FRONT_DOOR_STATUS_TOPIC);
back = new ReedSensor();
init_reed_sensor(back, BACK_DOOR_STATUS_PIN, BACK_DOOR_STATUS_TOPIC);
deck = new ReedSensor();
init_reed_sensor(deck, DECK_DOOR_STATUS_PIN, DECK_DOOR_STATUS_TOPIC);

//Doors
pinMode(FRONT_DOOR_STATUS_PIN, INPUT_PULLUP);
pinMode(BACK_DOOR_STATUS_PIN, INPUT_PULLUP);
pinMode(DECK_DOOR_STATUS_PIN, INPUT_PULLUP);

digitalWrite(FRONT_DOOR_STATUS_PIN, LOW);
digitalWrite(BACK_DOOR_STATUS_PIN, LOW);
digitalWrite(DECK_DOOR_STATUS_PIN, LOW);

}

WiFiClient espClient;
PubSubClient client(espClient);

int debounceTime = 100;

void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("NodeMCU_Door", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("NodeMCU_Door/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting Security System NodeMCU Door");  
  
  //Setup WiFi
  setup_wifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  //Setup Reed Switches
  setup_reed_switches();
}

// the loop function runs over and over again forever
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  checkStatusForReedSensors();
}

void checkStatusForReedSensors() {
    checkStatusFrontDoor();
    checkStatusBackDoor();
    checkStatusDeckDoor();
  }

//Front Door
void checkStatusFrontDoor() {
  int currentStatusValue = digitalRead(FRONT_DOOR_STATUS_PIN);
  int lastStatusValue = lastStatusValueForSensor(FRONT_DOOR_STATUS_PIN);
  unsigned long lastSwitchTime = lastSwitchTimeForSensor(FRONT_DOOR_STATUS_PIN);
  if (currentStatusValue != lastStatusValue) {
    unsigned int currentTime = millis();
    if (currentTime - lastSwitchTime >= debounceTime) {
      publishStatus(FRONT_DOOR_STATUS_PIN);
      setLastStatusValueForSensor(FRONT_DOOR_STATUS_PIN, currentStatusValue);
      setLastSwitchTimeForSensor(FRONT_DOOR_STATUS_PIN, currentTime);
    }
  }
}

// Back Door
void checkStatusBackDoor() {
  int currentStatusValue = digitalRead(BACK_DOOR_STATUS_PIN);
  int lastStatusValue = lastStatusValueForSensor(BACK_DOOR_STATUS_PIN);
  unsigned long lastSwitchTime = lastSwitchTimeForSensor(BACK_DOOR_STATUS_PIN);
  if (currentStatusValue != lastStatusValue) {
    unsigned int currentTime = millis();
    if (currentTime - lastSwitchTime >= debounceTime) {
      publishStatus(BACK_DOOR_STATUS_PIN);
      setLastStatusValueForSensor(BACK_DOOR_STATUS_PIN, currentStatusValue);
      setLastSwitchTimeForSensor(BACK_DOOR_STATUS_PIN, currentTime);
    }
  }
}

// Deck Door
void checkStatusDeckDoor() {
  int currentStatusValue = digitalRead(DECK_DOOR_STATUS_PIN);
  int lastStatusValue = lastStatusValueForSensor(DECK_DOOR_STATUS_PIN);
  unsigned long lastSwitchTime = lastSwitchTimeForSensor(DECK_DOOR_STATUS_PIN);
  if (currentStatusValue != lastStatusValue) {
    unsigned int currentTime = millis();
    if (currentTime - lastSwitchTime >= debounceTime) {
      publishStatus(DECK_DOOR_STATUS_PIN);
      setLastStatusValueForSensor(DECK_DOOR_STATUS_PIN, currentStatusValue);
      setLastSwitchTimeForSensor(DECK_DOOR_STATUS_PIN, currentTime);
    }
  }
}

void publishStatus(int SENSOR_PIN) {
    String topic = statusTopicForSensor(SENSOR_PIN);
    const char *stateTopic = topic.c_str();
    if(digitalRead(SENSOR_PIN) == LOW) {
      client.publish(stateTopic, "closed", true);
    } else {
      client.publish(stateTopic, "open", true);
    }
}

int lastStatusValueForSensor(int SENSOR_PIN) {
    switch(SENSOR_PIN){
      case FRONT_DOOR_STATUS_PIN: return FRONT_DOOR_LAST_STATUS_VALUE;
      case BACK_DOOR_STATUS_PIN: return BACK_DOOR_LAST_STATUS_VALUE;
      case DECK_DOOR_STATUS_PIN: return DECK_DOOR_LAST_STATUS_VALUE;
      default : return 2;
    }
    return 2;
  }

unsigned long lastSwitchTimeForSensor(int SENSOR_PIN) {
    switch(SENSOR_PIN){
      case FRONT_DOOR_STATUS_PIN: return FRONT_DOOR_LAST_SWITCH_TIME;
      case BACK_DOOR_STATUS_PIN: return BACK_DOOR_LAST_SWITCH_TIME;
      case DECK_DOOR_STATUS_PIN: return DECK_DOOR_LAST_SWITCH_TIME;
      default : return 0;
    }
    return 0;
  }

void setLastStatusValueForSensor(int SENSOR_PIN, int statusValue) {
    switch(SENSOR_PIN){
      case FRONT_DOOR_STATUS_PIN: FRONT_DOOR_LAST_STATUS_VALUE = statusValue;
      case BACK_DOOR_STATUS_PIN: BACK_DOOR_LAST_STATUS_VALUE = statusValue;
      case DECK_DOOR_STATUS_PIN: DECK_DOOR_LAST_STATUS_VALUE = statusValue;
      default : return;
    }
    return;
  }

void setLastSwitchTimeForSensor(int SENSOR_PIN, unsigned long switchTime) {
    switch(SENSOR_PIN){
      case FRONT_DOOR_STATUS_PIN: FRONT_DOOR_LAST_SWITCH_TIME = switchTime;
      case BACK_DOOR_STATUS_PIN: BACK_DOOR_LAST_SWITCH_TIME = switchTime;
      case DECK_DOOR_STATUS_PIN: DECK_DOOR_LAST_SWITCH_TIME = switchTime;
      default : return;
    }
    return;
  }

String statusTopicForSensor(int SENSOR_PIN) {
  switch(SENSOR_PIN){
    case FRONT_DOOR_STATUS_PIN: return FRONT_DOOR_STATUS_TOPIC;
    case BACK_DOOR_STATUS_PIN: return BACK_DOOR_STATUS_TOPIC;
    case DECK_DOOR_STATUS_PIN: return DECK_DOOR_STATUS_TOPIC;
    default : return "ERROR";
    }
    return "ERROR";
  }

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String strTopic = String((char*)topic);
  String payloadValue = String((char*)payload);

  Serial.print("Topic - ");
  Serial.println(strTopic);

  Serial.print("Payload - ");
  Serial.println(payloadValue);
}



