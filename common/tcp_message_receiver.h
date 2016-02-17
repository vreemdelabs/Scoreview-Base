
#define MESSAGE_BUFFER_SIZE (2 * 4096)

class Creceived_message
{
 public:
  Creceived_message();
  ~Creceived_message();

  void add_data(char *pbuffer, int size);
  bool available_message(); 
  void get_message(char *pbuffer, int buffer_size, int *psize);

 private:
  void reset();

 private:
  char *m_pmessage_buffer;
  int   m_index;
  int   m_message_size;
  bool  m_bavailable;
};
