#include "connection.h"
#include <vector>
#include <string>



std::vector<std::vector<int>> subscribe_map = {
  {1},    // sensor app => comm app
  {0, 2}, // comm app => sensor app, control app
  {0}     // control app => sensor app
};

/*///////////////////////////////////////////////////////
  MESSAGE CLASS
*////////////////////////////////////////////////////////

Message::Message(int app_id) : app_id(app_id), message_id(0) {}

Message::~Message() {}

char* Message::get_data() {
  snprintf(this->buffer, sizeof(buffer), "%d, %d, %s", this->app_id, this->message_id, this->message.data());
  return this->buffer;
}

void Message::set_message_id(int message_id) {
  this->message_id = message_id;
}

void Message::set_data(const std::string& data) {
  this->message = data;
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////