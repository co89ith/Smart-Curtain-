/* This code is for Adafruit Feather Huzz
*/

#include <ESP8266WiFi.h>        // 本程序使用 ESP8266WiFi库
#include <ESP8266WiFiMulti.h>   //  ESP8266WiFiMulti库
#include <ESP8266WebServer.h>   //  ESP8266WebServer库
 
ESP8266WiFiMulti wifiMulti;     // 建立ESP8266WiFiMulti对象,对象名称是 'wifiMulti'
 
ESP8266WebServer esp8266_server(80);// 建立网络服务器对象，该对象用于响应HTTP请求。监听端口（80）
 
// Define pin connections & motor's steps per revolution
const int dirPin = 0; // Direction pin - change as needed
const int stepPin = 13; // Step pin - change as needed
const int sleepPin = 16; // Enable pin - change as needed
const int stepsPerRevolution = 200; // Change this depending on your stepper motor
 
char cmd;  //用户指令字符
int data;  //用户指令数据
int motorSpeed = 2000;  //电机转速(数值越小速度越小)

void setup(void){
  // Initialize the serial port
  Serial.begin(115200);
  while (!Serial); // Wait for the serial port to connect - only needed for boards with native USB
  
  Serial.println("Starting motor test...");

  // Sets the three pins as Outputs
  pinMode(stepPin, OUTPUT); 
  pinMode(dirPin, OUTPUT);
  pinMode(sleepPin, OUTPUT); // Enable pin
  
  digitalWrite(sleepPin, LOW); // Wake up the stepper motor driver (High: sleep; Low: awake)
  delay(1000); // Wait 1 second to ensure everything is fully initialized
 
 //------------------------- Wifi-----------------------------------------------
  //pinMode(LED_BUILTIN, OUTPUT); //设置内置LED引脚为输出模式以便控制LED
  
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2"); // ESP8266-NodeMCU再启动后会扫描当前网络
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3"); // 环境查找是否有这里列出的WiFi ID。如果有
  Serial.println("Connecting ...");                            // 则尝试使用此处存储的密码进行连接。
  
  int i = 0;                                 
  while (wifiMulti.run() != WL_CONNECTED) {  // 此处的wifiMulti.run()是重点。通过wifiMulti.run()，NodeMCU将会在当前
    delay(1000);                             // 环境中搜索addAP函数所存储的WiFi。如果搜到多个存储的WiFi那么NodeMCU
    Serial.print(i++); Serial.print(' ');    // 将会连接信号最强的那一个WiFi信号。
  }                                          // 一旦连接WiFI成功，wifiMulti.run()将会返回“WL_CONNECTED”。这也是
                                             // 此处while循环判断是否跳出循环的条件。
  
  // WiFi连接成功后将通过串口监视器输出连接成功信息 
  Serial.println('\n');
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());              // 通过串口监视器输出连接的WiFi名称
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());           // 通过串口监视器输出ESP8266-NodeMCU的IP
 
  esp8266_server.begin();                           // 启动网站服务
  esp8266_server.on("/", HTTP_GET, handleRoot);     // 设置服务器根目录即'/'的函数'handleRoot', 请求方法：HTTP_GET（通过http，发送get请求）
  esp8266_server.on("/Motor", HTTP_POST, handleMotor);  // 设置处理LED控制请求的函数'handleMotor'， 浏览器通过HTTP_POST方法向/LED页面时，调用handleMotor
  esp8266_server.onNotFound(handleNotFound);        // 设置处理404情况的函数'handleNotFound'
 
  Serial.println("HTTP esp8266_server started");//  告知用户ESP8266网络服务功能已经启动
}
 

// ----------------Serve visit and motor control-------------------

void loop() {
  // 检查http服务器访问
  esp8266_server.handleClient();                     
  // Check for serial input at the beginning of each loop iteration
  
  if (Serial.available()) {     // 检查串口缓存是否有数据等待传输 
    cmd = Serial.read();        // 获取电机指令中电机编号信息    
    Serial.print("cmd = ");
    Serial.print(cmd);    
    Serial.print(" , "); 
 
    data = Serial.parseInt();
    Serial.print("data = ");
    Serial.print(data);   
    Serial.println("");    
 
    runUsrCmd();
    // digitalWrite(sleepPin, LOW);
  }
}

