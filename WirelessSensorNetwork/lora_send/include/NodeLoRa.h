#pragma once

// Hàm setup LoRa
void setupNodeLoRa();

// Hàm "thần thánh" cho team: Chỉ việc ném biến vào đây
// Trả về 1 nếu gửi thành công (được Master gọi), trả về 0 nếu đang chờ
int guiDuLieu(float nhietDo, float doAm);