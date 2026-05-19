/**
 * @file Message.h
 *
 * COMM messages.
 */

#pragma once

#include "rocket/rocket.h"

#include <string>

namespace rocket::comm {

// #Message -------------------------------------------------------------------------------------------------

struct Message {
  static u64 parseSize(std::string_view input);

  Message() = default;

  Message(u64 size);

  Message(const std::string& payload) : payload_(payload) {}

  std::string display() const;

  std::string& payload() { return payload_; }

  const std::string& payload() const { return payload_; }

  u64 size() const { return payload_.size(); }

private:

  std::string payload_;
};

} // namespace rocket::comm
