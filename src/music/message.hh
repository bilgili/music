/*
 *  This file is part of MUSIC.
 *  Copyright (C) 2009 INCF
 *
 *  MUSIC is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  MUSIC is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MUSIC_MESSAGE_HH

#include <music/index_map.hh>

namespace MUSIC {

  class Message {
  public:
    double t;
    int id;
    Message (double t_, int id_) : t (t_), id (id_) { }
    bool operator< (const Message& other) const { return t < other.t; }
  };

  class MessageHandlerGlobalIndex {
  public:
    virtual void operator () (double t, GlobalIndex id) = 0;
  };
  
  class MessageHandlerGlobalIndexDummy : public MessageHandlerGlobalIndex {
  public:
    virtual void operator () (double t, GlobalIndex id) { };
  };
  
  class MessageHandlerGlobalIndexProxy
    : public MessageHandlerGlobalIndex {
    void (*messageHandler) (double t, void* msg, size_t size);
  public:
    MessageHandlerGlobalIndexProxy () { }
    MessageHandlerGlobalIndexProxy (void (*mh) (double t,
						void* msg,
						size_t size))
      : messageHandler (mh) { }
    void operator () (double t, GlobalIndex id)
    {
      //*fixme*
      char c = id;
      messageHandler (t, &c, 1);
    }
  };

  //*fixme*
  class MessageHandlerPtr {
    union {
      MessageHandlerGlobalIndex* global;
    } ptr;
  public:
    MessageHandlerPtr () { }
    MessageHandlerPtr (MessageHandlerGlobalIndex* p) { ptr.global = p; }
    MessageHandlerGlobalIndex* global () { return ptr.global; }
  };
  
}

#define MUSIC_MESSAGE_HH
#endif
