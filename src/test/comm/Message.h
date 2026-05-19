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

/**
 * A COMM message.
 */
struct Message {
  /**
   * Parses a size from a string.
   *
   * @param input the input string, e.g. `"16b"`, `"100k"`, `"1m"`, or `"1g"`
   * @return the size in bytes
   */
  static u64 parseSize(std::string_view input);

  /// @ctor_default
  Message() = default;

  /**
   * Makes a message with a payload of @p size bytes.
   *
   * @param size the size of the payload in bytes
   */
  Message(u64 size);

  /**
   * Makes a message with the payload @p payload.
   *
   * @param payload the payload
   */
  Message(const std::string& payload) : payload_(payload) {}

  /**
   * Returns a string representation of the message.
   *
   * @return a string representation of the message
   */
  std::string display() const;

  /**
   * Returns the payload of the message.
   *
   * @return the payload of the message
   */
  std::string& payload() { return payload_; }

  /**
   * Returns the payload of the message.
   *
   * @return the payload of the message
   */
  const std::string& payload() const { return payload_; }

  /**
   * Returns the size of the payload.
   *
   * @return the size of the payload
   */
  u64 size() const { return payload_.size(); }

private:

  std::string payload_;
};

} // namespace rocket::comm
