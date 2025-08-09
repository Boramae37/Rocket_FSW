#include "connection.h"
#include <vector>
#include <string>
#include <cstring> // for memset, strncpy
#include <unordered_map>
#include <algorithm>

#include "spdlog/fmt/fmt.h"

// std::vector<std::vector<int>> subscribe_map = {
//   {1},    // sensor app => comm app
//   {0, 2}, // comm app => sensor app, control app
//   {0}     // control app => sensor app
// };

std::unordered_map<int, std::vector<int>> subscribe_map{
    {SENSOR_ID, {COMM_ID, CONTROL_ID}}, // sensor app의 구독자 
    {COMM_ID, {SENSOR_ID, CONTROL_ID}}, // comm app => sensor app, control app
    {CONTROL_ID, {COMM_ID}} // control app => comm
};

/*///////////////////////////////////////////////////////
  MESSAGE CLASS
*////////////////////////////////////////////////////////

Message::Message(int app_id) : app_id(app_id), message_id(0), message_length(0) {}

Message::~Message() {}

char* Message::get_message() {
  return this->buffer;
}

void Message::set_message_id(int message_id) {
  this->message_id = message_id;
}

void Message::set_data(char* data) {
  // strncpy(this->message, data, sizeof(this->message) - 1);
  // this->message[sizeof(this->message) - 1] = '\0'; // Ensure null termination
  
  // snprintf guarantees null-termination
  //this->message_length = snprintf(this->buffer, sizeof(buffer), "%d,%d,%s%c", this->app_id, this->message_id, data, TELE_END);
  
  auto ret = fmt::format_to_n(this->buffer , sizeof(this->buffer)-1, "{},{},{},{}", this->app_id, this->message_id, data, TELE_END);
  *ret.out = '\0';
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
