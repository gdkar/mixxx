
_Pragma("once")
class Console {
  public:
    Console();
    virtual ~Console();
  private:
#ifdef __WINDOWS__
    unsigned int m_oldCodePage;
    bool m_shouldResetCodePage;
#endif
};