/*设置服务器根目录即'/'的函数'handleRoot'
  该函数的作用是每当有客户端访问NodeMCU服务器根目录时，
  NodeMCU都会向访问设备发送 HTTP 状态 200 (Ok) 这是send函数的第一个参数。
  同时NodeMCU还会向浏览器发送HTML代码，以下示例中send函数中第三个参数，
  也就是双引号中的内容就是NodeMCU发送的HTML代码。该代码可在网页中产生LED控制按钮。 
  当用户按下按钮时，浏览器将会向NodeMCU的/LED页面发送HTTP请求，请求方式为POST。
  NodeMCU接收到此请求后将会执行handleMotor函数内容*/

// void handleRoot() {       
//   esp8266_server.send(200, "text/html", "<form action=\"/Motor\" method=\"POST\"><input type=\"submit\" value=\"Run motor\"></form>");
// }
void handleRoot() {       
  esp8266_server.send(200, "text/html", R"(
    <form action="/Motor" method="POST">
      <label><input type="checkbox" name="sleep" value="1"> Sleep Mode</label><br>
      <label><input type="radio" name="direction" value="0"> Clockwise</label><br>
      <label><input type="radio" name="direction" value="1"> Counter Clockwise</label><br>
      <label>Speed (1000-20000): <input type="number" name="speed" min="1000" max="20000" value="2000"></label><br>
      <label>Steps: <input type="number" name="steps" min="1" value="800"></label><br>
      <input type="submit" value="Run Motor">
    </form>
  )");
}


void handleMotor() {
  String direction = esp8266_server.arg("direction");
  String speedStr = esp8266_server.arg("speed");
  String stepsStr = esp8266_server.arg("steps");
  String sleepMode = esp8266_server.arg("sleep");

  int speed = speedStr.toInt();
  int steps = stepsStr.toInt();
  bool isSleep = !sleepMode.isEmpty(); // Sleep mode is enabled if checkbox is checked

  // Set direction
  digitalWrite(dirPin, direction.toInt());

  // Set sleep mode
  digitalWrite(sleepPin, isSleep ? HIGH : LOW); // Sleep mode active if checkbox is checked

  if (!isSleep) {
    // Set speed - Adjust as necessary based on your motor's specifications
    int delayTime = 60000000 / speed / stepsPerRevolution; // Calculate delay per step for the desired speed
    delayTime = constrain(delayTime, 50, 5000); // Ensure delay time is within a practical range

    // Run stepper
    runStepper(speed, steps);
  }

  // Provide feedback or redirect as necessary
  esp8266_server.sendHeader("Location", "/");
  esp8266_server.send(303);
}


// 
 
// 设置处理404情况的函数'handleNotFound'
void handleNotFound(){
  esp8266_server.send(404, "text/plain", "404: Not found"); // 发送 HTTP 状态 404 (未找到页面) 并向浏览器发送文字 "404: Not found"
}


//此函数用于运行用户指令
void runUsrCmd(){
  switch(cmd){ 
    case 'x':    // 设置步进电机旋转(顺时针/逆时针)
      Serial.print("Set Rotation To "); 
      if (data == 0){
        digitalWrite(dirPin, 0);
        Serial.println("Clockwise."); 
      } else {
        digitalWrite(dirPin, 1);
        Serial.println("Counter Clockwise."); 
      }
      break;

    case 'm':  // 设置A4988 enable功能
      Serial.print("Set Motor To "); 
      if (data == 0){
        digitalWrite(sleepPin, 1);
        Serial.println("Sleep."); 
      } else {
        digitalWrite(sleepPin, 0);
        Serial.println("Awake."); 
      }
      break;

    case 'z': // 设置步进电机运行步数 200steps=1revo
      runStepper(motorSpeed, data);
      break;
 
    case 'd': // 设置步进电机运行速度 1000-20000
      motorSpeed = data;
      Serial.print("Set Motor Speed To ");
      Serial.println(data);
      break;

    case 's':
      digitalWrite(sleepPin, HIGH); // Disable the driver, effectively stopping the motor
      break;
          
    default:  // 未知指令
      Serial.println("Unknown Command");
  }
}
 
//运行步进电机
void runStepper (int rotationSpeed, int stepNum){
  for(int x = 0; x < stepNum; x++) {
    digitalWrite(stepPin,HIGH); 
    delayMicroseconds(rotationSpeed); 
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(rotationSpeed); 
  }  
}



