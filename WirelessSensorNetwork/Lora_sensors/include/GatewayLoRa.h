#pragma once
#include <Arduino.h>

void setupGatewayLoRa();
// Hàm này sẽ hỏi 1 ID, đợi trả lời và trả về chuỗi dữ liệu của Node đó
String pollNode(int targetID);