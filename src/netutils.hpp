/**
 * Real Time Protocol Music Instrument Digital Interface Daemon
 * Copyright (C) 2019 David Moreno Montero <dmoreno@coralbits.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <cstdint>
#include <string_view>
#include "./exceptions.hpp"
#include "./logger.hpp"

namespace rtpmidid{
  struct parse_buffer_t {
    uint8_t *start;
    uint8_t *end;
    uint8_t *position;

    parse_buffer_t(uint8_t *start, uint8_t *end, uint8_t *position){
      this->start = start;
      this->end = end;
      this->position = position;
    }
    parse_buffer_t(uint8_t *start, uint32_t size){
      this->start = start;
      this->end = start + size;
      this->position = start;
    }

    void check_enought(int nbytes) const{
      if (position + nbytes > end)
        throw exception("Try to access end of buffer at {}", (position - start) + nbytes);
    }
    void assert_valid_position() const{
      if (position > end)
        throw exception("Invalid buffer position {}", position - start);
    }

    // This is used for writing to it, says current length
    uint32_t capacity() const{
      return position - start;
    }

    // This is the total size, from start to end
    uint32_t size() const{
      return end - start;
    }

    uint32_t read_uint32(){
      check_enought(4);
      auto data = position;
      position += 4;
      return ((uint32_t)data[0]<<24) + ((uint32_t)data[1]<<16) + ((uint32_t)data[2]<< 8) + ((uint32_t)data[3]);
    }

    uint64_t read_uint64(){
      check_enought(8);
      auto data = position;
      position += 8;
      return (
        ((uint64_t)data[0]<<56) + ((uint64_t)data[1]<<48) + ((uint64_t)data[2]<<40) + ((uint64_t)data[3]<<32) +
        ((uint64_t)data[4]<<24) + ((uint64_t)data[5]<<16) + ((uint64_t)data[6]<< 8) + ((uint64_t)data[7])
      );
    }

    uint16_t read_uint16(){
      check_enought(2);
      auto data = position;
      position += 2;
      return ((uint16_t)data[0]<< 8) + ((uint16_t)data[1]);
    }

    uint8_t read_uint8(){
      check_enought(1);
      auto data = position;
      position += 1;
      return data[0];
    }

    // The returned str is the address inside the buffer.
    std::string read_str0(){
      auto *strstart = position;
      while (*position && position < end){
        position++;
      }
      // Normally I stopped because of *position == 0.. But I might have got out of range
      // If I'm on range, construct the std::string up tp pos
      std::string ret((char*)strstart, size_t(position - strstart));
      position++;
      return ret;
    }

    void print_hex(bool to_end=false) const{
      auto data = start;
      auto n = (to_end ? end : position) - data;
      for( int i=0 ; i<n ; i++ ){
        printf("%02X ", data[i] & 0x0FF);
        if (i % 4 == 3)
          printf(" ");
        if (i % 16 == 15)
          printf("\n");
      }
      printf("\n");
      for( int i=0 ; i<n ; i++ ){
        if (isprint(data[i])){
          printf("%c", data[i]);
        }
        else{
          printf(".");
        }
        if (i % 4 == 3)
          printf(" ");
        if (i % 16 == 15)
          printf("\n");
      }
      printf("\n");
    }

    void write_uint8(uint16_t n){
      check_enought(1);
      *position++ = (n & 0x0FF);
    }
    void write_uint16(uint16_t n){
      check_enought(2);
      *position++ = (n>>8) & 0x0FF;
      *position++ = (n & 0x0FF);
    }

    void write_uint32(uint32_t n){
      check_enought(4);
      *position++ = (n>>24) & 0x0FF;
      *position++ = (n>>16) & 0x0FF;
      *position++ = (n>>8) & 0x0FF;
      *position++ = (n & 0x0FF);
    }
    void write_uint64(uint64_t n){
      check_enought(8);
      *position++ = (n>>56) & 0x0FF;
      *position++ = (n>>48) & 0x0FF;
      *position++ = (n>>40) & 0x0FF;
      *position++ = (n>>32) & 0x0FF;

      *position++ = (n>>24) & 0x0FF;
      *position++ = (n>>16) & 0x0FF;
      *position++ = (n>>8) & 0x0FF;
      *position++ = (n & 0x0FF);
    }

    void write_str0(const std::string_view &view){
      for (auto c: view){
        *position++ = c;
      }
      *position++ = '\0';
    }

    /// Copies from position to the end
    void copy_from(parse_buffer_t &from){
      copy_from(from, from.end - from.position);
    }

    void copy_from(parse_buffer_t &from, uint32_t chars){
      check_enought(chars);
      from.check_enought(chars);
      memcpy(position, from.position, chars);
      position += chars;
      from.position += chars;
    }

    bool compare(const parse_buffer_t &other){
      if (size() != other.size())
        return false;
      uint32_t i=0;
      auto l = size();
      for (i=0;i<l;i++){
        if (other.start[i] != start[i])
          return false;
      }
      return true;
    }
  };

  uint8_t *raw_write_uint16(uint8_t *data, uint16_t n);
  void read_label(parse_buffer_t &buffer, parse_buffer_t &label);
  void write_label(parse_buffer_t &data, const std::string_view &name);
}
