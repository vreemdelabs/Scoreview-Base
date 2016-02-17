

#include <stdio.h>
#include <string.h>

#include <string>
#include <list>

#include "messages.h"
#include "audioapi.h"
#include "score.h"
#include "message_decoder.h"
#include "tcp_message_receiver.h"

//-------------------------------------------------------------------------------
// Message reception
// add_data returns true when the complete message is received
//-------------------------------------------------------------------------------
Creceived_message::Creceived_message():
  m_index(0),
  m_message_size(0)
{
  m_pmessage_buffer = new char[MESSAGE_BUFFER_SIZE];
}

Creceived_message::~Creceived_message()
{
  delete[] m_pmessage_buffer;
}

void Creceived_message::reset()
{
  m_index = m_message_size = 0;
  m_bavailable = false;
}

void Creceived_message::add_data(char *pbuffer, int size)
{
  bool     bfail;
  //int32_t *ptotal_message_size;
  int     *ptotal_message_size;

  bfail = false;
  //printf("Message copy of %d bytes.\n", size);
  memcpy(&m_pmessage_buffer[m_index], pbuffer, size);
  m_index += size;
  if (m_index > HEADER_SIZE)
    {
      if (m_message_size == 0)
	{
	  // Check the Magic number
	  bfail = (m_pmessage_buffer[0] != (NMAGICN & 0xFF) && m_pmessage_buffer[1] != ((NMAGICN >> 8) & 0xFF));
	  if (bfail)
	    {
	      printf("Error: wrong message's magic number: 0x%x%x.\n", m_pmessage_buffer[1], m_pmessage_buffer[0]);
	      reset();
	    }
	  else
	    {
	      // (m_message_size == 0), if is the first chunk of data then get the total message size
	      // Get the message size
	      //ptotal_message_size = (int32_t*)(&m_pmessage_buffer[2]);
	      ptotal_message_size = (int*)(&m_pmessage_buffer[2]);
	      m_message_size = *ptotal_message_size;
	      m_message_size += HEADER_SIZE;
	      printf("Message size is %d\n", m_message_size);
	      if (m_message_size >= MESSAGE_BUFFER_SIZE)
		{
		  printf("Message overflows\n");
		  bfail = true;
		}
	    }
	}
      else
	{
	  // Get the end of a message and the following shit if needed
	}
    }
  // Check if the message was received
  m_bavailable = (!bfail && m_index >= m_message_size && m_message_size != 0);
  if (bfail)
    reset();
}

bool Creceived_message::available_message()
{
  return m_bavailable;
}

void Creceived_message::get_message(char *pbuffer, int buffer_size, int *psize)
{
  int  next_size;
  char tmp_buffer[MESSAGE_BUFFER_SIZE];

  *psize = 0;
  if (buffer_size >= m_index && m_index >= m_message_size)
    {
      memcpy(pbuffer, m_pmessage_buffer, m_message_size);
      pbuffer[m_message_size] = 0;
      *psize = m_message_size;
      // If we have only on message in the buffer send it and reset
      if (m_index == m_message_size)
	{
	  reset();
	}
      else
	{
	  // Otherwise send this message and try to get the next data
	  // Copy the next data to the begining
	  next_size = m_index - m_message_size;
	  memcpy(tmp_buffer, m_pmessage_buffer + m_message_size, next_size);
	  reset();
	  //--------------------------------
	  // Next message
	  //--------------------------------
	  add_data(tmp_buffer, next_size);
	}
    }
  else
    {
      if (buffer_size >= m_index)
	printf("Warning in tcp ip messages: not enough buffer to store the message.\n");
      else
	printf("Warning in tcp ip messages: size error.\n");
      reset();
    }
}

